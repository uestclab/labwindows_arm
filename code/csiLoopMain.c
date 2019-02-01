#include "csiLoopMain.h"
#include "utility.h"
#include "process.h" // sendToPc()
#include "cst_net.h"


void *dev_lq = NULL;

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

void *dev_next = NULL;

void* initCstNet(){
	printf("initCstNet-------------\n");
	dev_lq = axidma_open();
	if(dev_lq == NULL)
		printf("dev_lq == NULL axidma_open\n");
	int rc = -99;
	rc = axidma_register_callback(dev_lq, csi_callback, NULL);
	printf("rc = %d , initCstNet\n", rc);
	startcsi(dev_lq);
	return dev_lq;
}

void startcsi(void* pd){
	printf("startcsi\n");
	int rc = -99;
	if(pd == NULL)
		printf("pd == NULL startcsi\n");
	else{
		dev_next = pd;
		rc = axidma_start(pd);
	}
	printf("rc = %d , startcsi \n" , rc);
}

void stopcsi(){
	printf("stopcsi\n");
	if(dev_next == NULL)
		printf("dev_next == NULL");
	if(dev_lq == NULL)
		printf("dev_lq == NULL");
	int rc;
	rc = axidma_stop(dev_next);
	axidma_close(dev_next);
	
}


/* ========================================================================================== */




















