#ifndef STUB_H
#define STUB_H
#include "zlog.h"

int stubMain(int *fd);
void startLoop();

void startcsi();
void stopcsi();

void close_csi();

int inquiry_state_from(char *buf, int buf_len);
int initProcBroker(char *argv,int* fd,zlog_category_t* log_handler);
void* initCstNet(zlog_category_t* log_handler);

void destoryProcBroker();
int rssi_state_change(char *buf, int buf_len);

#endif
