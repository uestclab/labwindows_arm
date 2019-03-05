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
#include "cJSON.h"
#include "utility.h"

#ifdef USE_STUB
	#include "stub.h"
#endif

#ifndef USE_STUB
	#include "csiLoopMain.h"
	#include "procBroker.h"
#endif

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
int gLinkfd = 0;

void initHandlePcProcess(){
	gReceBuffer_ = (char*)malloc(BUFFER_SIZE);
	gSendMessage = (char*)malloc(BUFFER_SIZE);
	if(para_t == NULL)
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

void sendCjson(int connfd, char* stat_buf, int stat_buf_len){
	int length = stat_buf_len + 4 + 4;
	char* temp_buf = malloc(length);
	// htonl ?
	*((int32_t*)temp_buf) = (stat_buf_len + sizeof(int32_t));
	*((int32_t*)(temp_buf+ sizeof(int32_t))) = (3); // 1--rssi , 2--CSI , 3--json
	memcpy(temp_buf + SEND_HEADROOM,stat_buf,stat_buf_len);
	int ret = sendToPc(connfd, temp_buf, length);
	free(temp_buf);
}

// ========= transfer json to broker and wait for response
void processMessage(const char* buf, int32_t length,int connfd){ // later use thread pool
	int type = myNtohl(buf + 4);
	char* jsonfile = buf + sizeof(int32_t) + sizeof(int32_t);
	if(type == 4){
		initCstNet();
	}else if(type == 5){
		stopcsi();
		close_csi();
		printf("receive : %s\n",jsonfile);
		receive_running = 0;
		return;
	}else if(type == 7){
		startcsi();
		return;
	}else if(type == 8){
		stopcsi();
		return;
	}else if(type == 1){ // json
		inquiry_state_from(jsonfile,length-4);	
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
	//printf("totalByte = %d \n",totalByte); 
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
			//printf("messageLength = %d, totalByte = %d \n",messageLength,totalByte);
	
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

void receive_signal(){ //  exit program if SIGINT
	receive_running = 0;
	printf("end_receive_signal\n");
	exit(0);
}


int initNet(int *fd){
	int connfd = -1;
    struct sockaddr_in servaddr;
 
    if( (listenfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
    {
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        return -1;
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
        return -1;
    }
 
    if( listen(listenfd,10) == -1)
    {
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        return -1;
    }
 
    printf("========waiting for client's request========\n");

	while(1){
		if( (connfd = accept(listenfd,(struct sockaddr*)NULL,NULL)) == -1 ){
		    printf("accept socket error: %s(errno: %d)\n",strerror(errno),errno);
		}else{
			printf("accept new client , connfd = %d \n", connfd);
			*fd = connfd;
			gLinkfd = connfd;
			
			initHandlePcProcess();		
			receive_running = 1;
			int ret = pthread_create(para_t->thread_pid, NULL, receive_thread, (void*)(fd));
#ifdef USE_STUB
			startLoop(); // stub test
#endif
		}
	}
	return 0;
}

// send thread safe
/* ---------------------------------------------------------------- */
int sendToPc(int connfd, char* send_buf, int send_buf_len){
	pthread_mutex_lock(para_t->mutex_);
	int ret = send(gLinkfd,send_buf,send_buf_len,0);
	pthread_mutex_unlock(para_t->mutex_);
	return ret;
}



/* ---------------------------------------------------------------- */



