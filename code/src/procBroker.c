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


typedef int (*log_print)(const char *format,...);
void dev_set_log(log_print log_func);

int my_zlog(const char *format,...){
 	;
}

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
	if(g_broker_temp->g_server->waiting == STATE_DISCONNECTED)
		return -1;
	if(strcmp(from,"mon/all/pub/system_stat") == 0){
		zlog_info(g_broker_temp->log_handler,"process_exception: mon/all/pub/system_stat , buf = %s \n", buf);
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
	
	dev_set_log(printf);

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
	zlog_info(g_broker->log_handler,"start dev_transfer......");
	ret = dev_transfer(buf, buf_len, &stat_buf, &stat_buf_len, item->valuestring, -1);
	zlog_info(g_broker->log_handler,"after dev_transfer...... ret = %d \n", ret);
	// type is my own string used to distinguish different message request
	// GW_messageType.h define
	item_type = cJSON_GetObjectItem(root,"gw_type");

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
		free(stat_buf);
	}
	
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

// ---------- rf and mf ----------------

void send_rf_mf_State(g_server_para* g_server, char* stat_buf, int stat_buf_len){
	int length = stat_buf_len + 4 + 4;
	char* temp_buf = malloc(length);
	// htonl ?
	*((int32_t*)temp_buf) = (stat_buf_len + sizeof(int32_t));
	*((int32_t*)(temp_buf+ sizeof(int32_t))) = (5); // 1--rssi , 2--CSI , 3--json , 5--mf and rf state my own json
	memcpy(temp_buf + HEADROOM,stat_buf,stat_buf_len);
	int ret = sendToPc(g_server, temp_buf, length, 5);
	free(temp_buf);
}

// size : 0--no data,1--, 2--
int i2cset(g_broker_para* g_broker, const char* dev, const char* addr, const char* reg, int size, const char* data){
	int     ret = -1;
	char*   jsonfile = NULL;
	char*   stat_buf = NULL;
	int     stat_buf_len = 0;
	cJSON * root = NULL; 
	cJSON * item = NULL;
	cJSON * array = NULL;
	cJSON * arrayobj = NULL;
	
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "dev", dev); // "/dev/i2c-0" , "/dev/i2c-1"

	cJSON_AddStringToObject(root, "addr", addr);
	cJSON_AddStringToObject(root, "force", "0x1");
	cJSON_AddStringToObject(root, "dst", "rf");
	array=cJSON_CreateArray();
	cJSON_AddItemToObject(root,"op_cmd",array);

	arrayobj=cJSON_CreateObject();
	cJSON_AddItemToArray(array,arrayobj);
	cJSON_AddStringToObject(arrayobj, "_comment","R0");
	cJSON_AddStringToObject(arrayobj, "cmd","1"); // i2cset
	cJSON_AddStringToObject(arrayobj, "reg",reg);
	if(size == 0)
		cJSON_AddStringToObject(arrayobj, "size","0");
	else if(size == 1){
		cJSON_AddStringToObject(arrayobj, "size","1");
		cJSON_AddStringToObject(arrayobj, "data",data); // "0x00"
	}

	jsonfile = cJSON_Print(root);
	item = cJSON_GetObjectItem(root,"dst");
	ret = dev_transfer(jsonfile,strlen(jsonfile), &stat_buf, &stat_buf_len, item->valuestring, -1);

	cJSON_Delete(root);
	root = NULL;
	free(jsonfile);

	if(ret == 0 && stat_buf_len > 0){

		root = cJSON_Parse(stat_buf);
		item = cJSON_GetObjectItem(root,"stat");

		if(strcmp(item->valuestring,"0") != 0){
			zlog_info(g_broker->log_handler,"i2cset ----- buf_json = %s\n",stat_buf);
		}

		cJSON_Delete(root);
		root = NULL;
		free(stat_buf);
	}
	
	return 0;
}

char* i2cget(g_broker_para* g_broker, const char* dev, const char* addr, const char* reg, int size){
	int     ret = -1;
	char*   jsonfile = NULL;
	char*   stat_buf = NULL;
	int     stat_buf_len = 0;
	cJSON * root = NULL; 
	cJSON * item = NULL;
	cJSON * array = NULL;
	cJSON * arrayobj = NULL;
	char *  ret_return = NULL;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "dev", dev);
	cJSON_AddStringToObject(root, "addr", addr);
	cJSON_AddStringToObject(root, "force", "0x1");
	cJSON_AddStringToObject(root, "dst", "rf");
	array=cJSON_CreateArray();
	cJSON_AddItemToObject(root,"op_cmd",array);

	arrayobj=cJSON_CreateObject();
	cJSON_AddItemToArray(array,arrayobj);
	cJSON_AddStringToObject(arrayobj, "_comment","R0");
	cJSON_AddStringToObject(arrayobj, "cmd","0"); // i2cget
	cJSON_AddStringToObject(arrayobj, "reg",reg);
	if(size == 1)
		cJSON_AddStringToObject(arrayobj, "size","1");
	else if(size == 2)
		cJSON_AddStringToObject(arrayobj, "size","2");

	jsonfile = cJSON_Print(root);
	item = cJSON_GetObjectItem(root,"dst");
	ret = dev_transfer(jsonfile,strlen(jsonfile), &stat_buf, &stat_buf_len, item->valuestring, -1);

	cJSON_Delete(root);
	root = NULL;
	free(jsonfile);

	if(ret == 0 && stat_buf_len > 0){

		cJSON * arry_return = NULL;

		root = cJSON_Parse(stat_buf);
		item = cJSON_GetObjectItem(root,"stat");
		if(strcmp(item->valuestring,"0") != 0){
			zlog_info(g_broker->log_handler,"i2cget ----- buf_json = %s\n",stat_buf);
			cJSON_Delete(root);
			root = NULL;
			free(stat_buf);
			return NULL;
		}

		arry_return = cJSON_GetObjectItem(root,"return");
		if(NULL != arry_return){
			cJSON* temp_list = arry_return->child;
			while(temp_list != NULL){
				char * ret_tmp   = cJSON_GetObjectItem( temp_list , "ret")->valuestring;
				temp_list = temp_list->next;
				ret_return = malloc(32);
				memcpy(ret_return,ret_tmp,strlen(ret_tmp)+1);
			}
		}
		cJSON_Delete(root);
		root = NULL;
		free(stat_buf);
	}
	
	return ret_return;
}


char* ad7417_temperature(g_broker_para* g_broker){

	char* p_ret = NULL;
	
    //i2cset -y -f 1 0x28 0x00
	i2cset(g_broker, "/dev/i2c-1", "0x28", "0x00", 0, NULL);

	//i2cset -y -f 1 0x28 0x01 0x00
	i2cset(g_broker, "/dev/i2c-1", "0x28", "0x01", 1, "0x00");

	//i2cget -y -f 1 0x28 0x00
	p_ret = i2cget(g_broker, "/dev/i2c-1", "0x28", "0x00", 2);

	return p_ret;
}

ret_byte* rf_current(g_broker_para* g_broker){
	ret_byte* p_ret = (ret_byte*)malloc(sizeof(ret_byte));
	
    //i2cset -y -f 1 0x48 0x19 0x14
	i2cset(g_broker, "/dev/i2c-1", "0x48", "0x19", 1, "0x14");

	//i2cget -y -f 1 0x48 0x0a
	p_ret->high = i2cget(g_broker, "/dev/i2c-1", "0x48", "0x0a", 1);

	//i2cget -y -f 1 0x48 0x04
	p_ret->low  = i2cget(g_broker, "/dev/i2c-1", "0x48", "0x04", 1);

	return p_ret;

}



int inquiry_rf_and_mf(g_broker_para* g_broker){
	//
	cJSON *root = cJSON_CreateObject();

	char* temperature_ad7417 = ad7417_temperature(g_broker);
	cJSON_AddStringToObject(root, "ad7417_temper", temperature_ad7417);	
	free(temperature_ad7417);

	ret_byte* rf_cur = rf_current(g_broker);
	cJSON_AddStringToObject(root, "rf_cur_low", rf_cur->low);
	cJSON_AddStringToObject(root, "rf_cur_high", rf_cur->high);
	free(rf_cur->low);
	free(rf_cur->high);
	free(rf_cur);


	char* send_jsonfile = cJSON_Print(root);

	send_rf_mf_State(g_broker->g_server, send_jsonfile, strlen(send_jsonfile)+1);

	cJSON_Delete(root);
	free(send_jsonfile);
	return 0;
}





