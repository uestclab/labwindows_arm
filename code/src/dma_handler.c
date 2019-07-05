#include "dma_handler.h"
#include "utility.h"
#include "cst_net.h"
#include "zlog.h"

/* ========================================================================================== */
int IQ_register_callback(char* buf, int buf_len, void* arg)
{
	g_dma_para* g_dma = (g_dma_para*)arg;
	if(g_dma->slow_cnt < 50){
		//zlog_info(g_dma->log_handler,"IQ_register_callback !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		g_dma->slow_cnt = g_dma->slow_cnt + 1;
		return 0;
	}
	g_dma->slow_cnt = 0;

	int messageLen = buf_len + 4 + 4;
	int type = 0;
	if(g_dma->csi_state == 1 && g_dma->constellation_state == 0){
		type = 2;
	}else if(g_dma->constellation_state == 1 && g_dma->csi_state == 0){
		type = 4;
	}
	// htonl ?
	*((int32_t*)buf) = (buf_len + 4);
	*((int32_t*)(buf+ sizeof(int32_t))) = (type); // 1--rssi , 2--CSI , 3--json , 4--Constellation
	int ret = sendToPc(g_dma->g_server, buf, messageLen,type);
	return 0;
}

int init_dma_state(g_dma_para** g_dma, g_server_para* g_server, g_msg_queue_para*  g_msg_queue, zlog_category_t* handler){

	zlog_info(handler,"init_cst_state()");
	*g_dma = (g_dma_para*)malloc(sizeof(struct g_dma_para));
	(*g_dma)->g_server      	   = g_server;                                    // g_server->g_receive : sendToPc
	(*g_dma)->dma_state            = 0;                                           // dma_state
	(*g_dma)->csi_state            = 0;                                           // csi_state
	(*g_dma)->constellation_state  = 0;                                           // constellation_state
	(*g_dma)->g_msg_queue          = g_msg_queue;
	(*g_dma)->p_axidma             = NULL;
	(*g_dma)->log_handler          = handler;
	(*g_dma)->slow_cnt             = 0;

	zlog_info(handler,"init_cst_state open axidma -------------\n");
	(*g_dma)->p_axidma = axidma_open();
	if((*g_dma)->p_axidma == NULL){
		zlog_error(handler," In init_cst_state -------- axidma_open failed !! \n");
		return -1;
	}
	(*g_dma)->dma_state = 1;
	zlog_info(handler,"End init_cst_state open axidma() p_csi = %x, \n", (*g_dma)->p_axidma);
	return 0;

}

int dma_register_callback(g_dma_para* g_dma){
	int rc;
	rc = axidma_register_callback(g_dma->p_axidma, IQ_register_callback, (void*)(g_dma));
	if(rc != 0){
		zlog_error(g_dma->log_handler," axidma_register_callback failed\n");
		return -1;
	}
	return 0;
}


void gw_startcsi(g_dma_para* g_dma){
	int rc;
	if(g_dma->csi_state == 1){
		zlog_info(g_dma->log_handler,"axidma is already start\n");
		return;
	}
	if(g_dma->p_axidma == NULL){
		zlog_info(g_dma->log_handler,"g_dma->p_axidma == NULL gw_startcsi\n");
		g_dma->csi_state = 0;
		return;
	}else{
		rc = axidma_chan(g_dma->p_axidma, 0x40);
		rc = axidma_start(g_dma->p_axidma);
		g_dma->csi_state = 1;
		zlog_info(g_dma->log_handler,"rc = %d , gw_startcsi \n" , rc);
	}
}

void gw_stopcsi(g_dma_para* g_dma){
	int rc;
	if(g_dma->csi_state == 0){
		zlog_info(g_dma->log_handler,"csi is already stop\n");
		return;
	}
	if(g_dma->p_axidma == NULL){
		zlog_info(g_dma->log_handler,"g_dma->p_axidma == NULL gw_stopcsi\n");
		g_dma->csi_state = 0;
		return;
	}else{
		rc = axidma_stop(g_dma->p_axidma);
		g_dma->csi_state = 0;
		g_dma->constellation_state = 0;
		zlog_info(g_dma->log_handler,"rc = %d , gw_stopcsi \n" , rc);
	}
}

void gw_closedma(g_dma_para* g_dma){
	if(g_dma->p_axidma == NULL){
		zlog_info(g_dma->log_handler,"g_dma->p_axidma == NULL in gw_closedma\n");
		return;
	}
	zlog_info(g_dma->log_handler,"before axidma_close : %x \n" , g_dma->p_axidma);
	axidma_close(g_dma->p_axidma);
	g_dma->p_axidma = NULL;
	g_dma->dma_state = 0;
	zlog_info(g_dma->log_handler,"gw_closedma \n");
}

void gw_start_constellation(g_dma_para* g_dma){
	int rc;
	if(g_dma->constellation_state == 1){
		zlog_info(g_dma->log_handler,"constellation is already start\n");
		return;
	}
	if(g_dma->p_axidma == NULL){
		zlog_info(g_dma->log_handler,"g_dma->p_axidma == NULL gw_start_constellation\n");
		g_dma->constellation_state = 0;
		return;
	}else{
		rc = axidma_chan(g_dma->p_axidma, 0x80);
		rc = axidma_start(g_dma->p_axidma);
		g_dma->constellation_state = 1;
		zlog_info(g_dma->log_handler,"rc = %d , gw_start_constellation \n" , rc);
	}	
}

void gw_stop_constellation(g_dma_para* g_dma){
	int rc;
	if(g_dma->constellation_state == 0){
		zlog_info(g_dma->log_handler,"constellation is already stop\n");
		return;
	}
	if(g_dma->p_axidma == NULL){
		zlog_info(g_dma->log_handler,"g_dma->p_axidma == NULL gw_stop_constellation\n");
		g_dma->constellation_state = 0;
		return;
	}else{
		rc = axidma_stop(g_dma->p_axidma);
		g_dma->constellation_state = 0;
		g_dma->csi_state = 0;
		zlog_info(g_dma->log_handler,"rc = %d , gw_stop_constellation \n" , rc);
	}
}

















