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
#include "csi_handler.h"
#include "countDownLatch.h"


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

void c_compiler_builtin_macro(zlog_category_t* zlog_handler)
{
	zlog_info(zlog_handler,"gcc compiler ver:%s\n",__VERSION__);
	zlog_info(zlog_handler,"this version built time is:[%s  %s]\n",__DATE__,__TIME__);
	//printf("gcc compiler ver:%s\n",__VERSION__);
	//printf("this version built time is:[%s  %s]\n",__DATE__,__TIME__);
}

int main(int argc,char** argv)
{
	zlog_category_t *zlog_handler = serverLog("/run/media/mmcblk1p1/etc/zlog_default.conf");
	//zlog_category_t *zlog_handler = serverLog("./zlog_default.conf");

	zlog_info(zlog_handler,"start devAdapter process\n");

	c_compiler_builtin_macro(zlog_handler);

	/* countDownLatch */
	g_cntDown_para* g_cntDown = NULL;
	int state = initCountDown(&g_cntDown, 1, zlog_handler);
	
	
	/* msg_queue */
	g_msg_queue_para* g_msg_queue = createMsgQueue(zlog_handler);
	if(g_msg_queue == NULL){
		zlog_info(zlog_handler,"No msg_queue created \n");
		return 0;
	}
	zlog_info(zlog_handler, "g_msg_queue->msgid = %d \n", g_msg_queue->msgid);

	/* timer thread */
	g_timer_para* g_timer = NULL;
	state = InitTimerThread(&g_timer, g_msg_queue, g_cntDown, zlog_handler);
	if(state == -1 || g_timer == NULL){
		zlog_info(zlog_handler,"No Timer created \n");
		return 0;
	}

	/* server thread */
	g_server_para* g_server = NULL;
	state = InitServerThread(&g_server, g_msg_queue, g_cntDown, zlog_handler);
	if(state == -1 || g_server == NULL){
		zlog_info(zlog_handler,"No server thread created \n");
		return 0;
	}
	
	/* broker handler */
	g_broker_para* g_broker = NULL;
	state = initProcBroker(argv[0], &g_broker, g_server, zlog_handler);
	if(state != 0 || g_server == NULL){
		zlog_info(zlog_handler,"No Broker created \n");
		return 0;
	}

	/* csi_handler */
	g_csi_para* g_csi = NULL;
	state = init_cst_state(&g_csi, g_server, g_msg_queue, zlog_handler);
	if(state != 0 || g_csi == NULL){
		zlog_info(zlog_handler,"No init_cst_state created \n");
		return 0;
	}

	eventLoop(g_server, g_msg_queue, g_timer, g_broker, g_csi, zlog_handler);

	closeServerLog();
    return 0;
}


