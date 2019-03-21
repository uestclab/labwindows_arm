#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "utility.h"
#include "process.h"
#include "zlog.h"

#ifdef USE_STUB
	#include "stub.h"
#endif

#ifndef USE_STUB
	#include "csiLoopMain.h"
	#include "procBroker.h"
#endif


/*---------------------------------------------------------------------------*/
/*  main thread , receive socket thread , broker thread inform callback 	 */
/*  receive -- | messageLength(4 Byte) | json file(messageLength) |          */
/*  send    -- | messageLenght(4 Byte) | type(4 Byte) | payload   |          */
/*  type : 1--rssi , 2--CSI , 3--json                                        */
/*                                                                           */
/*---------------------------------------------------------------------------*/

static void process_signal(int signal)
{
    switch(signal) {  
		case SIGINT: 
			receive_signal();
			break;
		default: 
		    break;
    }
}


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
	if( SIG_ERR == signal(SIGINT, process_signal) ){
		
	}

	zlog_info(zlog_handler,"start devAdapter process\n");
	int connfd = -1;
#ifdef USE_STUB
	printf("stubMain()\n");
	stubMain(&connfd); // stub test
#endif
	int ret = initProcBroker(argv[0],&connfd,zlog_handler);
	if(ret != 0){
		zlog_info(zlog_handler,"initProcBroker fail ... end process\n");
		closeServerLog();
		return 0;
	}

	ret = initNet(&connfd,zlog_handler);

	destoryProcBroker();
	zlog_info(zlog_handler,"devAdapter end main\n");
	closeServerLog();
    return 0;
}


