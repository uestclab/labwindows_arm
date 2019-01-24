#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<errno.h>
#include<string.h>
#include<unistd.h>
#include<netinet/in.h>
#define BUFFER_SIZE 2560*4

static void Usage(const char* proc)
{
    printf("%s : [server_ip][server_port]",proc);
}

//./client server_ip  server_port
int main(int argc,char* argv[])
{
	int port = 55555;
	const char* tempBuf = "192.168.2.112";
	char sendbuf[1024] = {'1'};
	char sendMessage[1500];

    int sock = socket(AF_INET,SOCK_STREAM,0);
    if(sock < 0)
    {
        perror("sock");
        return 1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(tempBuf);


    if(connect(sock,(struct sockaddr*)&server,sizeof(server)) < 0)
    {
        perror("connect");
        return 2;
    }
	
	char* recvbuf = malloc(BUFFER_SIZE);

	int messageLen = 1024 + 4;
	memcpy(sendMessage,&messageLen,4);
	memcpy(sendMessage+4,sendbuf,1024);
	int status = send(sock, sendMessage, messageLen + sizeof(int32_t), 0);
    while(1)
    {
		int n = recv(sock, recvbuf, BUFFER_SIZE,0);
    }
    close(sock);
    return 0;
}

