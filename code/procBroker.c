#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "procBroker.h"
#include "broker.h"
#include "utility.h"
#include "process.h"
#include "cJSON.h"
#include "zlog.h"

zlog_category_t * broker_log_handler = NULL;

shareBufInfo* shareInfo  = NULL;

int *connect_fd = NULL;

#define MAXSHAREBUF_SIZE 1024*60
#define HEADROOM 8

/* 

typedef struct shareBufInfo{
    char*       buf_;
    int32_t 	len_;
}shareBufInfo;

struct timeval  
{  
__time_t tv_sec;        //Seconds.  
__suseconds_t tv_usec;  //Microseconds.  
}
*/

#define	MAX_RSSI_NO	60000
struct rssi_priv{
	struct timeval timestamp;
	int rssi_buf_len;
	char rssi_buf[MAX_RSSI_NO]; 
}__attribute__((packed));


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

void print_rssi_struct(int connfd, char* buf, int buf_len){
	struct rssi_priv* tmp_buf = (struct rssi_priv*)buf;
	//printf("timestamp tv_sec = %d , tv_usec = %d \n",tmp_buf->timestamp.tv_sec,tmp_buf->timestamp.tv_usec);
	//printf("rssi_buf_len = %d \n",tmp_buf->rssi_buf_len);

	int msg_len = 4 + 4 + 4 + tmp_buf->rssi_buf_len;
	int length = msg_len + 4 + 4;
	*((int32_t*)shareInfo->buf_) = (msg_len + sizeof(int32_t));
	*((int32_t*)(shareInfo->buf_+ sizeof(int32_t) )) = 1; // 1--rssi
	*((int32_t*)(shareInfo->buf_+ sizeof(int32_t) * 2)) = tmp_buf->timestamp.tv_sec;
	*((int32_t*)(shareInfo->buf_+ sizeof(int32_t) * 3)) = tmp_buf->timestamp.tv_usec;
	*((int32_t*)(shareInfo->buf_+ sizeof(int32_t) * 4)) = tmp_buf->rssi_buf_len;
	memcpy(shareInfo->buf_ + sizeof(int32_t) * 5, tmp_buf->rssi_buf, tmp_buf->rssi_buf_len);	
	
	int ret = sendToPc(connfd, shareInfo->buf_, length);
}

int process_exception(char* buf, int buf_len, char *from, void* arg)
{
	int ret = 0;
	if((*connect_fd) == -1)
		return -1;
	if(strcmp(from,"mon/all/pub/system_stat") == 0){
		sendStateInquiry(*connect_fd,buf,buf_len+1,41);
	}else if(strcmp(from,"rf/all/pub/rssi") == 0){ 
		print_rssi_struct(*connect_fd,buf,buf_len); // send rssi struct stream to pc	
	}

	return ret;
}


int initProcBroker(char *argv,int* fd,zlog_category_t* log_handler){
	broker_log_handler = log_handler;
	int ret = init_broker(get_prog_name(argv), NULL, -1, NULL, NULL);
	zlog_info(broker_log_handler,"get_prog_name(argv) = %s , ret = %d \n",get_prog_name(argv),ret);
	if( ret != 0)
		return -2;

	ret = register_callback("all", process_exception, "#");
	if(ret != 0){
		zlog_error(broker_log_handler,"register_callback error in initBroker\n");
		return -1;
	}


	shareInfo = (shareBufInfo*)malloc(sizeof(shareBufInfo));
	shareInfo->buf_ = malloc(MAXSHAREBUF_SIZE);
	shareInfo->len_ = 0;
	connect_fd = fd;
	zlog_info(broker_log_handler,"end initProcBroker()\n");
	return 0;
}

void printbuf_temp(char* buf,int buf_len){
	int len = strlen(buf) + 1;
	zlog_info(broker_log_handler,"buf_len = %d , len = %d \n",buf_len,len);
	zlog_info(broker_log_handler,"buf_json = %s\n",buf);
	zlog_info(broker_log_handler,"receive from reg ============================ \n");
}

int inquiry_state_from(char *buf, int buf_len){
	int ret = -1;
	char* stat_buf = NULL;
	int stat_buf_len = 0;
	cJSON * root = NULL;
    cJSON * item = NULL;
	cJSON * item_type = NULL;
    root = cJSON_Parse(buf);
    item = cJSON_GetObjectItem(root,"dst"); // different device is a dst
	//zlog_info(broker_log_handler,"dst = %s , \n",item->valuestring);
	ret = dev_transfer(buf, buf_len, &stat_buf, &stat_buf_len, item->valuestring, -1);

	// type is my own string used to distinguish different message request
	// GW_messageType.h define
	item_type = cJSON_GetObjectItem(root,"gw_type");
	//zlog_info(broker_log_handler,"item_type = %s , \n",item_type->valuestring);
	int type = 0;

	if(ret == 0 && stat_buf_len > 0 && connect_fd != NULL){
		if(strcmp(item->valuestring,"mon") == 0){ // system monitor State
			sendStateInquiry(*connect_fd,stat_buf,stat_buf_len+1,41); 
		}else if(strcmp(item->valuestring,"gpio") == 0){ // gpio State
			sendStateInquiry(*connect_fd,stat_buf,stat_buf_len+1,51);
		}else if(strcmp(item->valuestring,"reg") == 0){ // reg state
			if(strcmp(item_type->valuestring,"fpga") == 0)
				type = 32;
			else
				type = 31;
			sendStateInquiry(*connect_fd,stat_buf,stat_buf_len+1,type);
		}
		//printbuf_temp(stat_buf,stat_buf_len);
		free(stat_buf);
	}
	//zlog_info(broker_log_handler,"---------------------end inquiry_state_from---------\n");
	
	cJSON_Delete(root);
	return ret;
}

// ---------- rssi ----------------
int rssi_state_change(char *buf, int buf_len){
	//printf("rssi json = %s \n",buf);
	int ret = -1;
	char* stat_buf = NULL;
	int stat_buf_len = 0;

	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(buf);
    item = cJSON_GetObjectItem(root,"dst"); // different device is a dst

	ret = dev_transfer(buf, buf_len, &stat_buf, &stat_buf_len, item->valuestring, -1);

	if(ret == 0 && stat_buf_len > 0 && connect_fd != NULL){
		zlog_info(broker_log_handler,"rssi return json = %s \n", stat_buf);
		free(stat_buf);
	}
	cJSON_Delete(root);
}



void destoryProcBroker(){
	close_broker();
	free(shareInfo->buf_);
	free(shareInfo);
}



