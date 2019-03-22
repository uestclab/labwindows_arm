#ifndef PROCESS_H
#define PROCESS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "cJSON.h"
#include "zlog.h"


int initNet(int *fd,zlog_category_t* log_handler);

int sendToPc(int connfd, char* send_buf, int send_buf_len);

void receive_signal();

void stopReceThread();

#endif//PROCESS_H
