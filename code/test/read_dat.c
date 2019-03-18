#include <stdlib.h>
#include <stdio.h>
#include <string.h>


void swap(char*a){
	char temp;
	temp = a[0];
	a[0] = a[1];
	a[1] = temp;
}

double tranform(char *input,int length){
	double value_d = 0;
	unsigned short temp = 0;
	
	char a[2];
	memcpy(a,input,2);
	swap(a);
	temp = *((unsigned short*)a);
	
	int value_i = temp;
	if(value_i > 32767)
		value_i = value_i - 65536;
	value_d =  (value_i / 1.0) / 32768;
	printf("original = %d , value_i = %d , value_d = %f\n",temp,value_i,value_d);
	return value_d;
}

void parse_IQ_from_net(char* file_buf, int length){
	char input_c[2];
	int counter = 0;
	int stream_counter = 0;
	double IQ_stream[256*2];
	int i;
	for(i=0;i<length;i++){
		input_c[counter] = *(file_buf + i);
		counter = counter + 1;
		if(counter == 2){
			double temp_d = tranform(input_c,2);
			IQ_stream[stream_counter] = temp_d;
			stream_counter = stream_counter + 1;
			counter = 0;
		}
	}
	printf("stream_counter = %d \n",stream_counter);
	stream_counter = 0;
/*
	for(i=0;i<512;i = i + 16){
		out_IQ[stream_counter].real   = IQ_stream[i];
		out_IQ[stream_counter+1].real = IQ_stream[i+1];
		out_IQ[stream_counter+2].real = IQ_stream[i+2];
		out_IQ[stream_counter+3].real = IQ_stream[i+3];
		out_IQ[stream_counter+4].real = IQ_stream[i+4];
		out_IQ[stream_counter+5].real = IQ_stream[i+5];
		out_IQ[stream_counter+6].real = IQ_stream[i+6];
		out_IQ[stream_counter+7].real = IQ_stream[i+7];
		out_IQ[stream_counter].imaginary   = IQ_stream[i+8];
		out_IQ[stream_counter+1].imaginary = IQ_stream[i+9];
		out_IQ[stream_counter+2].imaginary = IQ_stream[i+10];
		out_IQ[stream_counter+3].imaginary = IQ_stream[i+11];
		out_IQ[stream_counter+4].imaginary = IQ_stream[i+12];
		out_IQ[stream_counter+5].imaginary = IQ_stream[i+13];
		out_IQ[stream_counter+6].imaginary = IQ_stream[i+14];
		out_IQ[stream_counter+7].imaginary = IQ_stream[i+15];
		stream_counter = stream_counter + 8;
	}
*/
}

void main()
{
	FILE* fp;

	if((fp=fopen("test_parse.dat","rb"))==NULL)
	{
		printf("无法打开文件");
		exit(0);
	}
	printf("unsigned short = %d \n", sizeof(unsigned short));
	char get[1024];
	size_t num = fread(get,sizeof(char),1024,fp);
	parse_IQ_from_net(get,1024);
}



