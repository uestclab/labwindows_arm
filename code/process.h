#ifndef PROCESS_H
#define PROCESS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "cJSON.h"

#define RECEIVE_BUFFER 1000 * 40


pthread_t* initNet(int *fd);

int sendToPc(int connfd, char* send_buf, int send_buf_len);

void* test_send(void* args);

#endif//PROCESS_H
