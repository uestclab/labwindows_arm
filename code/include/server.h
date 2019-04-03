#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "cJSON.h"
#include "zlog.h"
#include "msg_queue.h"
#include "utility.h"

typedef struct g_receive_para{
	g_msg_queue_para*  g_msg_queue;     
	int                receive_running;
	para_thread*       para_t;
	para_thread*       para_t_cancel;
	int                connfd;
	int                connected;
	int                disconnect_cnt;
	int                gMoreData_;
	char*              sendMessage;
	char*              recvbuf;
	zlog_category_t*   log_handler;
}g_receive_para;

typedef struct g_server_para{
	g_msg_queue_para*  g_msg_queue;
	g_receive_para*    g_receive;
	int                listenfd;            
	int                waiting;   // for state indicator
	int                hasTimer;
	para_thread*       para_t;
	zlog_category_t*   log_handler;
}g_server_para;


int InitServerThread(g_server_para** g_server, g_msg_queue_para* g_msg_queue, zlog_category_t* handler);
int InitReceThread(g_receive_para** g_receive, g_msg_queue_para* g_msg_queue, int sock_cli, zlog_category_t* handler);
int sendToPc(g_receive_para* g_receive, char* send_buf, int send_buf_len);
void stopReceThread(g_server_para* g_server);


#endif//SERVER_H
