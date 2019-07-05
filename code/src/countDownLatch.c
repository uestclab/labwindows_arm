#include "countDownLatch.h"

int initCountDown(g_cntDown_para** g_cntDown, int counter, zlog_category_t* handler){
	zlog_info(handler,"initCountDown()");
	*g_cntDown = (g_cntDown_para*)malloc(sizeof(struct g_cntDown_para));
	(*g_cntDown)->counter      	    = counter;
	(*g_cntDown)->para_t       		= newThreadPara();
	(*g_cntDown)->log_handler       = handler;

	zlog_info(handler,"end initCountDown()\n");
	return 0;
}

void counterWait(g_cntDown_para* g_cntDown){
	pthread_mutex_lock(g_cntDown->para_t->mutex_);  
	while(g_cntDown->counter > 0){//
		pthread_cond_wait(g_cntDown->para_t->cond_, g_cntDown->para_t->mutex_);		
	}
	pthread_mutex_unlock(g_cntDown->para_t->mutex_);
}

void counterDown(g_cntDown_para* g_cntDown){
	pthread_mutex_lock(g_cntDown->para_t->mutex_); 
	g_cntDown->counter--;
	if(g_cntDown->counter == 0){
		pthread_cond_signal(g_cntDown->para_t->cond_);  //used to wake up the lock;
	}
	pthread_mutex_unlock(g_cntDown->para_t->mutex_);
}
