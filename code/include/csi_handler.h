#ifndef CSI_HANDLER_H
#define CSI_HANDLER_H

#include "cst_net.h"
#include "zlog.h"
#include "server.h" // sendToPc()

typedef struct g_csi_para{
	g_msg_queue_para*  g_msg_queue;
	g_server_para*     g_server;
	int                csi_state;
	void *             p_csi_axidma;
	zlog_category_t*   log_handler;
}g_csi_para;

int init_cst_state(g_csi_para** g_csi, g_server_para* g_server, g_msg_queue_para*  g_msg_queue, zlog_category_t* handler);
void gw_startcsi(g_csi_para* g_csi);
void gw_stopcsi(g_csi_para* g_csi);
void gw_closecsi(g_csi_para* g_csi);
// --------------------------------------------------------
/*
void* initCstNet();
void init_cst_state(zlog_category_t* log_handler);

void gw_startcsi();
void gw_stopcsi();
void gw_closecsi();
*/






#endif//CSI_HANDLER_H
