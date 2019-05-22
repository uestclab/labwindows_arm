#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#define LENGTH_COMPLEX 16
void test(void)
{
    double*      vectReal = (double*)malloc(LENGTH_COMPLEX*sizeof(double));
    double*      vectImag = (double*)malloc(LENGTH_COMPLEX*sizeof(double));
	char tmp_str[32] = {0x3c,0xeb,0x13,0xeb,0xea,0xeb,0x14,0xeb,
			0x3d,0x3c,0x3d,0xc2,0x14,0x3d,0xea,0x14,
			0x3d,0xc2,0x14,0x14,0x3d,0x14,0xeb,0xc1,
			0xeb,0x14,0xeb,0xea,0xc1,0x14,0xea,0xc2};

	int i,j=0;
	int sw_temp = 0;
	int cnt_stream = 0;
	for(i=0;i<32;i++){
		unsigned short tmp = (unsigned short)(tmp_str[i]);
		int value_i = tmp;
		if(value_i > 32767)
			value_i = value_i - 65536;
		printf("tmp_str[%d] = 0x%x , %d\n",i,tmp_str[i],value_i);
		if(sw_temp == 0){
			vectReal[j] = value_i;
			sw_temp = 1;
			cnt_stream = cnt_stream + 1;
		}else if(sw_temp == 1){
			vectImag[j] = value_i;
			sw_temp = 0;
			j = j + 1;
			cnt_stream = cnt_stream + 1;
		}
	}
	
	for(i=0;i<16;i++){
		printf("vectReal[%d] = %f , vectImag[%d] = %f \n",i,vectReal[i],i,vectImag[i]);
	}
	
}
 
int main(void)
{
	test();
	int value = 0x3<<5;
	printf("value = 0x%x\n",value);
	return 0;
}

