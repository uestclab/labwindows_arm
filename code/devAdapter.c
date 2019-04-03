#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "zlog.h"

#include "msg_queue.h"
#include "event_process.h"
#include "gw_timer.h"
#include "procBroker.h"

//#ifndef PC_STUB
//#endif

zlog_category_t * serverLog(const char* path){
	int rc;
	zlog_category_t *zlog_handler = NULL;

	rc = zlog_init(path);

	if (rc) {
		printf("init serverLog failed\n");
		return NULL;
	}

	zlog_handler = zlog_get_category("devAdapterlog");

	if (!zlog_handler) {
		printf("get cat fail\n");

		zlog_fini();
		return NULL;
	}

	return zlog_handler;
}

void closeServerLog(){
	zlog_fini();
}


int main(int argc,char** argv)
{
	zlog_category_t *zlog_handler = serverLog("/run/media/mmcblk1p1/etc/zlog_default.conf");

	zlog_info(zlog_handler,"start devAdapter process\n");

	/* */
	
	/* msg_queue */
	g_msg_queue_para* g_msg_queue = createMsgQueue(zlog_handler);
	if(g_msg_queue == NULL){
		zlog_info(zlog_handler,"No msg_queue created \n");
		return 0;
	}
	zlog_info(zlog_handler, "g_msg_queue->msgid = %d \n", g_msg_queue->msgid);

	/* timer thread */
	g_timer_para* g_timer = NULL;
	int state = InitTimerThread(&g_timer, g_msg_queue, zlog_handler);
	if(state == -1 || g_timer == NULL){
		zlog_info(zlog_handler,"No Timer created \n");
		return 0;
	}

	/* server thread */
	g_server_para* g_server = NULL;
	state = InitServerThread(&g_server, g_msg_queue, zlog_handler);
	if(state == -1 || g_server == NULL){
		zlog_info(zlog_handler,"No server thread created \n");
		return 0;
	}

	/* */
	g_broker_para* g_broker = NULL;
	state = initProcBroker(argv[0], &g_broker, g_server, zlog_handler);
	if(state != 0 || g_server == NULL){
		zlog_info(zlog_handler,"No Broker created \n");
		return 0;
	}

	eventLoop(g_server, g_msg_queue, g_timer, g_broker, zlog_handler);

	/* 

	InitTimer(zlog_handler);
	init_cst_state(zlog_handler);
	int connfd = -1;

	int ret = initProcBroker(argv[0],&connfd,zlog_handler);
	if(ret != 0){
		zlog_info(zlog_handler,"initProcBroker fail ... end process\n");
		closeServerLog();
		return 0;
	}

	ret = initNet(&connfd,zlog_handler);

	destoryProcBroker();
	zlog_info(zlog_handler,"devAdapter end main\n");
	closeTimer();
	*/
	closeServerLog();
    return 0;
}


