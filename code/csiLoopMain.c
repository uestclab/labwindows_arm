#include "csiLoopMain.h"
#include "utility.h"
#include "process.h" // sendToPc()

#define SEND_SIZE 2560*4
#define HEADROOM 8

para_thread* send_t = NULL;
char* gCsiMessage = NULL;
int Loop = 0;
int stop_t = 0;
//extern int gLinkfd;
// test
const char* file_path = NULL;
char* file_buf = NULL;
int length = 0;

void test_init(){
	file_path = "/home/gyl/liqingSpace/code/labwindows/labwindows_arm/code/cst_upload_to_arm.dat";
	file_buf = readfile(file_path);
	length = strlen(file_buf);
	printf("length = %d\n",length);
}


void test(int fd){
	char rssi[2560] = {'a'};
	int send_num = 100003;
	int error = 0;
	int counter = 0;
	
	int messageLen = length + 4 + 4;
	
	while(send_num--){
		if(send_num == 3500 || send_num == 1900 || send_num == 8888){
			*((int32_t*)gCsiMessage) = (2560 + 4);
			*((int32_t*)(gCsiMessage+ sizeof(int32_t))) = (1); // 1--rssi , 2--CSI , 3--json
			memcpy(gCsiMessage+HEADROOM,rssi,2560);
			messageLen = 2560 + 4 + 4;
			int ret = sendToPc(fd, gCsiMessage, messageLen);
			printf("send rssi \n");
		}else{//file_buf
			send_csi(file_buf,length);
		}
		//delay();
	}
	printf("send csi end \n");
	stop_t = 1;
}

void startLoop(){
	Loop = 1;
	pthread_cond_signal(send_t->cond_);
}


void* csiLoopThread(void* args){
	int* connfd = (int*)args;
	printf("csiLoopThread : connfd = %d \n",*connfd);
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


/* ---------------------------  external interface  ------------------------------------- */
int csiLoopMain(int *fd){
	test_init();
	send_t = newThreadPara();
	gCsiMessage = (char*)malloc(SEND_SIZE);
	int ret = pthread_create(send_t->thread_pid, NULL, csiLoopThread, (void*)fd);
	return 0;
}




/* ========================================================================================== */




void send_csi(char* buf, int buf_len){

	int messageLen = buf_len + 4 + 4;
	*((int32_t*)gCsiMessage) = (buf_len + 4);
	*((int32_t*)(gCsiMessage+ sizeof(int32_t))) = (2); // 1--rssi , 2--CSI , 3--json
	memcpy(gCsiMessage+HEADROOM,buf,buf_len);

	int ret = sendToPc(-1, gCsiMessage, messageLen);

}


int recv_callback(char* buf, int buf_len, void* arg) // step 4 : copy and transfer to my own buffer
{
	send_csi(buf,buf_len);
	return 0;
}

















