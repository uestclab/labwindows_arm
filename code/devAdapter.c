#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<unistd.h>
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


int main(int argc,char** argv)
{
	int connfd = -1;
	int ret = initProcBroker(argv[0],&connfd);
	pthread_t* receive_pid= initNet(&connfd);

	pthread_join(*receive_pid, NULL);
	printf("end_2\n");
	while(1){

	}
    return 0;
}

