#include "csiLoopMain.h"
#include "utility.h"
#include "process.h" // sendToPc()
#include "cst_net.h"
#include "zlog.h"

zlog_category_t *csi_log_handler = NULL;

void *dev_lq = NULL;
int useSwitch = 0;

/* ---------------------------  external interface  ------------------------------------- */

void send_csi(char* buf, int buf_len){

	int messageLen = buf_len + 4 + 4;
	// htonl ?
	*((int32_t*)buf) = (buf_len + 4);
	*((int32_t*)(buf+ sizeof(int32_t))) = (2); // 1--rssi , 2--CSI , 3--json
	int ret = sendToPc(-1, buf, messageLen);

}


int csi_callback(char* buf, int buf_len, void* arg)
{
	send_csi(buf,buf_len);
	return 0;
}

void init_cst_state(zlog_category_t* log_handler){
	csi_log_handler = log_handler;
	zlog_info(csi_log_handler,"init_cst_state()\n");
	useSwitch = 0;
	dev_lq = NULL;
}

void* initCstNet(){
	zlog_info(csi_log_handler,"initCstNet open axidma -------------\n");
	if(dev_lq != NULL){
		zlog_info(csi_log_handler,"return dev_lq = %x -------------\n" , dev_lq);
		return dev_lq;
	}
	else{
		dev_lq = axidma_open();
		zlog_info(csi_log_handler,"axidma_open() dev_lq = %x -------------\n" , dev_lq);
	}
	if(dev_lq == NULL){
		zlog_error(csi_log_handler,"dev_lq == NULL axidma_open\n");
		return NULL;
	}
	int rc;
	rc = axidma_register_callback(dev_lq, csi_callback, NULL);
	return dev_lq;
}

void gw_startcsi(){
	int rc;
	if(useSwitch == 1){
		zlog_info(csi_log_handler,"axidma is already start\n");
		return;
	}
	if(dev_lq == NULL){
		zlog_info(csi_log_handler,"dev_lq == NULL gw_startcsi\n");
		return;
	}else{
		rc = axidma_start(dev_lq);
		useSwitch = 1;
		zlog_info(csi_log_handler,"rc = %d , gw_startcsi \n" , rc);
	}
}

void gw_stopcsi(){
	zlog_info(csi_log_handler,"enter gw_stopcsi() , useSwitch = %d \n",useSwitch);
	int rc;
	if(useSwitch == 0){
		zlog_info(csi_log_handler,"axidma is already stop\n");
		return;
	}
	if(dev_lq == NULL){
		zlog_info(csi_log_handler,"dev_lq == NULL gw_stopcsi\n");
		return;
	}else{
		rc = axidma_stop(dev_lq);
		useSwitch = 0;
		zlog_info(csi_log_handler,"rc = %d , gw_stopcsi \n" , rc);
	}
}

void gw_closecsi(){ // close error ! ---- 20190323
	zlog_info(csi_log_handler,"enter gw_closecsi \n");
	if(dev_lq == NULL){
		zlog_info(csi_log_handler,"dev_lq == NULL in gw_closecsi\n");
		return;
	}
	zlog_info(csi_log_handler,"before axidma_close : %x \n" , dev_lq);
	axidma_close(dev_lq);
	dev_lq = NULL;
	zlog_info(csi_log_handler,"gw_closecsi \n");
}


/* ========================================================================================== */




















