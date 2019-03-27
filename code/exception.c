#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include "exception.h"
#include "utility.h"
#include "process.h"

zlog_category_t * ex_log_handler = NULL;

static int heart_beat = 0;
static int timer_running = 0;

para_thread* para_t_check = NULL;


void setTimer(int seconds, int mseconds)
{
    struct timeval temp;

    temp.tv_sec = seconds;
    temp.tv_usec = mseconds;

    select(0, NULL, NULL, NULL, &temp);
    return ;
}

void OnTimer(){
	int ret = check_variable(0,0);
	if(ret == 1){ // receive heart_beat
		//zlog_info(ex_log_handler,"received heart_beat ** \n");
	}else if(ret == 0){ // not receive in timer
		zlog_info(ex_log_handler,"No heart_beat !!!!! \n");
		stopReceThread();
		StopTimer();
	}
}


void* thread_proc()
{
	zlog_info(ex_log_handler,"Start Timer thread \n");
    while (timer_running)
    {
		setTimer(4,0); // 4s check
		OnTimer();
    }
	zlog_info(ex_log_handler,"Stop Timer thread \n");
}

void StartTimer()
{
	zlog_info(ex_log_handler,"StartTimer() \n");
	timer_running = 1;
	pthread_create(para_t_check->thread_pid, NULL, thread_proc, NULL);
}

void StopTimer()
{
	zlog_info(ex_log_handler,"StopTimer() \n");
	timer_running = 0;
	//pthread_exit(0);
}

void InitTimer(zlog_category_t * log_handler){
	ex_log_handler = log_handler;
	if(para_t_check == NULL)
		para_t_check = newThreadPara();
	heart_beat = 0;
	timer_running = 0;
	zlog_info(ex_log_handler,"InitTimer() \n");
}

void closeTimer(){
	if(para_t_check != NULL){
		destoryThreadPara(para_t_check);
		para_t_check = NULL;
	}
	ex_log_handler = NULL;
}

/* cmd : 
	read  0 ; check_variable(0,X)
	write 1 ; check_variable(1,x)
   return :
	2: 
	1: received in time_out
	0: not received in time_out
*/
int check_variable(int cmd, int value){
	pthread_mutex_lock(para_t_check->mutex_);
	
	int ret = -3;
	if(cmd == 0){
		//zlog_info(ex_log_handler,"check_variable() : read \n");
		if(heart_beat == 0)
			ret = 0;
		else if(heart_beat == 1){
			heart_beat = 0;
			ret = 1;
		}
	}else if(cmd == 1){
		//zlog_info(ex_log_handler,"check_variable() : write \n");
		heart_beat = value;
		ret = 2;
	}	

	pthread_mutex_unlock(para_t_check->mutex_);
	return ret;
} 




