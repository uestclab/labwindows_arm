#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<unistd.h>

#include "procBroker.h"
#include "broker.h"
#include "utility.h"
#include "process.h"


//para_thread* rssi_t = NULL;
int resume = 0;
shareBufInfo* shareInfo  = NULL;

int *connect_fd = NULL;

#define MAXSHAREBUF_SIZE 1024*10
#define FRAME_HEADROOM 8

void sendRssi(){
	int length = shareInfo->len_ + sizeof(int32_t) + sizeof(int32_t);
	*((int32_t*)(shareInfo->buf_)) = htonl(shareInfo->len_ + sizeof(int32_t));
	*((int32_t*)(shareInfo->buf_+ sizeof(int32_t))) = htonl(1); // 1--rssi , 2--CSI , 3--json
	
	int ret = sendToPc(*connect_fd, shareInfo->buf_, length);
	//resume = 0;
}

// callback from other thread ,can be block by myself , copy buffer to allocate memory then join threadPool
int receive_rssi(char* buf, int buf_len, char *from, void* arg){ 
	memcpy(shareInfo->buf_ + FRAME_HEADROOM,buf,buf_len);
	shareInfo->len_ = buf_len;
	sendRssi();
	//resume = 1;
	//pthread_cond_signal(rssi_t->cond_);
}

/*
void * sendRssi_thread(void* args){
	pthread_mutex_lock(rssi_t->mutex_);
    while(1){
		while (resume == 0)
		{
			pthread_cond_wait(rssi_t->cond_, rssi_t->mutex_);
		}
		pthread_mutex_unlock(rssi_t->mutex_);
    	sendRssi();
    }
}
*/

int initProcBroker(char *argv,int* fd){

	int ret = init_broker(get_prog_name(argv), NULL, -1, NULL, NULL);
	printf("get_prog_name(argv) = %s , ret = %d \n",get_prog_name(argv),ret);
	
	ret = register_callback("all", NULL, "event"); //	
	ret = register_callback("all", receive_rssi, "rssi");

	//rssi_t = newThreadPara();

	//ret = pthread_create(rssi_t->thread_pid, NULL, sendRssi_thread, NULL);

	shareInfo = (shareBufInfo*)malloc(sizeof(shareBufInfo));
	shareInfo->buf_ = malloc(MAXSHAREBUF_SIZE);
	connect_fd = fd;
	
	return 0;
	
	//return rssi_t->thread_pid;
}

void destoryProcBroker(){
	free(shareInfo->buf_);
	free(shareInfo);
}






