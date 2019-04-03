#include "msg_queue.h"

g_msg_queue_para* createMsgQueue(zlog_category_t* handler){
	zlog_info(handler,"Enter createMsgQueue\n");
	g_msg_queue_para* g_msg_queue = (g_msg_queue_para*)malloc(sizeof(struct g_msg_queue_para));
	g_msg_queue->log_handler = handler;
	g_msg_queue->msgid = -1;
	g_msg_queue->seq_id = 1;

  	g_msg_queue->msgid = msgget((key_t)1556, 0666 | IPC_CREAT);  
	if(g_msg_queue->msgid == -1){
		zlog_error(handler,"msgget failed with error: %d\n", errno); 
		free(g_msg_queue);
		return NULL;  
	}
	g_msg_queue->para_t = newThreadPara();

	return g_msg_queue; 
}

void postMsgQueue(struct msg_st* data, g_msg_queue_para* g_msg_queue){

	data->msg_number = g_msg_queue->seq_id;
	g_msg_queue->seq_id = g_msg_queue->seq_id + 1;

	pthread_mutex_lock(g_msg_queue->para_t->mutex_);
	if(msgsnd(g_msg_queue->msgid, (void*)data, MAX_TEXT, 0) == -1){  
		zlog_info(g_msg_queue->log_handler,"postMsgQueue : msgsnd failed\n"); 
	}
	pthread_mutex_unlock(g_msg_queue->para_t->mutex_);
}

struct msg_st* getMsgQueue(g_msg_queue_para* g_msg_queue){
	struct msg_st* out_data = (struct msg_st*)malloc(sizeof(struct msg_st));
	long int msgtype = 0;

	if(msgrcv(g_msg_queue->msgid, (void*)out_data, MAX_TEXT, msgtype, 0) == -1){  
		zlog_info(g_msg_queue->log_handler, "msgrcv failed with errno: %d\n", errno);
		free(out_data);
		return NULL;   
	}
	return out_data;
}

int delMsgQueue(g_msg_queue_para* g_msg_queue)
{
	destoryThreadPara(g_msg_queue->para_t);
	// g_msg_queue->msgid?
	zlog_info(g_msg_queue->log_handler,"delMsgQueue()");
	free(g_msg_queue);
	return 0;
}








 
