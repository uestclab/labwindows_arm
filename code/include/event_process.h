#ifndef EVENT_PROCESS_H
#define EVENT_PROCESS_H
#include <stdio.h>  
#include <pthread.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <signal.h>   
#include <string.h>  
#include <errno.h>
#include "zlog.h"
  
#include "server.h"
#include "msg_queue.h"
#include "gw_timer.h"
#include "procBroker.h"


void eventLoop(g_server_para* g_server, g_msg_queue_para* g_msg_queue, g_timer_para* g_timer, 
		g_broker_para* g_broker, zlog_category_t* zlog_handler); 



#endif//EVENT_PROCESS_H


