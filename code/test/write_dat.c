#include <stdlib.h>
#include <stdio.h>
void main()
{
	FILE* fp;
	if((fp=fopen("1.dat","wb"))==NULL)
	{
		printf("无法打开文件");
		exit(0);
	}

    double data[2] = {0.003, -0.006};
	fwrite(data, sizeof(double), 2, fp);
	fclose(fp);

	if((fp=fopen("test.dat","rb"))==NULL)
	{
		printf("无法打开文件");
		exit(0);
	}
	double get[256] = {0};
	size_t num = fread(get,sizeof(double),256,fp);
	int i;	
	for(i=0;i<256;i++)	
		printf("get_data = %f,\n",get[i]);
}



