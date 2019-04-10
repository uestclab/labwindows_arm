#ifndef COUNTDOWNLATCH_H
#define COUNTDOWNLATCH_H

#include <unistd.h>
#include "zlog.h"

#include "utility.h"

typedef struct g_cntDown_para{
	para_thread*       para_t;
	int                counter;
	zlog_category_t*   log_handler;
}g_cntDown_para;


int initCountDown(g_cntDown_para** g_cntDown, int counter, zlog_category_t* handler);
void counterWait(g_cntDown_para* g_cntDown);
void counterDown(g_cntDown_para* g_cntDown);

#endif//COUNTDOWNLATCH_H

