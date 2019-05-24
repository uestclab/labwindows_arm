#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
int checkIQ(char input){
	if(input < 0)
		return 1;
	else
		return 0;
}

void shiftIQ(char* input){
	*input = (*input) << 1;
}


#define LENGTH_COMPLEX 16
void test(void)
{
	int i,j=0;
	int sw_temp = 0;
    double*      vectReal = (double*)malloc(LENGTH_COMPLEX*sizeof(double));
    double*      vectImag = (double*)malloc(LENGTH_COMPLEX*sizeof(double));
	char tmp_str[32] = {
			0x3c,0xeb,0x13,0xeb,0xea,0xeb,0x14,0xeb,
			0x3d,0x3c,0x3d,0xc2,0x14,0x3d,0xea,0x14,
			0x3d,0xc2,0x14,0x14,0x3d,0x14,0xeb,0xc1,
			0xeb,0x14,0xeb,0xea,0xc1,0x14,0xea,0xc2};
	for(i=0;i<32;i++){
		printf("before tmp_str = 0x%x \n", tmp_str[i]);
		tmp_str[i] = (tmp_str[i] >> 1);
		printf("after shift tmp_str = 0x%x \n", tmp_str[i]);
		if(sw_temp == 0){
			if(checkIQ(tmp_str[i]) == 0)
				tmp_str[i] = tmp_str[i] + 0x80;
			sw_temp = 1;
		}else if(sw_temp == 1){
			if(checkIQ(tmp_str[i]) == 1)
				tmp_str[i] = tmp_str[i] - 0x80;
			sw_temp = 0;
		}
		printf("modify --- tmp_str[%d] = 0x%x \n",i,tmp_str[i]);
	}

	char shift_str[32];
	memcpy(shift_str,tmp_str,32);

	sw_temp = 0;
	int cnt_stream = 0;
	for(i=0;i<32;i++){
		if(checkIQ(tmp_str[i]) == 1)
			printf("tmp_str[%d] = 0x%x is I data \n", i,tmp_str[i]);
		else
			printf("tmp_str[%d] = 0x%x is Q data \n", i,tmp_str[i]);
		shiftIQ(&shift_str[i]);
		unsigned short tmp = (unsigned short)(shift_str[i]);
		int value_i = tmp;
		if(value_i > 32767)
			value_i = value_i - 65536;
		printf("tmp_str[%d] = 0x%x , value_i = %d , shift_str[%d] = 0x%x \n",i,tmp_str[i],value_i,i,shift_str[i]);
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

