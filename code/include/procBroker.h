#ifndef PROCBROKER_H
#define PROCBROKER_H

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include "zlog.h"

#include "server.h"
#include "utility.h"

typedef struct g_broker_para{
	g_msg_queue_para*  g_msg_queue;
	g_server_para*     g_server;
	shareBufInfo*      shareInfo;
	int                rssi_state;
	zlog_category_t*   log_handler;
}g_broker_para;


#define	MAX_RSSI_NO	60000
struct rssi_priv{
	struct timeval timestamp;
	int rssi_buf_len;
	char rssi_buf[MAX_RSSI_NO]; 
}__attribute__((packed));


int initProcBroker(char *argv, g_broker_para** g_broker, g_server_para* g_server, zlog_category_t* handler);
int inquiry_state_from(char *buf, int buf_len, g_broker_para* g_broker);
int rssi_state_change(char *buf, int buf_len, g_broker_para* g_broker);
void close_rssi(g_broker_para* g_broker);
void freeProcBroker(g_broker_para* g_broker);

#endif//PROCBROKER_H
