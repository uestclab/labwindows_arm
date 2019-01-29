#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include "utility.h"

para_thread* newThreadPara(){

	para_thread* para_t = (para_thread*)malloc(sizeof(para_thread));
	para_t->thread_pid = (pthread_t*)malloc(sizeof(pthread_t));
	para_t->mutex_ = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(para_t->mutex_,NULL);
	para_t->cond_ = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
	pthread_cond_init(para_t->cond_,NULL);	
	return para_t;
}

void destoryThreadPara(para_thread* para_t){
	pthread_cond_destroy(para_t->cond_);
    pthread_mutex_destroy(para_t->mutex_);
    free(para_t->cond_);
    free(para_t->mutex_);
    free(para_t->thread_pid);
}



int32_t myNtohl(const char* buf){
	int32_t be32 = 0;
	memcpy(&be32, buf, sizeof(be32));
	//return ntohl(be32);
	return be32;
}

int filelength(FILE *fp)
{
	int num;
	fseek(fp,0,SEEK_END);
	num=ftell(fp);
	fseek(fp,0,SEEK_SET);
	return num;
}

char* readfile(const char *path)
{
	FILE *fp;
	int length;
	char *ch;
	if((fp=fopen(path,"r"))==NULL)
	{
		return NULL;
	}
	length=filelength(fp);
	ch=(char *)malloc(length+1);
	fread(ch,length,1,fp);
	*(ch+length)='\0';
	fclose(fp);
	return ch;
}


//	===========================
	/*	
	int64_t start = now();
	.....
	int64_t end = now();
	double sec = (end-start)/1000000.0;
	printf("%f sec %f ms \n", sec, 1000*sec);
	*/
//  ===========================
int64_t now()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

void user_wait()
{
	int c;
	printf("user_wait... ");
	do
	{
		c = getchar();
		if(c == 'g') break;
	} while(c != '\n');
}

void delay(){
	int i = 0,j = 0;
	for(i = 0;i<50;i++){
		for(j=0;j<50;j++){
			int x = 0;
		}
	}
}

void stop(){
	while(1){
		;
	}
}
