#ifndef CSILOOPMAIN_H
#define CSILOOPMAIN_H

#include "cst_net.h"
#include "zlog.h"


void* initCstNet(zlog_category_t* log_handler);

void startcsi();
void stopcsi();

void closecsi();







#endif//CSILOOPMAIN_H
