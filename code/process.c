#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "process.h"
#include "zlog.h"
#include "broker.h"
#include "cJSON.h"
#include "utility.h"
#include "csiLoopMain.h"

#define BUFFER_SIZE 2560*4
#define SEND_HEADROOM 8
// temp


// --------------

int receive_running = 0;

int	 gMoreData_ = 0;
char* gReceBuffer_ = NULL;
char* gSendMessage = NULL;

para_thread* para_t = NULL;

int listenfd;

void initHandlePcProcess(){
	gReceBuffer_ = (char*)malloc(BUFFER_SIZE);
	gSendMessage = (char*)malloc(BUFFER_SIZE);

	para_t = newThreadPara();
}

void freeHandlePcProcess(){
	if(gReceBuffer_ != NULL)
	{
		free(gReceBuffer_);
		gReceBuffer_ = NULL;
	}
	if(gSendMessage != NULL)
	{
		free(gSendMessage);
		gSendMessage = NULL;
	}
	gMoreData_ = 0;
	receive_running = 0;
}


// prepare buf only for CSI 
int sendCSI(int connfd, void* csi_buf, int buf_len){

	// then call sendToPc
}

void sendCjson(int connfd, char* stat_buf, int stat_buf_len){
	int length = stat_buf_len + 4 + 4;
	char* temp_buf = malloc(length);
	//*((int32_t*)temp_buf) = htonl(stat_buf_len + sizeof(int32_t));
	//*((int32_t*)(temp_buf+ sizeof(int32_t))) = htonl(3); // 1--rssi , 2--CSI , 3--json
	*((int32_t*)temp_buf) = (stat_buf_len + sizeof(int32_t));
	*((int32_t*)(temp_buf+ sizeof(int32_t))) = (3); // 1--rssi , 2--CSI , 3--json
	memcpy(temp_buf + SEND_HEADROOM,stat_buf,stat_buf_len);
	//printf("send length = %d \n",length);
	int ret = sendToPc(connfd, temp_buf, length);
	free(temp_buf);
}

// ========= transfer json to broker and wait for response
void processMessage(const char* buf, int32_t length,int connfd){ // later use thread pool
	printf("processMessage()\n");
	int type = myNtohl(buf + 4);
	char* jsonfile = buf + sizeof(int32_t) + sizeof(int32_t);
	if(type == 5){
		printf("receive : %s\n",jsonfile);
		receive_running = 0;
		return;
	}
	char* stat_buf;
	int stat_buf_len;
	int ret;
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(jsonfile);
    item = cJSON_GetObjectItem(root,"dst");
	printf("dst = %s , type = %d \n",item->valuestring,type);
	
	cJSON_Delete(root);
	
	//ret = dev_transfer(jsonfile, strlen(jsonfile)+1, &stat_buf, &stat_buf_len, item->valuestring, -1); // block func
	if(ret == 0){
		sendCjson(connfd,jsonfile,length);
		//sendCjson(connfd,stat_buf,stat_buf_len+1);
	}
}

void receive(int connfd){ // receive -- | messageLength(4 Byte) | type(4 Byte) | json file(messageLength - 4) | 
    int n;
    int size = 0;
    int totalByte = 0;
    int messageLength = 0;
    char* temp_receBuffer = gReceBuffer_ + 2000; //
    char* pStart = NULL;
    char* pCopy = NULL;

    n = recv(connfd, temp_receBuffer, BUFFER_SIZE,0);
    if(n<=0){
		return;
    }
    size = n;
	
    pStart = temp_receBuffer - gMoreData_;
    totalByte = size + gMoreData_;
    const int MinHeaderLen = sizeof(int32_t);
	printf("totalByte = %d \n",totalByte); 
    while(1){
        if(totalByte <= MinHeaderLen)
        {
            gMoreData_ = totalByte;
            pCopy = pStart;
            if(gMoreData_ > 0)
            {
                memcpy(temp_receBuffer - gMoreData_, pCopy, gMoreData_);
            }
            break;
        }
        if(totalByte > MinHeaderLen)
        {
            messageLength= myNtohl(pStart);
			printf("messageLength = %d, totalByte = %d \n",messageLength,totalByte);
	
            if(totalByte < messageLength + MinHeaderLen )
            {
                gMoreData_ = totalByte;
                pCopy = pStart;
                if(gMoreData_ > 0){
                    memcpy(temp_receBuffer - gMoreData_, pCopy, gMoreData_);
                }
                break;
            } 
            else// at least one message 
            {
				processMessage(pStart,messageLength,connfd);
				// move to next message
                pStart = pStart + messageLength + MinHeaderLen;
                totalByte = totalByte - messageLength - MinHeaderLen;
                if(totalByte == 0){
                    gMoreData_ = 0;
                    break;
                }
            }          
        }
    }	
}

void *
receive_thread(void* args){
	printf("receive_thread()\n");
	int connfd = *((int*)args);
    while(receive_running == 1){
    	receive(connfd);
    }
	freeHandlePcProcess();
    printf("Exit receive_thread()\n");

}


/* ---------------------------  external interface  ------------------------------------- */

void receive_signal(){
	receive_running = 0;
	printf("end_receive_signal\n");
	exit(0);
}


pthread_t* initNet(int *fd){
	int connfd = -1;
    struct sockaddr_in servaddr;
 
    if( (listenfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
    {
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
 
    int one = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(55555);
 
    if( bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
    {
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
 
    if( listen(listenfd,10) == -1)
    {
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
 
    printf("========waiting for client's request========\n");

	while(1){
		if( (connfd = accept(listenfd,(struct sockaddr*)NULL,NULL)) == -1 ){
		    printf("accept socket error: %s(errno: %d)\n",strerror(errno),errno);
		}else{
			printf("accept new client , connfd = %d \n", connfd);

			initHandlePcProcess();
			startLoop();
			// new thread to handle this socket
			receive_running = 1;
			int ret = pthread_create(para_t->thread_pid, NULL, receive_thread, (void*)(fd));
			*fd = connfd;
			//return para_t->thread_pid;
		}
	}
	return NULL;
}

// send thread safe
/* ---------------------------------------------------------------- */
int sendToPc(int connfd, char* send_buf, int send_buf_len){
	pthread_mutex_lock(para_t->mutex_);
	int ret = send(connfd,send_buf,send_buf_len,0);
	pthread_mutex_unlock(para_t->mutex_);
	return ret;
}



/* ---------------------------------------------------------------- */



