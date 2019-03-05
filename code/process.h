#ifndef PROCESS_H
#define PROCESS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "cJSON.h"


int initNet(int *fd);

int sendToPc(int connfd, char* send_buf, int send_buf_len);

void receive_signal();

#endif//PROCESS_H
