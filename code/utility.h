#ifndef UTILITY_H
#define UTILITY_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct shareBufInfo{
    char*       buf_;
    int32_t 		len_;
}shareBufInfo;

typedef struct para_thread{
    pthread_t*       thread_pid;
    pthread_mutex_t* mutex_;
    pthread_cond_t*  cond_;
}para_thread;

para_thread* newThreadPara();
void destoryThreadPara(para_thread* para_t);

int32_t myNtohl(const char* buf);
int filelength(FILE *fp);
char* readfile(const char *path);
void user_wait();
void delay();
void stop();


#endif//UTILITY_H
