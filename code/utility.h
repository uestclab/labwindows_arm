#ifndef UTILITY_H
#define UTILITY_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

typedef struct shareBufInfo{
    char*       buf_;
    int32_t 	len_;
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
int64_t now();

char *get_prog_name(char *argv);
int connect_check(char* eth);


void user_wait();
void delay();
void stop();


#endif//UTILITY_H
