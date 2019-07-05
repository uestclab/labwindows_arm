#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include "cJSON.h"
#include "zlog.h"
#include "msg_queue.h"
#include "utility.h"
#include "countDownLatch.h"
#include <sys/ioctl.h>
#include <linux/sockios.h>

// system state 
//#define STATE_DISCONNECTED      0
//#define STATE_STARTUP           1
//#define STATE_CONNECTED         2

typedef struct g_receive_para{
	g_msg_queue_para*  g_msg_queue;     
	int                receive_running;
	para_thread*       para_t;
	para_thread*       para_t_cancel;
	int                connfd;
	int                connected; // heat beat use
	int                disconnect_cnt;
	int                gMoreData_;
	char*              sendMessage;
	char*              recvbuf;
	zlog_category_t*   log_handler;
	uint32_t           rcv_cnt;  // debug counter
	uint32_t           send_cnt; // debug counter
	uint32_t           comp_send_cnt;
	uint32_t           point_1_send_cnt;
	uint32_t           point_2_send_cnt;
}g_receive_para;

typedef struct g_server_para{
	g_msg_queue_para*  g_msg_queue;
	g_receive_para*    g_receive;
	int                listenfd;            
	int                waiting;            // for state indicator
	int                hasTimer;           // whether start timer
	g_cntDown_para*    g_cntDown;
	int                enableCallback;     // whether register cb to csi and procBroker
	int                send_enable;        // if network send error in sendToPc , set disable 
	para_thread*       para_t;
	para_thread*       para_waiting_t;
	zlog_category_t*   log_handler;
}g_server_para;


int InitServerThread(g_server_para** g_server, g_msg_queue_para* g_msg_queue, g_cntDown_para* g_cntDown, zlog_category_t* handler);
int InitReceThread(g_receive_para** g_receive, g_msg_queue_para* g_msg_queue, int sock_cli, zlog_category_t* handler);
/* 
	1. main thread -> inquiry_state_from -> sendStateInquiry -> 
	2. broker thread -> process_exception -> sendStateInquiry ->
	3. broker thread -> process_exception -> print_rssi_struct ->
	4. csi thread -> csi_callback -> 
*/
int sendToPc(g_server_para* g_server, char* send_buf, int send_buf_len, int device);
void stopReceThread(g_server_para* g_server);


#endif//SERVER_H
