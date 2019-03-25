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
#include "zlog.h"


#ifndef USE_STUB
	#include "csiLoopMain.h"
	#include "procBroker.h"
	#include "exception.h"
#endif

#define BUFFER_SIZE 2560*4
#define SEND_HEADROOM 8
// temp

zlog_category_t *temp_log_handler = NULL;

// --------------

int receive_running = 0;

int	 gMoreData_ = 0;
char* gReceBuffer_ = NULL;
char* gSendMessage = NULL;

para_thread* para_t = NULL;

int listenfd;
int gLinkfd = 0;

void initHandleProcess(){
	gReceBuffer_ = (char*)malloc(BUFFER_SIZE);
	gSendMessage = (char*)malloc(BUFFER_SIZE);
	if(para_t == NULL)
		para_t = newThreadPara();
}

void freeHandleProcess(){
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
	close(gLinkfd);
	zlog_info(temp_log_handler,"freeHandleProcess() \n");
}


// ========= transfer json to broker and wait for response
int processMessage(const char* buf, int32_t length,int connfd){ // later use thread pool
	int type = myNtohl(buf + 4);
	char* jsonfile = buf + sizeof(int32_t) + sizeof(int32_t);
	if(type == 4){
		initCstNet(temp_log_handler);
	}else if(type == 18){
		zlog_info(temp_log_handler,"receive end link \n");
		receive_running = 0;
		return 1;
	}else if(type == 99){ // heart beat
		//zlog_info(temp_log_handler," ---- heart beat \n");
		check_variable(1,1);
	}else if(type == 5){
		stopcsi();
		closecsi();
		zlog_info(temp_log_handler,"receive : %s\n",jsonfile);
	}else if(type == 7){
		startcsi();
	}else if(type == 8){
		stopcsi();
	}else if(type == 1){ // json
		inquiry_state_from(jsonfile,length-4);	
	}else if(type == 2){ // rssi json
		rssi_state_change(jsonfile,length-4);
	}
	return 0;
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
		if(n < 0)
			zlog_info(temp_log_handler,"recv() n <0 , n = %d \n" , n);
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
				int ret = processMessage(pStart,messageLength,connfd);
				if(ret == 1)
					break;
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
	zlog_info(temp_log_handler,"start receive_thread()\n");
	int connfd = *((int*)args);
    while(receive_running == 1){
    	receive(connfd);
    }
	freeHandleProcess();
    zlog_info(temp_log_handler,"end Exit receive_thread()\n");

}


/* ---------------------------  external interface  ------------------------------------- */

void receive_signal(){ //  exit program if SIGINT
	zlog_info(temp_log_handler,"receive_signal -- SIGINT\n");
}


int initNet(int *fd,zlog_category_t* log_handler){
	temp_log_handler = log_handler;
	int connfd = -1;
    struct sockaddr_in servaddr;
 
    if( (listenfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
    {
        zlog_error(temp_log_handler,"create socket error: %s(errno: %d)\n",strerror(errno),errno);
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
        zlog_error(temp_log_handler,"bind socket error: %s(errno: %d)\n",strerror(errno),errno);
        return -1;
    }
 
    if( listen(listenfd,10) == -1)
    {
        zlog_error(temp_log_handler,"listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        return -1;
    }
 
    zlog_info(temp_log_handler,"========waiting for client's request========\n");

	while(1){
		if( (connfd = accept(listenfd,(struct sockaddr*)NULL,NULL)) == -1 ){
		    zlog_error(temp_log_handler,"accept socket error: %s(errno: %d)\n",strerror(errno),errno);
		}else{
			zlog_info(temp_log_handler," -------------------accept new client , connfd = %d \n", connfd);
			*fd = connfd;
			gLinkfd = connfd;		
			initHandleProcess(); // init memory and thread variable		
			receive_running = 1;
			int ret = pthread_create(para_t->thread_pid, NULL, receive_thread, (void*)(fd));
			// call timer
			StartTimer();	
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

void stopReceThread(){
    pthread_cancel(*para_t->thread_pid);
    pthread_join(*para_t->thread_pid, NULL); //wait the thread stopped
	freeHandleProcess();
	// stop and close csi
	zlog_info(temp_log_handler,"stopcsi() \n");
	stopcsi();
	zlog_info(temp_log_handler,"stopReceThread : close_csi() \n");
	closecsi();
	// close rssi
	zlog_info(temp_log_handler,"close_rssi() \n"); 
	close_rssi();
	zlog_info(temp_log_handler,"stopReceThread() \n");
}

