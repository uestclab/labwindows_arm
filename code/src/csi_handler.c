#include "csi_handler.h"
#include "utility.h"
#include "cst_net.h"
#include "zlog.h"

/* ========================================================================================== */
int csi_callback(char* buf, int buf_len, void* arg)
{
	g_server_para* g_server = (g_server_para*)arg;

	if(g_server->csi_cnt != 0){
		g_server->csi_cnt = g_server->csi_cnt + 1;
		g_server->csi_cnt = g_server->csi_cnt % 100;
		return 0 ;
	}

	g_server->csi_cnt = g_server->csi_cnt + 1;
	g_server->csi_cnt = g_server->csi_cnt % 100;

	int messageLen = buf_len + 4 + 4;
	// htonl ?
	*((int32_t*)buf) = (buf_len + 4);
	*((int32_t*)(buf+ sizeof(int32_t))) = (2); // 1--rssi , 2--CSI , 3--json
	int ret = sendToPc(g_server, buf, messageLen,2);
	/*
	if(ret != messageLen){//postMsg MSG_CSI_SEND_ERROR
		struct msg_st data;
		data.msg_type = MSG_CSI_SEND_ERROR;
		data.msg_number = MSG_CSI_SEND_ERROR;
		data.msg_len = 0;
		postMsgQueue(&data,g_server->g_msg_queue);
	}
	*/
	return 0;
}

int init_cst_state(g_csi_para** g_csi, g_server_para* g_server, g_msg_queue_para*  g_msg_queue, zlog_category_t* handler){

	zlog_info(handler,"init_cst_state()");
	*g_csi = (g_csi_para*)malloc(sizeof(struct g_csi_para));
	(*g_csi)->g_server      	   = g_server;                                    // g_server->g_receive : sendToPc
	(*g_csi)->csi_state            = 0;                                           // csi_state
	(*g_csi)->g_msg_queue          = g_msg_queue;
	(*g_csi)->p_csi_axidma         = NULL;
	(*g_csi)->log_handler          = handler;

	zlog_info(handler,"init_cst_state open axidma -------------\n");
	(*g_csi)->p_csi_axidma = axidma_open();
	if((*g_csi)->p_csi_axidma == NULL){
		zlog_error(handler," In init_cst_state -------- axidma_open failed !! \n");
		return -1;
	}
	
	zlog_info(handler,"End init_cst_state open axidma() p_csi = %x, \n", (*g_csi)->p_csi_axidma);
	return 0;

}

int cst_register_callback(g_csi_para* g_csi){
	int rc;
	rc = axidma_register_callback(g_csi->p_csi_axidma, csi_callback, (void*)(g_csi->g_server));
	if(rc != 0){
		zlog_error(g_csi->log_handler," axidma_register_callback failed\n");
		return -1;
	}
	return 0;
}


void gw_startcsi(g_csi_para* g_csi){
	int rc;
	if(g_csi->csi_state == 1){
		zlog_info(g_csi->log_handler,"axidma is already start\n");
		return;
	}
	if(g_csi->p_csi_axidma == NULL){
		zlog_info(g_csi->log_handler,"g_csi->p_csi_axidma == NULL gw_startcsi\n");
		g_csi->csi_state = 0;
		return;
	}else{
		rc = axidma_start(g_csi->p_csi_axidma);
		g_csi->csi_state = 1;
		zlog_info(g_csi->log_handler,"rc = %d , gw_startcsi \n" , rc);
	}
}

void gw_stopcsi(g_csi_para* g_csi){
	int rc;
	if(g_csi->csi_state == 0){
		zlog_info(g_csi->log_handler,"axidma is already stop\n");
		return;
	}
	if(g_csi->p_csi_axidma == NULL){
		zlog_info(g_csi->log_handler,"g_csi->p_csi_axidma == NULL gw_stopcsi\n");
		g_csi->csi_state = 0;
		return;
	}else{
		rc = axidma_stop(g_csi->p_csi_axidma);
		g_csi->csi_state = 0;
		zlog_info(g_csi->log_handler,"rc = %d , gw_stopcsi \n" , rc);
	}
}

void gw_closecsi(g_csi_para* g_csi){
	if(g_csi->p_csi_axidma == NULL){
		zlog_info(g_csi->log_handler,"g_csi->p_csi_axidma == NULL in gw_closecsi\n");
		return;
	}
	zlog_info(g_csi->log_handler,"before axidma_close : %x \n" , g_csi->p_csi_axidma);
	axidma_close(g_csi->p_csi_axidma);
	g_csi->p_csi_axidma = NULL;
	zlog_info(g_csi->log_handler,"gw_closecsi \n");
}
















