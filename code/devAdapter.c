#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "procBroker.h"
#include "utility.h"
#include "process.h"

/*---------------------------------------------------------------------------*/
/*  main thread , receive socket thread , broker thread inform callback 	 */
/*  receive -- | messageLength(4 Byte) | json file(messageLength) |          */
/*  send    -- | messageLenght(4 Byte) | type(4 Byte) | payload   |          */
/*  type : 1--rssi , 2--CSI , 3--json                                        */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*  change version : 1. 	//return ntohl(be32); return be32;               */
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



int main(int argc,char** argv)
{

	if( SIG_ERR == signal(SIGINT, process_signal) ){
		
	}

	int connfd = -1;
	pthread_t* receive_pid = NULL;
	//int ret = initProcBroker(argv[0],&connfd);

	receive_pid= initNet(&connfd);

	pthread_join(*receive_pid, NULL);
	printf("end_2\n");

	printf("end main\n");
    return 0;
}

