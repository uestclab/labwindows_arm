#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>

#include "gw_timer.h"



void set_Timer(int seconds, int mseconds)
{
    struct timeval temp;

    temp.tv_sec = seconds;
    temp.tv_usec = mseconds;

    select(0, NULL, NULL, NULL, &temp);
    return ;
}

void process_loop(g_timer_para* g_timer){
	while(1){
		set_Timer(g_timer->seconds,g_timer->mseconds);
		g_timer->timer_cb(g_timer->g_msg_queue);
	}
}

void* timer_thread(void* args){
	g_timer_para* g_timer = (g_timer_para*)args;

	pthread_mutex_lock(g_timer->para_t->mutex_);
    while(1){
		while (g_timer->running == 0 )
		{
			zlog_info(g_timer->log_handler,"timer_thread() : wait for condition\n");
			pthread_cond_wait(g_timer->para_t->cond_, g_timer->para_t->mutex_);
		}
		pthread_mutex_unlock(g_timer->para_t->mutex_);
		if(g_timer->running == 2)
			break;
    	process_loop(g_timer);		
    }
    zlog_info(g_timer->log_handler,"Exit timer_thread()");

}

void Start_Timer(timercb_func timer_cb, int sec, int msec, g_timer_para* g_timer){
	zlog_info(g_timer->log_handler,"StartTimer()");
	g_timer->timer_cb = timer_cb;
	g_timer-> seconds  = sec;
	g_timer-> mseconds  = msec;
	g_timer-> running  = 1;
	pthread_cond_signal(g_timer->para_t->cond_);
}



int InitTimerThread(g_timer_para** g_timer, g_msg_queue_para* g_msg_queue, g_cntDown_para* g_cntDown, zlog_category_t* handler){
	zlog_info(handler,"InitTimerThread()");
	*g_timer = (g_timer_para*)malloc(sizeof(struct g_timer_para));
	(*g_timer)-> timer_cb = NULL;
	(*g_timer)-> running  = 0;
	(*g_timer)-> seconds  = 0;
	(*g_timer)-> mseconds  = 0;
	(*g_timer)-> para_t = newThreadPara();
	(*g_timer)->g_msg_queue = g_msg_queue;
	(*g_timer)->g_cntDown   = g_cntDown;
	(*g_timer)-> log_handler = handler;
	int ret = pthread_create((*g_timer)->para_t->thread_pid, NULL, timer_thread, (void*)(*g_timer));
    if(ret != 0){
        zlog_error(handler,"create monitor_thread error ! error_code = %d", ret);
		return -1;
    }	
	return 0;
}


void close_Timer(g_timer_para* g_timer){
	g_timer-> running  = 2;
	pthread_cond_signal(g_timer->para_t->cond_);
	pthread_join(*(g_timer->para_t->thread_pid),NULL);
	zlog_info(g_timer->log_handler,"closeTimer()");
	// continue to release timer resource --- 
}

// ---------------------------------------------------------------------




