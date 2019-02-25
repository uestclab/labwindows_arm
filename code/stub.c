#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "utility.h"
#include "process.h" // sendToPc()
#include "stub.h"

#define SEND_SIZE 2560*4
#define HEADROOM 8

// =================================================================
FILE* fp = NULL;
const char* file_path = NULL;
para_thread* send_t = NULL;
char* gCsiMessage = NULL;
int Loop = 0;
int stop_t = 0;


// ==================================================================
void send_csi(char* buf, int buf_len){

	int messageLen = buf_len + 4 + 4;
	// htonl ?
	*((int32_t*)gCsiMessage) = (buf_len + 4);
	*((int32_t*)(gCsiMessage+ sizeof(int32_t))) = (2); // 1--rssi , 2--CSI , 3--json
	memcpy(gCsiMessage+HEADROOM,buf,buf_len);

	int ret = sendToPc(-1, gCsiMessage, messageLen);
}


void test(int fd){
	int send_num = 10000000;
	printf("start test \n");
	while(send_num--){
		char get[1024];
		size_t num = fread(get,sizeof(char),1024,fp);
		if(num == 0){
			printf("send_num = %d \n" , send_num);
			break;		
		}else if(num != 1024){
			printf("num != 1024 \n");		
		}
		send_csi(get,num);
	}
	printf("send csi end \n");
	stop_t = 1;
}



void test_init(){
	file_path = "/home/gyl/liqingSpace/code/labwindows/labwindows_arm/code/test/test_parse.dat";
	if((fp=fopen(file_path,"rb"))==NULL)
	{
		printf("无法打开文件");
		exit(0);
	}
}


void* stubLoopThread(void* args){
	int* connfd = (int*)args;
	printf("stubLoopThread : connfd = %d \n",*connfd);
	pthread_mutex_lock(send_t->mutex_);
    while(stop_t == 0){
		while (Loop == 0 )
		{
			pthread_cond_wait(send_t->cond_, send_t->mutex_);
		}
		pthread_mutex_unlock(send_t->mutex_);
		int fd = *connfd;
		printf("test : connfd = %d \n",fd);
    	test(fd);
    }
	printf("end csi Loop \n");
}



int stubMain(int *fd){
	test_init();
	send_t = newThreadPara();
	gCsiMessage = (char*)malloc(SEND_SIZE);
	int ret = pthread_create(send_t->thread_pid, NULL, stubLoopThread, (void*)fd);
	return 0;
}

void startLoop(){
	Loop = 1;
	pthread_cond_signal(send_t->cond_);
}

void stopcsi(){
	printf("stopcsi()\n");
}

void* initCstNet(){
	printf("initCstNet()\n");
}

int inquiry_state_from(char *buf, int buf_len){
	int get_len = strlen(buf) + 1;
	printf("get_len = %d , buf_len = %d \n", get_len, buf_len);
	printf("json buf = %s \n", buf);
	printf("=====================================\n");

}

int initProcBroker(char *argv,int* fd){
	printf("initProcBroker()\n");
}


























