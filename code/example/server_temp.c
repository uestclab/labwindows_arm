#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "broker.h"
#include "cJSON.h"

#define BUFFER_SIZE 2560*4
char* sendMessage = NULL;

int filelength(FILE *fp)
{
	int num;
	fseek(fp,0,SEEK_END);
	num=ftell(fp);
	fseek(fp,0,SEEK_SET);
	return num;
}

char* readfile(const char *path)
{
	FILE *fp;
	int length;
	char *ch;
	if((fp=fopen(path,"r"))==NULL)
	{
		exit(0);
	}
	length=filelength(fp);
	ch=(char *)malloc(length+1);
	fread(ch,length,1,fp);
	*(ch+length)='\0';
	fclose(fp);
	return ch;
}

void user_wait()
{
	int c;
	printf("user_wait... ");
	do
	{
		c = getchar();
		if(c == 'g') break;
	} while(c != '\n');
}

void delay(){
	int i = 0,j = 0;
	for(i = 0;i<1000;i++){
		for(j=0;j<1000;j++){
			int x = 0;
		}
	}
}

void stop(){
	while(1){
		;
	}
}


int client_exception(char* buf, int buf_len, char *from, void* arg)
{
	int ret = 0;
	printf("buf = %s\n",buf); // exception json
	return ret;
}


int main(int argc, char **argv){
	sendMessage = malloc(BUFFER_SIZE);
	const char* file_path = "/home/gyl/liqingSpace/code/labwindows/code/cst_upload_to_arm.dat";
	char* file_buf = readfile(file_path);
	int length = strlen(file_buf);
	printf("length = %d\n",length);
	printf("app name = %s \n",get_prog_name(argv[0]));
	int ret = init_broker(get_prog_name(argv[0]), NULL, -1, NULL, NULL);
	ret = register_callback("all", client_exception, "event");

	char *stat_buf;
	int stat_buf_len;
	int inbuf_len = 0;
	char* json_file = readfile("/home/gyl/liqingSpace/code/labwindows/code/gpio_stat.json");
	inbuf_len = strlen(json_file);
	printf("inbuf_len = %d\n",inbuf_len);
	cJSON * root = NULL;
    cJSON * item = NULL;
    root = cJSON_Parse(json_file);
	printf("root = %s\n",root);
	free(json_file);
    item = cJSON_GetObjectItem(root, "dst");
	printf("dst = %s\n",item->valuestring);
	//ret = dev_transfer(json_file, inbuf_len, &stat_buf, &stat_buf_len, item->valuestring, -1); // get value
	printf("dev_transfer ret = %d\n",ret);
	cJSON_Delete(root);
	//stop();

	printf("-------------------------------------------------");


    int listenfd,connfd;
    struct sockaddr_in servaddr;
 
    if( (listenfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
    {
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
 
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(55555);
 	
    if( bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
    {
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
 
    if( listen(listenfd,10) == -1)
    {
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
 
    printf("========waiting for client's request========\n");

    if( (connfd = accept(listenfd,(struct sockaddr*)NULL,NULL)) == -1 )
    {
        printf("accept socket error: %s(errno: %d)\n",strerror(errno),errno);
		return 0;
    }

    printf("accept from client: %d\n",connfd);
	
	int send_num = 100000;
	int error = 0;
	int counter = 0;
	user_wait();
	while(send_num--){
		//*((int32_t*)sendMessage) = htonl(length);
		int messageLen = length + 4;
		memcpy(sendMessage,&messageLen,4);
		memcpy(sendMessage+4,&send_num,4);
		memcpy(sendMessage+8,file_buf,length);
		int status = send(connfd, sendMessage, length+4+4, 0);
		if(status != (length + 4+4))
			error = error + 1;
		counter = counter + 1;
		if(counter == 5){
			counter = 0;
			int n = recv(connfd,sendMessage,BUFFER_SIZE,0);
			printf("receive %d , json: %s\n",n , sendMessage);			
			//user_wait();
		}
		delay();
	}
	printf("error = %d",error);
	stop();
	close(connfd);
	close(listenfd);
	return 0;
}














