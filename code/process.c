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

#define BUFFER_SIZE 2560*4
#define SEND_HEADROOM 8
// temp

char* file_buf = NULL;

// --------------

int	 gMoreData_ = 0;
char* gReceBuffer_ = NULL;
char* gSendMessage = NULL;

para_thread* para_t = NULL;

int listenfd;

void initHandlePcProcess(){
	file_buf = "server send";
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
}


// prepare buf only for CSI 
int sendCSI(int connfd, void* csi_buf, int buf_len){
	// add frame header and type(int or cjson) in csi_buf headroom 

	// then call sendToPc
}

void sendCjson(int connfd, char* stat_buf, int stat_buf_len){
	int length = stat_buf_len + 4 + 4;
	char* temp_buf = malloc(length);
	*((int32_t*)temp_buf) = htonl(stat_buf_len + sizeof(int32_t));
	*((int32_t*)(temp_buf+ sizeof(int32_t))) = htonl(3); // 1--rssi , 2--CSI , 3--json
	memcpy(temp_buf + SEND_HEADROOM,stat_buf,stat_buf_len);
	int ret = sendToPc(connfd, temp_buf, length);
	free(temp_buf);
}

void processMessage(const char* buf, int32_t length,int connfd){ // later use thread pool
	// transfer json to broker and wait for response
	printf("processMessage()\n");
	char* jsonfile = buf + sizeof(int32_t);
	printf("receive : %s\n",jsonfile);
	char* stat_buf;
	int stat_buf_len;
	int ret;
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(jsonfile);
    item = cJSON_GetObjectItem(root,"dst");
	printf("dst = %s\n",item->valuestring);
	
	sendCjson(connfd,jsonfile,length);
	cJSON_Delete(root);
	return;
	ret = dev_transfer(jsonfile, strlen(jsonfile)+1, &stat_buf, &stat_buf_len, item->valuestring, -1); // block func
	if(ret == 0){
		// call thread safe func sendToPc
		sendCjson(connfd,stat_buf,stat_buf_len+1);
	}
}

void receive(int connfd){ // receive -- | messageLength(4 Byte) | json file(messageLength) | 
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
	printf("receive : %s\n",temp_receBuffer);
    size = n;
	
    pStart = temp_receBuffer - gMoreData_;
    totalByte = size + gMoreData_;
    printf("receive : totalByte = %d , size = %d \n", totalByte,size);
    const int MinHeaderLen = sizeof(int32_t);

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
    while(1){
    	receive(connfd);
    }
	close(connfd);
    printf("Exit receive_thread()");

}



/* ---------------------------  external interface  ------------------------------------- */

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

    if( (connfd = accept(listenfd,(struct sockaddr*)NULL,NULL)) == -1 ){
        printf("accept socket error: %s(errno: %d)\n",strerror(errno),errno);
    }else{
		printf("accept new client\n");
		//
		initHandlePcProcess();
		// new thread to handle this socket
		int ret = pthread_create(para_t->thread_pid, NULL, receive_thread, (void*)(fd));
		*fd = connfd;
		return para_t->thread_pid;
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
void* test_send(void* args){
	int length = strlen(file_buf);
	printf("length = %d\n",length);

	int send_num = 20000;
	int error = 0;
	int counter = 0;
	printf("press...");
	
	
	while(send_num--){
		//*((int32_t*)gSendMessage) = htonl(length);
		int messageLen = length + 4;
		memcpy(gSendMessage,&messageLen,4);
		memcpy(gSendMessage+4,&send_num,4);
		memcpy(gSendMessage+8,file_buf,length);
		// send
		counter = counter + 1;
		if(counter == 5){
			counter = 0;		
			//user_wait();
		}
		delay();
	}
	printf("test_send() end \n");	
}





