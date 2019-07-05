#ifndef DMA_HANDLER_H
#define DMA_HANDLER_H

#include "cst_net.h"
#include "zlog.h"
#include "server.h" // sendToPc()

typedef struct g_dma_para{
	g_msg_queue_para*  g_msg_queue;
	g_server_para*     g_server;
	uint32_t           slow_cnt;            		   // for send cnt to slow datas
	int                dma_state;
	int                csi_state;
	int                constellation_state;
	void *             p_axidma;
	zlog_category_t*   log_handler;
}g_dma_para;

int init_dma_state(g_dma_para** g_dma, g_server_para* g_server, g_msg_queue_para*  g_msg_queue, zlog_category_t* handler);
int dma_register_callback(g_dma_para* g_dma);
void gw_startcsi(g_dma_para* g_dma);
void gw_stopcsi(g_dma_para* g_dma);

void gw_start_constellation(g_dma_para* g_dma);
void gw_stop_constellation(g_dma_para* g_dma);

void gw_closedma(g_dma_para* g_dma);





#endif//DMA_HANDLER_H
