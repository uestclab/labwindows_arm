#ifndef GW_MSG_QUEUE_H
#define GW_MSG_QUEUE_H
#include <stdio.h>  
#include <pthread.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <signal.h>   
#include <string.h>  
#include <errno.h>  
#include <sys/msg.h> 
#include <ctype.h>
#include "cJSON.h"
#include "zlog.h"

#include "utility.h" 

#define MAX_TEXT 2048

// msg
// post from server thread : runServer()
#define MSG_ACCEPT_NEW_CLIENT          10
// post from receiver thread : processMessage()
#define MSG_INQUIRY_STATE              11
#define MSG_RECEIVED_HEART_BEAT        12
#define MSG_RSSI_STATE_CHANGE          13
#define MSG_START_CSI                  14
#define MSG_STOP_CSI                   15
#define MSG_START_CONSTELLATION        16
#define MSG_STOP_CONSTELLATION         17

// post from receiver thread : exit thread
#define MSG_RECEIVE_THREAD_CLOSED      18

// post from timer thread call back
#define MSG_TIMEOUT                    20
#define MSG_STOP_TIMER                 21

// system state 
#define STATE_DISCONNECTED      0
#define STATE_STARTUP           1
#define STATE_CONNECTED         2

 
struct msg_st  
{  
    long int msg_type;
	int      msg_number;
	int      msg_len;  
    char     msg_json[MAX_TEXT];  
};


typedef struct g_msg_queue_para{
	int                msgid;
	para_thread*       para_t;
	int                seq_id;
	zlog_category_t*   log_handler;
}g_msg_queue_para;


g_msg_queue_para* createMsgQueue(zlog_category_t* handler);
int delMsgQueue(g_msg_queue_para* g_msg_queue);

void postMsgQueue(struct msg_st* data, g_msg_queue_para* g_msg_queue);
struct msg_st* getMsgQueue(g_msg_queue_para* g_msg_queue);


#endif//GW_MSG_QUEUE_H


