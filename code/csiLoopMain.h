#ifndef CSILOOPMAIN_H
#define CSILOOPMAIN_H

//#include "libaxidma.h"


int csiLoopMain(int *fd);

void startLoop();

void send_csi(char* buf, int buf_len);









#endif//CSILOOPMAIN_H
