#ifndef STUB_H
#define STUB_H

int stubMain(int *fd);
void startLoop();

void stopcsi();
void* initCstNet();

int inquiry_state_from(char *buf, int buf_len);
int initProcBroker(char *argv,int* fd);




#endif
