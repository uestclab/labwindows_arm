#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <string.h>

#define HOST "localhost"
#define PORT  1883
#define KEEP_ALIVE 60
#define MSG_MAX_SIZE  512

//gcc -o publish publish.c -lmosquitto

bool session = true;

int main()
{
    char buff[MSG_MAX_SIZE];
    struct mosquitto *mosq = NULL;

    mosquitto_lib_init(); //

    mosq = mosquitto_new(NULL,session,NULL);//
    if(!mosq){
        printf("create client failed..\n");
        mosquitto_lib_cleanup();
        return 1;
    }

    if(mosquitto_connect(mosq, HOST, PORT, KEEP_ALIVE)){ //
        fprintf(stderr, "Unable to connect.\n");
        return 1;
    }

    int loop = mosquitto_loop_start(mosq); // new internal thread
    if(loop != MOSQ_ERR_SUCCESS)
    {
        printf("mosquitto loop error\n");
        return 1;
    }
    while(fgets(buff, MSG_MAX_SIZE, stdin) != NULL)
    {
        mosquitto_publish(mosq,NULL,"jjl/message",strlen(buff)+1,buff,0,0);
        memset(buff,0,sizeof(buff));
    }
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
