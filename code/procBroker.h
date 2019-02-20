#ifndef PROCBROKER_H
#define PROCBROKER_H

int initProcBroker(char *argv,int* fd);
int inquiry_state_from(char *buf, int buf_len);

#endif//PROCBROKER_H
