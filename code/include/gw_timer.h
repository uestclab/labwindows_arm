#ifndef GW_TIMER_H
#define GW_TIMER_H


#include "zlog.h"
#include "utility.h"

#include "msg_queue.h"

typedef void (*timercb_func)(g_msg_queue_para* g_msg_queue); 

typedef struct g_timer_para{
	g_msg_queue_para*  g_msg_queue;
	timercb_func       timer_cb;
	int                seconds;
	int                mseconds;       
	int                running;
	para_thread*       para_t;
	zlog_category_t*   log_handler;
}g_timer_para;


int InitTimerThread(g_timer_para** g_timer, g_msg_queue_para* g_msg_queue, zlog_category_t* handler);
void close_Timer(g_timer_para* g_timer);
void Start_Timer(timercb_func timer_cb, int sec, int msec, g_timer_para* g_timer);


#endif//GW_TIMER_H


