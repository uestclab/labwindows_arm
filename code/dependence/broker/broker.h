#ifndef	__BROKER__

#define	__BROKER__

#include "list.h"
#include "mosquitto.h"

#define MOSQ_LOG_NONE 0x00
#define MOSQ_LOG_INFO 0x01
#define MOSQ_LOG_NOTICE 0x02
#define MOSQ_LOG_WARNING 0x04
#define MOSQ_LOG_ERR 0x08
#define MOSQ_LOG_DEBUG 0x10
#define MOSQ_LOG_ALL 0xFFFF


#define	WAITE_TIMEOUT	30 //s


#define	PUBLIC_QOS		0 // >0 if not dead lock
#define	PUBLIC_RETAIN	0

#define	MQTT_KEEPLIVE	600

#define	REQ_RSP	1
#define	NOR_OPE	0

// src/dst/type/topic/seq/

#define	REQUEST_TOPIC		"%s/%s/ack/request/%d"
#define	RESPOND_TOPIC_ACK	"%s/%s/ack/respond/%s"

#define	RESPOND_TOPIC		"+/%s/ack/respond/+"
#define	RESPOND_QOS		0 // >0 if not dead lock
#define	RESPOND_RETAIN	0

// src/all/pub/event/
#define	PUB_TOPIC			"%s/all/pub/%s"
#define	TOPIC_ALL			"+/%s/#"
#define	TOPIC_TYPE			"+/%s/pub/%s"
#define	TOPIC_ACK			"+/%s/ack/#"


typedef int (*recv_cb)(char* buf, int buf_len, char *from, void* arg); 

typedef int (*exception_cb)(void* arg); 

typedef int (*log_print)(const char *format,...);

struct waite_ack_s{
	struct list_head next;
	pthread_mutex_t mutex;
	pthread_cond_t  cond;

	pthread_mutex_t *list_lock;

	int mid;
};

struct msg_priv{

	char *buf;
	int  buf_len;
	char from[256];
	pthread_t cb_pid;
	recv_cb recv_callback;
	void *arg; //指向全局数据结构
};


struct dev_priv{
	struct waite_ack_s waite;
		
	pthread_t cb_pid;
	char topic[256];

	int is_req_rsp;
	recv_cb recv_callback;

	char *buf;
	int  buf_len;
	char from[256];
	
	void *arg; //指向全局数据结构
};

struct g_broker_s{
	pthread_t mqtt_pid;
	int connect_stat;
	
	pthread_mutex_t big_mutex;
	//connect
	//struct list_head cont_list;
	struct waite_ack_s cont_waite_ack;

	//disconnect
	//struct list_head discont_list;
	struct waite_ack_s discont_waite_ack;

	//public
	struct list_head pub_list;
	pthread_mutex_t pub_list_mutex;
	//sub
	struct list_head sub_list;
	pthread_mutex_t sub_list_mutex;

	//unsub
	struct list_head unsub_list;
	pthread_mutex_t unsub_list_mutex;
	
	char id[MOSQ_MQTT_ID_MAX_LENGTH + 1]; 

	char host[16];
	int  host_port;

	struct mosquitto *mosq;
	
	struct list_head cb_list;
	pthread_mutex_t cb_list_mutex;
	
	struct list_head srv_list;
	pthread_mutex_t srv_list_mutex;

	char respond_topic[256];
	char src[256];
	unsigned char req_resp_seq;

	exception_cb excep_callback;

	log_print log_print_cb;
	
	void *usr_arg;
};

extern struct g_broker_s g_broker;

#define	broker_log(...)\
	do{\
		if(g_broker.log_print_cb){\
			g_broker.log_print_cb(__VA_ARGS__);\
		}\
	}while(0)

void dev_set_log(log_print log_func);
int init_broker(char *src, char *host, int port, exception_cb excep_cb, void *arg);
int close_broker(void);
int dev_send(char *buf, int buf_len, char *event);
int dev_transfer(char *buf, int buf_len, char **out_buf, int *out_buf_len, char *dst, int to_sec);
int register_callback(char *src, recv_cb cb, char *type);
int unregister_callback(char *src, char *type);
int dev_send_respond(char *buf, int buf_len, char *from);

static inline void get_timeout_val(struct timespec *to)
{
	struct timeval now;
	
	gettimeofday(&now, NULL);
	to->tv_nsec = 0;
	to->tv_sec = now.tv_sec + WAITE_TIMEOUT;
}

static inline int waite_ack_init(struct waite_ack_s *waite_ack, struct list_head *head, int mid)
{
	int ret = 0;
	ret = pthread_mutex_init(&(waite_ack->mutex), NULL);
	
	pthread_cond_init(&(waite_ack->cond), NULL);
	
	waite_ack->mid = mid;

	//test
	pthread_mutex_lock(&(waite_ack->mutex));
	pthread_mutex_unlock(&(waite_ack->mutex));
	//

	pthread_mutex_lock(waite_ack->list_lock);
	list_add_tail(&(waite_ack->next),head);
	pthread_mutex_unlock(waite_ack->list_lock);

exit:
	return 0;
}

static inline int waite_ack_timeout(struct waite_ack_s *waite_ack, int mutex)
{
	int ret = -EINVAL;
	
	struct timespec timeout;

	if(mutex){
		pthread_mutex_lock(&(waite_ack->mutex));
		if(waite_ack->mid){
			get_timeout_val(&timeout);
			ret = pthread_cond_timedwait(&(waite_ack->cond), &(waite_ack->mutex), &timeout);
		}else{
			ret = 0;
		}
		pthread_mutex_unlock(&(waite_ack->mutex));
	}
	
	pthread_mutex_lock(waite_ack->list_lock);
	list_del(&(waite_ack->next));
	pthread_mutex_unlock(waite_ack->list_lock);
	
	pthread_mutex_destroy(&(waite_ack->mutex));
	pthread_cond_destroy(&(waite_ack->cond));


	return ret;
}

static inline int wakeup_waite_ack(struct waite_ack_s *waite_ack, int mid)
{
	int ret = -ENODEV;
	
	if(waite_ack->mid == mid){
		pthread_mutex_lock(&(waite_ack->mutex));
		waite_ack->mid = 0;
		pthread_cond_signal(&(waite_ack->cond));
		pthread_mutex_unlock(&(waite_ack->mutex));

		ret = 0;
	}

	//pthread_mutex_lock(waite_ack->list_lock);
	//list_del(&(waite_ack->next));
	//pthread_mutex_unlock(waite_ack->list_lock);
	
	return ret;
}


#if 0
static inline int is_pub(char *topic)
{
	int ret = 1;
	int split = 0;
	char *pub = NULL;
	char str_pub[] = "pub";
	int i = 0;
	
	while (*topic != '\0'){
		if(*topic == '/'){
			//*topic = '\0';
			split++;
		}
		topic++;

		if( (!pub) &&(2 == split)){
			pub = topic;
		}

		if(pub && (split < 3)){
			if(topic[i] != str_pub[i]){
				return 0;
			}
			i++;
		}

		if(pub && (split >= 3)){
			break;
		}
	}

	return ret;
}
#else
static inline char* is_pub(char *topic)
{
	return strstr(topic, "/pub/");
}

static inline int is_me(char *topic, char *src)
{
	char *tmp = topic + strcspn(topic, "/");
	int ret = 0;
	int i = 0;

	if(*tmp) tmp++;
	
	for(i=0; *tmp && (*tmp != '/') && *src; i++, src++, tmp++){
		if(*tmp != *src){
			ret = 0;
			break;
		}
		ret = 1;
	}
	
	return ret;
}

#endif

static inline int __dev_send(struct g_broker_s *broker, char *buf, int buf_len, char *dst)
{
	int ret;
	struct timespec timeout;
	struct waite_ack_s waite_ack;

	waite_ack.list_lock = &(broker->pub_list_mutex);
	waite_ack_init(&waite_ack, &(broker->pub_list), 0);

	ret = mosquitto_publish(broker->mosq, &(waite_ack.mid), dst, buf_len, buf, PUBLIC_QOS, PUBLIC_RETAIN);

	ret = waite_ack_timeout(&waite_ack, !ret);

	printf("%s: ret(%d)\n",__func__, ret);


exit0:
	return ret;
}

#endif
