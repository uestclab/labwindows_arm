#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "procBroker.h"
#include "broker.h"
#include "server.h"
#include "cJSON.h"
#include "zlog.h"
//int sendToPc(g_server_para* g_server, char* send_buf, int send_buf_len, int device);
#define MAXSHAREBUF_SIZE 1024*60
#define HEADROOM 8

g_broker_para* g_broker_temp = NULL;

void sendStateInquiry(g_server_para* g_server, char* stat_buf, int stat_buf_len, int type){
	int length = stat_buf_len + 4 + 4;
	char* temp_buf = malloc(length);
	// htonl ?
	*((int32_t*)temp_buf) = (stat_buf_len + sizeof(int32_t));
	*((int32_t*)(temp_buf+ sizeof(int32_t))) = (type); // 1--rssi , 2--CSI , 3--json: 31 -- reg_json
	memcpy(temp_buf + HEADROOM,stat_buf,stat_buf_len);
	int ret = sendToPc(g_server, temp_buf, length, type);
	free(temp_buf);
}

void print_rssi_struct(g_broker_para* g_broker, char* buf, int buf_len){

	shareBufInfo* shareInfo = g_broker->shareInfo;


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
	
	int ret = sendToPc(g_broker->g_server, shareInfo->buf_, length, 1);
	//zlog_info(g_broker->log_handler,"print_rssi_struct :  , buf_len = %d \n",buf_len);
}

int process_exception(char* buf, int buf_len, char *from, void* arg)
{ 
	int ret = 0;
	if(g_broker_temp->g_server->g_receive->connfd == -1)
		return -1;
	if(strcmp(from,"mon/all/pub/system_stat") == 0){
		zlog_info(g_broker_temp->log_handler,"process_exception: mon/all/pub/system_stat");
		sendStateInquiry(g_broker_temp->g_server,buf,buf_len+1,41);
	}else if(strcmp(from,"rf/all/pub/rssi") == 0){ 
		print_rssi_struct(g_broker_temp,buf,buf_len); // send rssi struct stream to pc	
	}

	return ret;
}

int initProcBroker(char *argv, g_broker_para** g_broker, g_server_para* g_server, zlog_category_t* handler){

	zlog_info(handler,"initProcBroker()");
	*g_broker = (g_broker_para*)malloc(sizeof(struct g_broker_para));
	(*g_broker)->g_server      	   = g_server;                                    // connect_fd : g_broker->g_server->g_receive->connfd
    (*g_broker)->shareInfo         = (shareBufInfo*)malloc(sizeof(shareBufInfo)); // shareBufInfo* shareInfo  = NULL;
	(*g_broker)->shareInfo->buf_   = malloc(MAXSHAREBUF_SIZE);
	(*g_broker)->shareInfo->len_   = 0;
	(*g_broker)->rssi_state        = 0;                                           // rssi_stat
	(*g_broker)->g_msg_queue       = NULL;
	(*g_broker)->log_handler       = handler;

	int ret = init_broker(get_prog_name(argv), NULL, -1, NULL, NULL);
	zlog_info(handler,"get_prog_name(argv) = %s , ret = %d \n",get_prog_name(argv),ret);
	if( ret != 0)
		return -2;

	g_broker_temp = *g_broker;
	zlog_info(handler,"end initProcBroker()\n");
	return 0;
}

int broker_register_callback(g_broker_para* g_broker){
	int ret = register_callback("all", process_exception, "#");
	if(ret != 0){
		zlog_error(g_broker->log_handler,"register_callback error in initBroker\n");
		return -1;
	}
	return 0;
}

void freeProcBroker(g_broker_para* g_broker){
	close_broker();
	free(g_broker->shareInfo->buf_);
	free(g_broker->shareInfo);
	free(g_broker);
}

void printbuf_temp(char* buf,int buf_len,g_broker_para* g_broker){
	int len = strlen(buf) + 1;
	zlog_info(g_broker->log_handler,"buf_len = %d , len = %d \n",buf_len,len);
	zlog_info(g_broker->log_handler,"buf_json = %s\n",buf);
	zlog_info(g_broker->log_handler,"receive from reg ============================ \n");
}

int inquiry_state_from(char *buf, int buf_len, g_broker_para* g_broker){
	g_receive_para* g_receive = g_broker->g_server->g_receive;
	int ret = -1;
	char* stat_buf = NULL;
	int stat_buf_len = 0;
	cJSON * root = NULL;
    cJSON * item = NULL;
	cJSON * item_type = NULL;
    root = cJSON_Parse(buf);
    item = cJSON_GetObjectItem(root,"dst"); // different device is a dst
	//zlog_info(g_broker->log_handler,"dst = %s , \n",item->valuestring);
	ret = dev_transfer(buf, buf_len, &stat_buf, &stat_buf_len, item->valuestring, -1);

	// type is my own string used to distinguish different message request
	// GW_messageType.h define
	item_type = cJSON_GetObjectItem(root,"gw_type");
	//zlog_info(g_broker->log_handler,"item_type = %s , \n",item_type->valuestring);
	int type = 0;

	if(ret == 0 && stat_buf_len > 0 && g_receive != NULL){
		if(strcmp(item->valuestring,"mon") == 0){ // system monitor State
			sendStateInquiry(g_broker->g_server,stat_buf,stat_buf_len+1,41); 
		}else if(strcmp(item->valuestring,"gpio") == 0){ // gpio State
			sendStateInquiry(g_broker->g_server,stat_buf,stat_buf_len+1,51);
		}else if(strcmp(item->valuestring,"reg") == 0){ // reg state
			if(strcmp(item_type->valuestring,"fpga") == 0)
				type = 32;
			else
				type = 31;
			sendStateInquiry(g_broker->g_server,stat_buf,stat_buf_len+1,type);
		}
		//printbuf_temp(stat_buf,stat_buf_len,g_broker);
		free(stat_buf);
	}
	//zlog_info(g_broker->log_handler,"---------------------end inquiry_state_from---------\n");
	
	cJSON_Delete(root);
	return ret;
}

// ---------- rssi ----------------
int rssi_state_change(char *buf, int buf_len, g_broker_para* g_broker){
	zlog_info(g_broker->log_handler,"rssi json = %s \n",buf);
	int ret = -1;
	char* stat_buf = NULL;
	int stat_buf_len = 0;

	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(buf);
    item = cJSON_GetObjectItem(root,"dst"); // different device is a dst

	ret = dev_transfer(buf, buf_len, &stat_buf, &stat_buf_len, item->valuestring, -1);

	if(ret == 0 && stat_buf_len > 0 && g_broker->g_server->g_receive != NULL){
		item = cJSON_GetObjectItem(root,"timer");
		if(strcmp(item->valuestring,"0") == 0)
			g_broker->rssi_state = 0;
		else
			g_broker->rssi_state = 1;
		zlog_info(g_broker->log_handler,"rssi_state = %d \n, rssi return json = %s \n", g_broker->rssi_state , stat_buf);
		free(stat_buf);
	}
	cJSON_Delete(root);
}

void close_rssi(g_broker_para* g_broker){
	if(g_broker->rssi_state == 0)
		return;
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "dev", "/dev/i2c-0");
	cJSON_AddStringToObject(root, "addr", "0x4a");
	cJSON_AddStringToObject(root, "force", "0x1");
	cJSON_AddStringToObject(root, "type", "rssi");
	cJSON_AddStringToObject(root, "timer", "0");
	cJSON_AddStringToObject(root, "pub_no", "1");
	cJSON_AddStringToObject(root, "dst", "rf");
	cJSON *array=cJSON_CreateArray();
	cJSON_AddItemToObject(root,"op_cmd",array);

	cJSON *arrayobj=cJSON_CreateObject();
	cJSON_AddItemToArray(array,arrayobj);
	cJSON_AddStringToObject(arrayobj, "_comment","rssi");
	cJSON_AddStringToObject(arrayobj, "cmd","0");
	cJSON_AddStringToObject(arrayobj, "reg","0x0a");
	cJSON_AddStringToObject(arrayobj, "size","1");

	char* close_rssi_jsonfile = cJSON_Print(root);

	rssi_state_change(close_rssi_jsonfile,strlen(close_rssi_jsonfile),g_broker);

	cJSON_Delete(root);
	free(close_rssi_jsonfile);
}




