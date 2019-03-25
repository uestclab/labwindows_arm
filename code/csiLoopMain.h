#ifndef CSILOOPMAIN_H
#define CSILOOPMAIN_H

#include "cst_net.h"
#include "zlog.h"


void* initCstNet();
void init_cst_state(zlog_category_t* log_handler);

void gw_startcsi();
void gw_stopcsi();

void gw_closecsi();







#endif//CSILOOPMAIN_H
