#include "broker.h"
#include "mosquitto.h"
#include <errno.h>
#include <string.h>

static int clean_global(struct g_broker_s *broker);


struct g_broker_s g_broker = {
	.big_mutex = PTHREAD_MUTEX_INITIALIZER,
	.pub_list_mutex = PTHREAD_MUTEX_INITIALIZER,
	.sub_list_mutex = PTHREAD_MUTEX_INITIALIZER,
	.unsub_list_mutex = PTHREAD_MUTEX_INITIALIZER,
	.cb_list_mutex = PTHREAD_MUTEX_INITIALIZER,
	.srv_list_mutex = PTHREAD_MUTEX_INITIALIZER,
	
	.connect_stat = -1,
	.log_print_cb = printf,
};

static void connect_callback(struct mosquitto *mosq, void *obj, int rc, int flags)
{
	struct g_broker_s *broker = (struct g_broker_s *)obj;

	broker_log("%s : rc=%d\n",__func__,rc);
	
	pthread_mutex_lock(&(broker->cont_waite_ack.mutex));
	broker->cont_waite_ack.mid = 0;
	g_broker.connect_stat = rc;
	pthread_cond_signal(&(broker->cont_waite_ack.cond));
	pthread_mutex_unlock(&(broker->cont_waite_ack.mutex));
				
exit:
	return;
}

static void disconnect_callback(struct mosquitto *mosq, void *obj, int result)
{
	broker_log("%s\n",__func__);
} 

static void publish_callback(struct mosquitto *mosq, void *obj, int mid)
{	
	struct g_broker_s *broker = (struct g_broker_s *)obj;
	struct list_head *pos, *n;
	struct waite_ack_s *waite_ack;
	unsigned int i = 0;

	broker_log("%s -mid(%d)\n",__func__,mid);

	list_for_each_safe(pos,n,&(broker->pub_list)){
		waite_ack = list_entry(pos, struct waite_ack_s, next);
		//g_broker.log_print_cb("%s2 -p(%p-%p - %p), i(%d) id:%s\n",__func__,n, pos,&(broker->pub_list), i++,g_broker.id);
		if(waite_ack){
			//g_broker.log_print_cb("%s2 -mid(%d-%d), waite_ack(%p)\n",__func__,mid, waite_ack->mid, waite_ack);
			if(!wakeup_waite_ack(waite_ack, mid))	break;
		}
	}

exit:
	return;
}

static void subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{	
	int ret;
	struct g_broker_s *broker = (struct g_broker_s *)obj;
	struct list_head *pos, *n;
	struct waite_ack_s *waite_ack;

	broker_log("%s: mid(%d)\n",__func__,mid);
	
	list_for_each_safe(pos,n,&(broker->sub_list)){
		waite_ack = list_entry(pos, struct waite_ack_s, next);
		if(waite_ack){
			if(!wakeup_waite_ack(waite_ack, mid))	break;
		}
	}

	
exit:
	return;
}
static void unsubscribe_callback(struct mosquitto *mosq, void *obj, int mid)
{	
	int ret;
	struct g_broker_s *broker = (struct g_broker_s *)obj;
	struct list_head *pos, *n;
	struct waite_ack_s *waite_ack;

	broker_log("%s\n",__func__);
	
	list_for_each_safe(pos,n,&(broker->unsub_list)){
		waite_ack = list_entry(pos, struct waite_ack_s, next);
		if(waite_ack){
			if(!wakeup_waite_ack(waite_ack, mid))	break;
		}
	}

	
exit:
	return;
}

static void* cb_thread(void *args)
{
	struct msg_priv *msg = (struct msg_priv *)args;
	struct g_broker_s *broker = (struct g_broker_s *)msg->arg;

	broker_log("%s- buf(%p),buf_len(%d)\n",__func__,msg->buf,msg->buf_len);
	
	msg->recv_callback(msg->buf, msg->buf_len, msg->from, broker->usr_arg);

	if(msg->buf){
		xfree(msg->buf);
		msg->buf_len = 0;
		//dev->buf = NULL;
	}
	pthread_detach(msg->cb_pid); //回收资源 
	return NULL;
}


static void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	int ret;
	bool res;
	struct list_head *pos, *n;
	struct dev_priv *dev, *found = NULL;
	struct waite_ack_s *waite_ack;
	struct g_broker_s *broker = (struct g_broker_s *)obj;
	struct msg_priv *msg = NULL;

	broker_log("%s - top=%s\n",__func__, message->topic);

	//先检查req/respond
	list_for_each_safe(pos, n, &(g_broker.srv_list)){
		waite_ack = list_entry(pos, struct waite_ack_s, next);
		if(waite_ack){
				dev = container_of(waite_ack, struct dev_priv, waite);
			//if(!strcmp(message->message, dev->topic)){
				//printf("xsrv %s - top=%s(mid:%d,waite_ack=%p) id=%s\n",__func__, broker->respond_topic,waite_ack->mid,waite_ack,g_broker.id);
				ret = mosquitto_topic_matches_sub(broker->respond_topic, message->topic, &res);
				//printf("ret = %d res=%d\n",ret, res);
				//printf("waite_ack next:%p : prev: %p\n",waite_ack->next.next,waite_ack->next.prev);
				if(!res){
					continue;
				}
				dev->buf = (char*)xmalloc(message->payloadlen);
				if(dev->buf){
					dev->buf_len = message->payloadlen;
					memcpy(dev->buf, message->payload, dev->buf_len);
				}
				//printf("del srv_listnext:%p : srv_listprev: %p\n",g_broker.srv_list.next,g_broker.srv_list.prev);
				wakeup_waite_ack(waite_ack, 1);
				//printf("delx srv_listnext:%p : srv_listprev: %p\n",g_broker.srv_list.next,g_broker.srv_list.prev);
				found = dev;
				break;
			//}
		}
	}
	broker_log("%s - found=%p\n",__func__, found);

	if(!found){
		list_for_each_safe(pos, n, &(g_broker.cb_list)){
			waite_ack = list_entry(pos, struct waite_ack_s, next);
			if(waite_ack){
				//if(!strcmp(topic, dev->topic)){
					dev = container_of(waite_ack, struct dev_priv, waite);
					//printf("%s - top=%s\n",__func__, dev->topic);
					mosquitto_topic_matches_sub(dev->topic, message->topic, &res);
					if(!res){
						continue;
					}
					//尽量开一个线程处理

					msg = (struct msg_priv*)xmalloc(sizeof(struct msg_priv));
					if(msg){
						msg->buf = (char*)xmalloc(message->payloadlen);
						if(msg->buf){
							msg->buf_len = message->payloadlen;
							memcpy(msg->buf, message->payload, msg->buf_len);
							strcpy(msg->from, message->topic);
							msg->arg = obj;
							msg->recv_callback = dev->recv_callback;
							ret = pthread_create(&(msg->cb_pid), NULL, cb_thread, (void*)msg);
						}else{
							xfree(msg);
						}
					}
					
					//printf("%s - pthread_create=%d - %d\n",__func__, ret, dev->cb_pid);
					if((!msg) || (!msg->buf) || ret){
						//printf("%s-recv_callback(%p)\n",__func__,dev->recv_callback);
						dev->recv_callback(message->payload, message->payloadlen, message->topic, broker->usr_arg);
					}
					//
					//break; //搜索完整个链表, 因为有通配符
				//}
			}
		}
	}
	
	return ;
}


static void client_id_generate(const char *id_base, struct g_broker_s *brk)
{
	int ret = 0;
	int len;
	char hostname[256];

	hostname[0] = '\0';
	gethostname(hostname, 256);
	hostname[255] = '\0';
	len = strlen(id_base) + strlen("|-") + 6 + strlen(hostname);
	
	snprintf(brk->id, len, "%s|%d-%s", id_base, getpid(), hostname);
	
	if(strlen(brk->id) > MOSQ_MQTT_ID_MAX_LENGTH){
		/* Enforce maximum client id length of 23 characters */
		brk->id[MOSQ_MQTT_ID_MAX_LENGTH] = '\0';
	}
	
	return;
}

static void* mqtt_thread(void *args)
{
	int ret = 0;
	struct g_broker_s *broker = (struct g_broker_s *)args;
	struct waite_ack_s *waite_ack = &(g_broker.discont_waite_ack);

	if(mosquitto_connect(broker->mosq, broker->host, broker->host_port, MQTT_KEEPLIVE)){
		ret = -errno;
		goto exit;
	}

	ret = mosquitto_loop_forever(broker->mosq, -1, 1);
	
exit:
	if(broker->excep_callback){
		broker->excep_callback(broker->usr_arg);
	}
	
	
	if(broker->mosq){
		mosquitto_destroy(broker->mosq);
		mosquitto_lib_cleanup();
		broker->mosq = NULL;
		clean_global(broker);
	}
	broker->connect_stat = -1;

	if(waite_ack->mid){
		pthread_mutex_lock(&(waite_ack->mutex));
		waite_ack->mid = 0;
		pthread_cond_signal(&(waite_ack->cond));
		pthread_mutex_unlock(&(waite_ack->mutex));
	}
	broker_log("%s end",__func__);
	return NULL;
}

void log_callback(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	broker_log("%s\n", str);
	//log_print_cb(str);
}

int init_broker(char *src, char *host, int port, exception_cb excep_cb, void *arg)
{
	int ret;
	struct timespec timeout;
	struct waite_ack_s waite_ack;

	pthread_mutex_lock(&(g_broker.big_mutex));

	if(!g_broker.connect_stat){
		ret = -EEXIST;
		goto exit0;
	}
	
	ret = mosquitto_lib_init();
	if(ret){
		ret = -errno;
		goto exit0;
	}

	client_id_generate(src, &g_broker);
	
	g_broker.mosq = mosquitto_new(g_broker.id, true, &g_broker);
	if(!g_broker.mosq){
		ret = -errno;
		goto exit1;
	}

	if(NULL == host){
		strcpy(g_broker.host, "localhost");
	}else{
		strcpy(g_broker.host, host);
	}

	if(port <= 0){
		port = 1883;
	}

	g_broker.host_port = port;

	INIT_LIST_HEAD(&(g_broker.cb_list));
	INIT_LIST_HEAD(&(g_broker.srv_list));

	INIT_LIST_HEAD(&(g_broker.pub_list));
	INIT_LIST_HEAD(&(g_broker.sub_list));
	INIT_LIST_HEAD(&(g_broker.unsub_list));


	//connect
	ret = pthread_mutex_init(&(g_broker.cont_waite_ack.mutex), NULL);
	if(ret){
		goto exit1;
	}
	
	ret = pthread_cond_init(&(g_broker.cont_waite_ack.cond), NULL);
	if(ret){
		goto exit1;
	}

	//disconnect
	ret = pthread_mutex_init(&(g_broker.discont_waite_ack.mutex), NULL);
	if(ret){
		goto exit1;
	}
	
	ret = pthread_cond_init(&(g_broker.discont_waite_ack.cond), NULL);
	if(ret){
		goto exit1;
	}

	g_broker.cont_waite_ack.mid = -1;
	
	mosquitto_log_callback_set(g_broker.mosq, log_callback);
	mosquitto_connect_with_flags_callback_set(g_broker.mosq, connect_callback);
	mosquitto_disconnect_callback_set(g_broker.mosq, disconnect_callback);
		
	ret = pthread_create(&(g_broker.mqtt_pid), NULL, mqtt_thread, (void*)&g_broker);
	if(ret){
		goto exit1;
	}
	pthread_detach(g_broker.mqtt_pid); //回收资源 

	pthread_mutex_lock(&(g_broker.cont_waite_ack.mutex));
	if(g_broker.cont_waite_ack.mid){
		get_timeout_val(&timeout);
		ret = pthread_cond_timedwait(&(g_broker.cont_waite_ack.cond), &(g_broker.cont_waite_ack.mutex), &timeout);
	}
	pthread_mutex_unlock(&(g_broker.cont_waite_ack.mutex));

	if(ret || g_broker.connect_stat){
		broker_log("connect err");
		goto exit2;
	}else{

		mosquitto_publish_callback_set(g_broker.mosq, publish_callback);
		mosquitto_subscribe_callback_set(g_broker.mosq, subscribe_callback);
		mosquitto_unsubscribe_callback_set(g_broker.mosq, unsubscribe_callback);
		mosquitto_message_callback_set(g_broker.mosq, message_callback);
	
		snprintf(g_broker.respond_topic,ARRAY_SIZE(g_broker.respond_topic),RESPOND_TOPIC, src);
		snprintf(g_broker.src,ARRAY_SIZE(g_broker.src),"%s", src);
		g_broker.req_resp_seq = 0;
		waite_ack.list_lock = &g_broker.sub_list_mutex;
		waite_ack_init(&waite_ack, &(g_broker.sub_list), 0);
		ret = mosquitto_subscribe(g_broker.mosq, &(waite_ack.mid), g_broker.respond_topic, RESPOND_QOS); //订阅所有的响应主题
		broker_log("%s :mosquitto_subscribe=%d - %d\n",__func__,ret,waite_ack.mid);
		ret = waite_ack_timeout(&waite_ack, !ret);
		
		broker_log("%s :%d - %d, connect_stat=%d - %d\n",__func__,ret, EINVAL,g_broker.connect_stat, waite_ack.mid);
		broker_log("%s :src(%s),g_broker.src(%s) \n",__func__, src, g_broker.src);
	} 
	
	g_broker.excep_callback = excep_cb;
	g_broker.usr_arg = arg;
	
exit2:
	if(g_broker.connect_stat){
		mosquitto_disconnect(g_broker.mosq);
		ret = g_broker.connect_stat;
	}

exit1:
	
	if(ret){
		mosquitto_lib_cleanup();
	}

exit0:
	//bugs   mutex con  失败后 没有destroy. 查阅glibc 貌似也没啥风险
	pthread_mutex_unlock(&(g_broker.big_mutex));

	if(ret){
		pthread_mutex_destroy(&(g_broker.big_mutex));
		pthread_mutex_destroy(&(g_broker.discont_waite_ack.mutex));
		pthread_mutex_destroy(&(g_broker.cont_waite_ack.mutex));

		pthread_cond_destroy(&(g_broker.discont_waite_ack.cond));
		pthread_cond_destroy(&(g_broker.cont_waite_ack.cond));
	}
	return ret;
}

static void clean_list(struct g_broker_s *broker)
{
	int ret;
	bool res;
	struct list_head *pos, *n;
	struct dev_priv *dev, *found = NULL;
	struct waite_ack_s *waite_ack;

	//先检查req/respond
	list_for_each_safe(pos, n, &(broker->srv_list)){
		waite_ack = list_entry(pos, struct waite_ack_s, next);
		if(waite_ack){
			dev = container_of(waite_ack, struct dev_priv, waite);
			pthread_mutex_destroy(&(dev->waite.mutex));
			pthread_cond_destroy(&(dev->waite.cond));
			list_del(&(dev->waite.next));
			//free(dev); //transfer的为局部变量   不能释放
		}
	}

	if(!found){
		list_for_each_safe(pos, n, &(broker->cb_list)){
			waite_ack = list_entry(pos, struct waite_ack_s, next);
			if(waite_ack){
				dev = container_of(waite_ack, struct dev_priv, waite);
				list_del(&(dev->waite.next));
				xfree(dev);
				dev = NULL;
			}
		}
	}
	
	return ;
}

static int clean_global(struct g_broker_s *broker)
{
	broker->excep_callback = NULL;
	clean_list(broker);
}

int close_broker(void)
{
	struct waite_ack_s *waite_ack = &(g_broker.discont_waite_ack);
	struct timespec timeout;
	//clean_global(&g_broker);
	broker_log("%s(%s)\n", __func__,g_broker.id);
		
	pthread_mutex_init(&(waite_ack->mutex), NULL);
	pthread_cond_init(&(waite_ack->cond), NULL);
	waite_ack->mid = 1;


	if(g_broker.mosq){
		mosquitto_disconnect(g_broker.mosq);
	}

	pthread_mutex_lock(&(waite_ack->mutex));
	if(waite_ack->mid){
		get_timeout_val(&timeout);
		pthread_cond_timedwait(&(waite_ack->cond), &(waite_ack->mutex), &timeout);
	}
	pthread_mutex_unlock(&(waite_ack->mutex));
	
	pthread_mutex_destroy(&(g_broker.big_mutex));
	pthread_mutex_destroy(&(g_broker.discont_waite_ack.mutex));
	pthread_mutex_destroy(&(g_broker.cont_waite_ack.mutex));

	pthread_cond_destroy(&(g_broker.discont_waite_ack.cond));
	pthread_cond_destroy(&(g_broker.cont_waite_ack.cond));

	//pthread_join(g_broker.mqtt_pid,NULL);
	g_broker.log_print_cb = NULL;
	return 0;
}

static struct dev_priv* found_cb(char *src, char *type)
{
	struct list_head *pos, *n;
	struct dev_priv *dev, *found = NULL;
	struct waite_ack_s *waite_ack;
	int ret = 0;
	char topic[256];

	if(!src){
		return NULL;
	}	
	
	if(type){
		snprintf(topic, ARRAY_SIZE(topic), TOPIC_TYPE, src, type);
	}else{
		snprintf(topic, ARRAY_SIZE(topic), TOPIC_ACK, src);
	}
				
	list_for_each_safe(pos, n, &(g_broker.cb_list)){
		waite_ack = list_entry(pos, struct waite_ack_s, next);
		if(waite_ack){
			dev = container_of(waite_ack, struct dev_priv, waite);
			if(!strcmp(src, dev->topic)){
				found = dev;
				break;
			}
		}
	}

	return found;
}

static int register_cb_common(char *src, recv_cb cb, char *type)
{
	struct dev_priv *new_dev;
	int ret = 0;
	struct waite_ack_s waite_ack;

	if(!src || !cb){
		return -EINVAL;
	}
	if(found_cb(src, type)){
		return -ENOTEMPTY;
	}
	
	new_dev = (struct dev_priv *)xzalloc(sizeof(struct dev_priv));
	if(new_dev){

		if(type){
			snprintf(new_dev->topic, ARRAY_SIZE(new_dev->topic), TOPIC_TYPE, src, type);
		}else{
			snprintf(new_dev->topic, ARRAY_SIZE(new_dev->topic), TOPIC_ACK, src);
		}
		
		new_dev->is_req_rsp = NOR_OPE;
		new_dev->recv_callback = cb;
		new_dev->arg = (void*)&g_broker;
		
		list_add_tail(&(new_dev->waite.next), &(g_broker.cb_list));

		waite_ack.list_lock = &g_broker.sub_list_mutex;
		
		waite_ack_init(&waite_ack, &(g_broker.sub_list), 0);

		broker_log("%s-id(%s),recv_callback(%p),cb(%p)\n",__func__,g_broker.id,new_dev->recv_callback, cb);
	
		ret = mosquitto_subscribe(g_broker.mosq, &(waite_ack.mid), new_dev->topic, PUBLIC_QOS);
		
		ret = waite_ack_timeout(&waite_ack, !ret);
	
		if(ret){
			list_del(&(new_dev->waite.next));
			xfree(new_dev);
		}	
	}else{
		return -ENOMEM;
	}

exit:	
	return ret;

}

static int unregister_cb_common(char *src, char *type)
{
	struct list_head *pos, *n;
	struct dev_priv *dev;
	struct waite_ack_s waite_ack;
	struct waite_ack_s *waite_ack_node;
	int ret = 0;
	char topic[256];

	if(!src){
		return -EINVAL;
	}

	if(type){
		snprintf(topic, ARRAY_SIZE(topic), TOPIC_TYPE, src, type);
	}else{
		snprintf(topic, ARRAY_SIZE(topic), TOPIC_ACK, src);
	}
		
	list_for_each_safe(pos, n, &(g_broker.cb_list)){
		waite_ack_node = list_entry(pos, struct waite_ack_s, next);
		if(waite_ack_node){
			dev = container_of(waite_ack_node, struct dev_priv, waite);
			if(!strcmp(topic, dev->topic)){
				waite_ack.list_lock = &g_broker.unsub_list_mutex;
				waite_ack_init(&waite_ack, &(g_broker.unsub_list), 0);
				ret = mosquitto_unsubscribe(g_broker.mosq, &(waite_ack.mid), dev->topic);

				ret = waite_ack_timeout(&waite_ack, !ret);
				
				if(!ret){
					list_del(&(dev->waite.next));
					xfree(dev);
					break;
				}
			}
		}
	}

exit:
	return ret;
}


int register_callback(char *src, recv_cb cb, char *type)
{
	
	return register_cb_common(src, cb, type);
}

int unregister_callback(char *src, char *type)
{
	return unregister_cb_common(src, type);
}

#if 0
int register_all_callback(recv_callback *cb)
{
	return register_cb_common(NULL, cb);
}

int unregister_all_callback(void)
{
	return unregister_cb_common(NULL);
}

#endif

int dev_send(char *buf, int buf_len, char *event)
{
	int ret;
	char to[256];

	snprintf(to, 256, PUB_TOPIC, g_broker.src, event);

	ret = __dev_send(&g_broker, buf, buf_len, to);

	printf("%s\n",__func__);


exit0:
	return ret;
}

#if 1
int dev_transfer(char *buf, int buf_len, char **out_buf, int *out_buf_len, char *dst, int to_sec)
{
	//struct dev_priv *new_dev;
	char request[256];
	int ret = -ENOMEM;
	struct timeval now;
	struct timespec timeout;
	struct waite_ack_s waite_ack;
	struct dev_priv *new_dev;
	
	if((!out_buf) && (!out_buf_len) && (!buf) && (!dst)){
		ret = -EINVAL;
		goto exit;
	}
	new_dev = (struct dev_priv *)xzalloc(sizeof(struct dev_priv));
	if(new_dev){
		snprintf(new_dev->topic, ARRAY_SIZE(new_dev->topic), "%s",g_broker.respond_topic);

		snprintf(request, 256, REQUEST_TOPIC, g_broker.src, dst, g_broker.req_resp_seq++);
		
		new_dev->waite.next.next = NULL;
		new_dev->waite.next.prev = NULL;
		new_dev->waite.list_lock = &g_broker.srv_list_mutex;
		waite_ack_init(&(new_dev->waite), &(g_broker.srv_list), 1);
		/*waite_ack_init(&waite_ack, &(g_broker.pub_list), 0);
		
		ret = mosquitto_publish(g_broker.mosq, &(waite_ack.mid), request, buf_len, buf, RESPOND_QOS, RESPOND_RETAIN);

		ret = waite_ack_timeout(&waite_ack, !ret);*/
	
		ret = __dev_send(&g_broker, buf, buf_len, request);

		ret = waite_ack_timeout(&(new_dev->waite), !ret);
		if(!ret){
			*out_buf = new_dev->buf;
			*out_buf_len = new_dev->buf_len;
		}
	}else{
		return -ENOMEM;
	}


exit:
	if(ret){
		//clean
		xfree(new_dev);
	}
	return ret;
}
#else
int dev_transfer(char *buf, int buf_len, char **out_buf, int *out_buf_len, char *dst, int to_sec)
{
	//struct dev_priv *new_dev;
	char request[256];
	int ret = -ENOMEM;
	struct timeval now;
	struct timespec timeout;
	struct waite_ack_s waite_ack;
	struct dev_priv new_dev;
	
	if((!out_buf) && (!out_buf_len) && (!buf) && (!dst)){
		ret = -EINVAL;
		goto exit;
	}
	//new_dev = (struct dev_priv *)xzalloc(sizeof(struct dev_priv));
	//if(new_dev){
		snprintf(new_dev.topic, ARRAY_SIZE(new_dev.topic), "%s",g_broker.respond_topic);

		snprintf(request, 256, REQUEST_TOPIC, g_broker.src, dst, g_broker.req_resp_seq++);
	
		new_dev.waite.list_lock = &g_broker.srv_list_mutex;
		waite_ack_init(&(new_dev.waite), &(g_broker.srv_list), 1);
		/*waite_ack_init(&waite_ack, &(g_broker.pub_list), 0);
		
		ret = mosquitto_publish(g_broker.mosq, &(waite_ack.mid), request, buf_len, buf, RESPOND_QOS, RESPOND_RETAIN);

		ret = waite_ack_timeout(&waite_ack, !ret);*/
		
		ret = __dev_send(&g_broker, buf, buf_len, request);

		ret = waite_ack_timeout(&(new_dev.waite), !ret);
		if(!ret){
			*out_buf = new_dev.buf;
			*out_buf_len = new_dev.buf_len;
		}
	//}else{
	//	return -ENOMEM;
	//}


exit:
	//if(ret){
		//clean
		//xfree(new_dev);
	
	//}
	return ret;
}

#endif

int dev_send_respond(char *buf, int buf_len, char *from)
{
	int ret;
	struct timespec timeout;
	struct waite_ack_s waite_ack;
	char *src, *dst = NULL, *seq = NULL, *p;
	int split = 0;
	char topic[256];
	
	src = from;
	p = from;
	while (*p != '\0'){
		if(*p == '/'){
			*p = '\0';
			split++;
		}
		p++;

		if( (!dst) && (1 == split)){
			dst = p;
		}else if( (!seq) && (4 == split)){
			seq = p;

			break;
		}
	}

	snprintf(topic, ARRAY_SIZE(topic), RESPOND_TOPIC_ACK, dst, src, seq);
	
	ret = __dev_send(&g_broker, buf, buf_len, topic);

	broker_log("%s: %s\n",__func__,topic);


exit0:
	return ret;
}

void dev_set_log(log_print log_func)
{
	g_broker.log_print_cb = log_func;
	
}
