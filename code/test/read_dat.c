#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void swap(char*a){
	char temp;
	temp = a[0];
	a[0] = a[1];
	a[1] = temp;
}

void main()
{
	FILE* fp;

	if((fp=fopen("csi_s.dat","rb"))==NULL)
	{
		printf("无法打开文件");
		exit(0);
	}
	printf("unsigned short = %d \n", sizeof(unsigned short));
	char get[1024];
	size_t num = fread(get,sizeof(char),1024,fp);
	char* p_get = get;
	char a[2];
	int i;
	for(i=0;i<1024;i=i+2){
		unsigned short temp = 0;
		memcpy(&temp,p_get,2);
		memcpy(a,p_get,2);
		p_get = p_get + 2;
		
		int value_i = temp;
		if(value_i > 32767)
			value_i = value_i - 65536;
	
		printf("get_data(ushort) = %d, value_i = %d \n",temp, value_i);
	}
}



