#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include "zlog.h"

#include "server.h"
#include "msg_queue.h"
#include "countDownLatch.h"


#include "cJSON.h"




#define BUFFER_SIZE 2560*4


// --------------
void gw_set_socket_buffer(int sock, zlog_category_t* handler){
	int err = -1;
	int snd_size = 0;
	int rcv_size = 0;
	socklen_t optlen;

	optlen = sizeof(snd_size); 
	err = getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &snd_size, &optlen); 
	optlen = sizeof(rcv_size); 
	err = getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcv_size, &optlen); 
	zlog_info(handler," before gw_set_socket_buffer");
	zlog_info(handler," send buf size = : %d ",snd_size);
	zlog_info(handler," rcv buf size = : %d ",rcv_size);

	snd_size = 10*1024;
	optlen = sizeof(snd_size); 
	err = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &snd_size, optlen);

	rcv_size = 10*1024;
	optlen = sizeof(rcv_size); 
	err = setsockopt(sock,SOL_SOCKET,SO_RCVBUF, (char *)&rcv_size, optlen);

	snd_size = 0;
	rcv_size = 0;
	optlen = sizeof(snd_size); 
	err = getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &snd_size, &optlen); 
	optlen = sizeof(rcv_size); 
	err = getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcv_size, &optlen); 
	zlog_info(handler," after gw_set_socket_buffer");
	zlog_info(handler," send buf size = : %d ",snd_size);
	zlog_info(handler," rcv buf size = : %d ",rcv_size);
}


void gw_set_non_blocking_mode(int sock)
{
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

void gw_set_recv_timeout_mode(int sock){
	struct timeval timeout = {0,5}; 
	setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));
}

void postMsg(long int msg_type, char *buf, int buf_len, g_receive_para* g_receive){
	struct msg_st data;
	data.msg_type = msg_type;
	data.msg_number = msg_type;
	data.msg_len = buf_len;
	if(buf != NULL && buf_len != 0)
		memcpy(data.msg_json,buf,buf_len);
	postMsgQueue(&data,g_receive->g_msg_queue);
}


// ========= transfer json to broker and wait for response
int processMessage(const char* buf, int32_t length,g_receive_para* g_receive){
	int type = myNtohl(buf + 4);
	char* jsonfile = buf + sizeof(int32_t) + sizeof(int32_t);
	if(type == 4){ // open csi
		zlog_info(g_receive->log_handler,"receive : open csi request");
	}else if(type == 99){ // heart beat
		postMsg(MSG_RECEIVED_HEART_BEAT,NULL,0,g_receive);
	}else if(type == 5){ // close link button : stop
		postMsg(MSG_CLOSE_LINK_REQUEST,NULL,0,g_receive);
		zlog_info(g_receive->log_handler,"receive : %s\n",jsonfile);
	}else if(type == 7){ //gw_startcsi();
		postMsg(MSG_START_CSI,NULL,0,g_receive);
	}else if(type == 8){ //gw_stopcsi();
		postMsg(MSG_STOP_CSI,NULL,0,g_receive);
	}else if(type == 1){ // json
		postMsg(MSG_INQUIRY_STATE,jsonfile,length-4,g_receive);
		//inquiry_state_from(jsonfile,length-4);	
	}else if(type == 2){ // rssi json
		postMsg(MSG_RSSI_STATE_CHANGE,jsonfile,length-4,g_receive);
		//rssi_state_change(jsonfile,length-4);
	}
	return 0;
}

void receive(g_receive_para* g_receive){
    int n;
    int size = 0;
    int totalByte = 0;
    int messageLength = 0;
    char* temp_receBuffer = g_receive->recvbuf + 2000; //
    char* pStart = NULL;
    char* pCopy = NULL;

    n = recv(g_receive->connfd, temp_receBuffer, BUFFER_SIZE,0);
    if(n<=0){
		//if(n < 0)
		//	zlog_info(g_receive->log_handler,"recv() n < 0 , n = %d \n" , n);
		//if(n == 0)
		//	zlog_info(g_receive->log_handler,"recv() n = 0\n");
		return;
    }
	g_receive->rcv_cnt = g_receive->rcv_cnt + 1;
    size = n;
	
    pStart = temp_receBuffer - g_receive->gMoreData_;
    totalByte = size + g_receive->gMoreData_;
    const int MinHeaderLen = sizeof(int32_t);
    while(1){
        if(totalByte <= MinHeaderLen)
        {
            g_receive->gMoreData_ = totalByte;
            pCopy = pStart;
            if(g_receive->gMoreData_ > 0)
            {
                memcpy(temp_receBuffer - g_receive->gMoreData_, pCopy, g_receive->gMoreData_);
            }
            break;
        }
        if(totalByte > MinHeaderLen)
        {
            messageLength= myNtohl(pStart);
			//printf("messageLength = %d, totalByte = %d \n",messageLength,totalByte);
	
            if(totalByte < messageLength + MinHeaderLen )
            {
                g_receive->gMoreData_ = totalByte;
                pCopy = pStart;
                if(g_receive->gMoreData_ > 0){
                    memcpy(temp_receBuffer - g_receive->gMoreData_, pCopy, g_receive->gMoreData_);
                }
                break;
            } 
            else// at least one message 
            {
				int ret = processMessage(pStart,messageLength,g_receive);
				// move to next message
                pStart = pStart + messageLength + MinHeaderLen;
                totalByte = totalByte - messageLength - MinHeaderLen;
                if(totalByte == 0){
                    g_receive->gMoreData_ = 0;
                    break;
                }
            }          
        }
    }	
}

int getRunningState(g_receive_para* g_receive, int value){
	pthread_mutex_lock(g_receive->para_t_cancel->mutex_);
	if(value == 1){
		zlog_info(g_receive->log_handler,"g_receive->receive_running = 0");
		g_receive->receive_running = 0;
		pthread_mutex_unlock(g_receive->para_t_cancel->mutex_); // note that: pthread_mutex_unlock before return !!!!! 
		return 0;
	}
	int ret = g_receive->receive_running;
	//zlog_info(g_receive->log_handler,"getRunningState ret = %d\n",ret);
	pthread_mutex_unlock(g_receive->para_t_cancel->mutex_);
	return ret;
}


void* receive_thread(void* args){
	g_receive_para* g_receive = (g_receive_para*)args;

	zlog_info(g_receive->log_handler,"start receive_thread()\n");
    while(getRunningState(g_receive,0) == 1){
    	receive(g_receive);
    }
	postMsg(MSG_RECEIVE_THREAD_CLOSED,NULL,0,g_receive); // pose MSG_RECEIVE_THREAD_CLOSED
    zlog_info(g_receive->log_handler,"end Exit receive_thread()\n");
}

// send thread safe
int sendToPc(g_server_para* g_server, char* send_buf, int send_buf_len, int device){
	g_receive_para* g_receive = g_server->g_receive;
	if(g_server->waiting == STATE_DISCONNECTED){
		zlog_error(g_server->log_handler,"sendToPc , STATE_DISCONNECTED !!!!!!!!!!!! ------ device = %d ", device);
		return 0 ;
	}else if(g_receive == NULL){
		zlog_error(g_server->log_handler,"sendToPc , g_receive == NULL !!!!!!!!!!!! ------ device = %d ", device);
		return 0 ;
	}
	pthread_mutex_lock(g_receive->para_t->mutex_);
	int ret = send(g_receive->connfd,send_buf,send_buf_len,0);
	if(ret != send_buf_len){
		zlog_error(g_receive->log_handler,"Error in client send socket: send length = %d , expected length = %d", ret , send_buf_len);
	}
	g_receive->send_cnt = g_receive->send_cnt + 1;
	pthread_mutex_unlock(g_receive->para_t->mutex_);
	return ret;
}

void* runServer(void* args){
	g_server_para* g_server = (g_server_para*)args;

    struct sockaddr_in servaddr; 
    if( (g_server->listenfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
    {
        zlog_error(g_server->log_handler,"create socket error: %s(errno: %d)\n",strerror(errno),errno);
		g_server->waiting = STATE_DISCONNECTED;
    }
 
    int one = 1;
    setsockopt(g_server->listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(55555);
 
    if( bind(g_server->listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
    {
        zlog_error(g_server->log_handler,"bind socket error: %s(errno: %d)\n",strerror(errno),errno);
		g_server->waiting = STATE_DISCONNECTED;
    }
 
    if( listen(g_server->listenfd,10) == -1)
    {
        zlog_error(g_server->log_handler,"listen socket error: %s(errno: %d)\n",strerror(errno),errno);
		g_server->waiting = STATE_DISCONNECTED;
    }

	if(g_server->waiting == STATE_DISCONNECTED){
		zlog_error(g_server->log_handler,"server thread failure\n");
		return NULL;
	}
	g_server->waiting = STATE_STARTUP;
    zlog_info(g_server->log_handler,"========waiting for client's request========\n");
	while(1){
		int connfd = -1;
		if( (connfd = accept(g_server->listenfd,(struct sockaddr*)NULL,NULL)) == -1 ){
		    zlog_error(g_server->log_handler,"accept socket error: %s(errno: %d)\n",strerror(errno),errno);
		}else{
			zlog_info(g_server->log_handler," -------------------accept new client , connfd = %d \n", connfd);
			g_receive_para* g_receive = NULL;
			int ret = InitReceThread(&g_receive, g_server->g_msg_queue, connfd, g_server->log_handler);
			g_server->g_receive = g_receive;
			g_server->waiting = STATE_CONNECTED;
			postMsg(MSG_ACCEPT_NEW_CLIENT,NULL,0,g_receive);
		}
		zlog_info(g_server->log_handler,"========waiting for client's request========\n");
	}
}

int InitServerThread(g_server_para** g_server, g_msg_queue_para* g_msg_queue, g_cntDown_para* g_cntDown, zlog_category_t* handler){
	zlog_info(handler,"InitServerThread()");
	*g_server = (g_server_para*)malloc(sizeof(struct g_server_para));
	(*g_server)->waiting      = STATE_STARTUP;
    (*g_server)->listenfd     = 0;
	(*g_server)->hasTimer     = 0;
	(*g_server)->g_cntDown    = g_cntDown;
	(*g_server)->enableCallback    = 0;
	(*g_server)->csi_cnt      = 0;
	(*g_server)->para_t       = newThreadPara();
	(*g_server)->g_msg_queue  = g_msg_queue;
	(*g_server)->g_receive    = NULL;
	(*g_server)->log_handler  = handler;
	int ret = pthread_create((*g_server)->para_t->thread_pid, NULL, runServer, (void*)(*g_server));
    if(ret != 0){
        zlog_error(handler,"create InitServerThread error ! error_code = %d", ret);
		return -1;
    }	
	return 0;
}


int InitReceThread(g_receive_para** g_receive, g_msg_queue_para* g_msg_queue, int connfd, zlog_category_t* handler){
	zlog_info(handler,"InitReceThread()");
	*g_receive = (g_receive_para*)malloc(sizeof(struct g_receive_para));
	(*g_receive)->g_msg_queue     = g_msg_queue;
    (*g_receive)->receive_running = 1;                          // receive_running
	(*g_receive)->para_t          = newThreadPara();
	(*g_receive)->para_t_cancel   = newThreadPara();
	(*g_receive)->connfd          = connfd;                     // connfd
	(*g_receive)->connected    	  = 0;                          // for heart beat check;
	(*g_receive)->disconnect_cnt  = 0;
	(*g_receive)->gMoreData_ 	  = 0;                          // gMoreData_
	(*g_receive)->sendMessage 	  = (char*)malloc(BUFFER_SIZE); // gSendMessage
	(*g_receive)->recvbuf 		  = (char*)malloc(BUFFER_SIZE); // gReceBuffer_
	(*g_receive)->log_handler 	  = handler;
	(*g_receive)->rcv_cnt         = 0;
	(*g_receive)->send_cnt        = 0;

	gw_set_recv_timeout_mode(connfd);

	//gw_set_socket_buffer(connfd,handler);

	int ret = pthread_create((*g_receive)->para_t->thread_pid, NULL, receive_thread, (void*)(*g_receive));
    if(ret != 0){
        zlog_error(handler,"create InitReceThread error ! error_code = %d", ret);
		return -1;
    }	
	return 0;
}

void freeReceThread(g_server_para* g_server){
	g_server->waiting = STATE_DISCONNECTED;
	g_receive_para* g_receive = g_server->g_receive;
	
	//pthread_cancel(*(g_receive->para_t->thread_pid));
	getRunningState(g_receive,1);
	zlog_info(g_receive->log_handler,"before pthread_join . \n");
	if ( pthread_join(*(g_receive->para_t->thread_pid), NULL) ){
		zlog_info(g_receive->log_handler,"error join thread. \n");
	}
	zlog_info(g_receive->log_handler,"receive thread is exit after join thread. \n");
	if(g_receive->recvbuf != NULL)
	{
		free(g_receive->recvbuf);
		g_receive->recvbuf = NULL;
	}
	if(g_receive->sendMessage != NULL)
	{
		free(g_receive->sendMessage);
		g_receive->sendMessage = NULL;
	}
	g_receive->gMoreData_ = 0;
	destoryThreadPara(g_receive->para_t);
	destoryThreadPara(g_receive->para_t_cancel);
	close(g_receive->connfd);
	g_receive->connfd = -1;
	zlog_info(g_receive->log_handler,"freeReceThread() \n");
	g_receive->connected = 0;
	g_receive->disconnect_cnt = 0;
	free(g_receive);
	g_server->g_receive = NULL;
}

void stopReceThread(g_server_para* g_server){ // only main thread call through event 
	zlog_info(g_server->log_handler,"enter stopReceThread() \n");
	freeReceThread(g_server);
	zlog_info(g_server->log_handler,"stopReceThread() \n");
}
















