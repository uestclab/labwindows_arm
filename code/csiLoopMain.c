#include "csiLoopMain.h"
#include "utility.h"
#include "process.h" // sendToPc()
#include "cst_net.h"


void *dev_lq = NULL;
static int useSwitch = 0;

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

void* initCstNet(){
	printf("initCstNet open axidma -------------\n");
	if(dev_lq != NULL)
		return dev_lq;
	else
		dev_lq = axidma_open();
	if(dev_lq == NULL){
		printf("dev_lq == NULL axidma_open\n");
		return NULL;
	}
	int rc;
	rc = axidma_register_callback(dev_lq, csi_callback, NULL);
	return dev_lq;
}

void startcsi(){
	int rc;
	if(useSwitch == 1){
		printf("axidma is already start\n");
		return;
	}
	if(dev_lq == NULL){
		printf("dev_lq == NULL startcsi\n");
		return;
	}else{
		rc = axidma_start(dev_lq);
		useSwitch = 1;
		printf("rc = %d , startcsi \n" , rc);
	}
}

void stopcsi(){
	int rc;
	if(useSwitch == 0){
		printf("axidma is already stop\n");
		return;
	}
	if(dev_lq == NULL){
		printf("dev_lq == NULL stopcsi\n");
		return;
	}else{
		rc = axidma_stop(dev_lq);
		useSwitch = 0;
		printf("rc = %d , stopcsi \n" , rc);
	}
}

void close_csi(){
	if(dev_lq == NULL){
		printf("dev_lq == NULL in close_csi\n");
		return;
	}
	axidma_close(dev_lq);
	dev_lq = NULL;
}


/* ========================================================================================== */




















