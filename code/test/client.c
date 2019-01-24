#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include "utility.h"
 
#define MAXLINE 4096
 
int main(int argc,char** argv)
{
	const char* file_path = "/home/gyl/liqingSpace/code/labwindows/labwindows_arm/code/gpio_stat.json";
	char* file_buf = readfile(file_path);
	int file_len = strlen(file_buf) + 1;

	char* sendBuf = malloc(MAXLINE);
	int send_len = file_len + sizeof(int32_t);
	printf("send_len = %d\n",send_len);
	*((int32_t*)sendBuf) = htonl(file_len);
	memcpy(sendBuf+4,file_buf,file_len);

    int sockfd,n;
    struct sockaddr_in servaddr;
    char sendline[MAXLINE],recvline[MAXLINE];
 	
    if(argc!=2)
    {
        printf("usage: ./client <ipaddress>\n");
        return 0;
    }
 
    if( (sockfd = socket(AF_INET,SOCK_STREAM,0))<0 )
    {
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
 
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(55555);
    if(inet_pton(AF_INET,argv[1],&servaddr.sin_addr)<=0)
    {
        printf("inet_pton error for %s\n",argv[1]);
        return 0;
    }
 
    if( connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr))<0 )
    {
        printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
 
    printf("send msg to server: \n");
    fgets(sendline,MAXLINE,stdin);
	while(1){
		if( send(sockfd,sendBuf,send_len,0)<0)
		{
		    printf("send msg error: %s(errno: %d)\n",strerror(errno),errno);
		    return 0;
		}
		recv(sockfd, recvline, MAXLINE,10);
		int len = myNtohl(recvline);
		int type = myNtohl(recvline+4);
		printf("messlen = %d , type = %d , receive from server : %s \n", len, type, recvline + 8);
		fgets(sendline,MAXLINE,stdin);
	}
    close(sockfd);

    return 0;
}
