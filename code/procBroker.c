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
#include "cJSON.h"

shareBufInfo* shareInfo  = NULL;

int *connect_fd = NULL;

#define MAXSHAREBUF_SIZE 1024*10
#define HEADROOM 8

void sendStateInquiry(int connfd, char* stat_buf, int stat_buf_len, int type){
	int length = stat_buf_len + 4 + 4;
	char* temp_buf = malloc(length);
	// htonl ?
	*((int32_t*)temp_buf) = (stat_buf_len + sizeof(int32_t));
	*((int32_t*)(temp_buf+ sizeof(int32_t))) = (type); // 1--rssi , 2--CSI , 3--json: 31 -- reg_json
	memcpy(temp_buf + HEADROOM,stat_buf,stat_buf_len);
	int ret = sendToPc(connfd, temp_buf, length);
	free(temp_buf);
}


int process_exception(char* buf, int buf_len, char *from, void* arg)
{
	int ret = 0;
	if(strcmp(from,"mon/all/pub/system_stat") == 0){
		sendStateInquiry(*connect_fd,buf,buf_len+1,41);
	}



	printf("from : %s\n",from);
	printf("buf = %s , buf_len = %d \n",buf, buf_len);
	return ret;
}


int initProcBroker(char *argv,int* fd){

	int ret = init_broker(get_prog_name(argv), NULL, -1, NULL, NULL);
	printf("get_prog_name(argv) = %s , ret = %d \n",get_prog_name(argv),ret);
	
	//register_callback("all",,"pub");

	ret = register_callback("all", process_exception, "#");
	if(ret != 0)
		printf("register_callback error in initBroker\n");


	shareInfo = (shareBufInfo*)malloc(sizeof(shareBufInfo));
	shareInfo->buf_ = malloc(MAXSHAREBUF_SIZE);
	shareInfo->len_ = 0;
	connect_fd = fd;
	
	return 0;
}

void printbuf_temp(char* buf,int buf_len){
	int len = strlen(buf) + 1;
	printf("buf_len = %d , len = %d \n",buf_len,len);
	printf("buf_json = %s\n",buf);
	printf("receive from reg ============================ \n");
}

int inquiry_state_from(char *buf, int buf_len){
	int ret = -1;
	char* stat_buf = NULL;
	int stat_buf_len = 0;
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(buf);
    item = cJSON_GetObjectItem(root,"dst");
	printf("dst = %s , \n",item->valuestring);
	ret = dev_transfer(buf, buf_len, &stat_buf, &stat_buf_len, item->valuestring, -1);

	if(ret == 0 && stat_buf_len > 0 && connect_fd != NULL){
		if(strcmp(item->valuestring,"mon") == 0){
			sendStateInquiry(*connect_fd,stat_buf,stat_buf_len+1,41); // system State
		}else if(strcmp(item->valuestring,"gpio") == 0){
			sendStateInquiry(*connect_fd,stat_buf,stat_buf_len+1,51); // gpio State
		}else{
			sendStateInquiry(*connect_fd,stat_buf,stat_buf_len+1,31); // reg state
			printbuf_temp(stat_buf,stat_buf_len);
		}
	}
	printf("------------------------------\n");
	cJSON_Delete(root);
	free(stat_buf);
	return ret;
}


void destoryProcBroker(){
	close_broker();
	free(shareInfo->buf_);
	free(shareInfo);
}



