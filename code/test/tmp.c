#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>


unsigned int stringToInt(char* ret){
	if(ret == NULL){
		return -1;
	}
	int i;
	int len = strlen(ret);
	if(len < 3)
		return -1;
	if(ret[0] != '0' || ret[1] != 'x')
		return -1;
	unsigned int number = 0;
	int flag = 0;
	for(i=2;i<len;i++){
		if(ret[i] == '0' && flag == 0)
			continue;
		flag = 1;
		int temp = 0;
		if(ret[i]>='0'&&ret[i]<='9')
			temp = ret[i] - '0';
		else if(ret[i]>='a'&&ret[i]<='f')
			temp = ret[i] - 'a' + 10;
		else if(ret[i]>='A'&&ret[i]<='F')
			temp = ret[i] - 'A' + 10;
		if(i == len - 1)
			number = number + temp;
		else
			number = (number + temp)*16;
	}
	return number;
}


double calculateFreq(char* ret){ // 0x04FF04FF , 0x04FF0400 , 0x04000400
	int freq_offset_i = 0, freq_offset_q =0, freq_offset_i_pre=0, freq_offset_q_pre=0;

	unsigned int number = stringToInt(ret);

	freq_offset_q_pre = number - ((number>>8) << 8);
	number = number >> 8;
	freq_offset_i_pre = number - ((number>>8) << 8);
	number = number >> 8;
	freq_offset_q = number - ((number>>8) << 8);
	number = number >> 8;
	freq_offset_i = number - ((number>>8) << 8);

	printf("calculateFreq--------- : freq_offset_i = %d , freq_offset_q = %d , freq_offset_i_pre = %d , freq_offset_q_pre = %d \n",
		freq_offset_i, freq_offset_q , freq_offset_i_pre , freq_offset_q_pre);

	if(freq_offset_i > 127)
		freq_offset_i = freq_offset_i - 256;

	if(freq_offset_q > 127)
		freq_offset_q = freq_offset_q - 256;

	if(freq_offset_i_pre > 127)
		freq_offset_i_pre = freq_offset_i_pre - 256;

	if(freq_offset_q_pre > 127)
		freq_offset_q_pre = freq_offset_q_pre - 256;

	printf("calculateFreq : freq_offset_i = %d , freq_offset_q = %d , freq_offset_i_pre = %d , freq_offset_q_pre = %d \n",
			freq_offset_i, freq_offset_q , freq_offset_i_pre , freq_offset_q_pre);

	double temp_1 = atan((freq_offset_q * 1.0)/(freq_offset_i * 1.0));
	double temp_2 = atan((freq_offset_q_pre * 1.0)/(freq_offset_i_pre * 1.0));
	double diff = temp_1 - temp_2;
	double result = (diff / 5529.203) * 1000000000;
	return result;
	
	//freq = (atan(freq_offset_q/freq_offset_i)-atan(freq_offset_q_pre/freq_offset_i_pre))/(2*PI*55*2*10-9*8)  (Hz)
}


double calculateFreq_old(char* ret){ // 0x04FF04FF , 0x04FF0400 , 0x04000400
	int freq_offset_i = 0, freq_offset_q =0, freq_offset_i_pre=0, freq_offset_q_pre=0;

	char temp[5];
	temp[0] = '0'; temp[1] = 'x'; temp[4] = '\0';
	int index = 2;
	
	temp[2] = ret[index] ; index = index + 1;
	temp[3] = ret[index] ; index = index + 1;
	freq_offset_i = stringToInt(temp);
	printf("str = %s , freq_offset_i = %d  \n" , temp,freq_offset_i);

	temp[2] = ret[index] ; index = index + 1;
	temp[3] = ret[index] ; index = index + 1;
	freq_offset_q = stringToInt(temp);
	printf("str = %s , freq_offset_q = %d  \n" , temp,freq_offset_q);

	temp[2] = ret[index] ; index = index + 1;
	temp[3] = ret[index] ; index = index + 1;
	freq_offset_i_pre = stringToInt(temp);
	printf("str = %s , freq_offset_i_pre = %d  \n" , temp,freq_offset_i_pre);

	temp[2] = ret[index] ; index = index + 1;
	temp[3] = ret[index] ; index = index + 1;
	freq_offset_q_pre = stringToInt(temp);
	printf("str = %s , freq_offset_q_pre = %d  \n" , temp,freq_offset_q_pre);

	printf("calculateFreq_old ------- : freq_offset_i = %d , freq_offset_q = %d , freq_offset_i_pre = %d , freq_offset_q_pre = %d \n",
		freq_offset_i, freq_offset_q , freq_offset_i_pre , freq_offset_q_pre);

	if(freq_offset_i_pre > 127)
		freq_offset_i_pre = freq_offset_i_pre - 256;

	if(freq_offset_q > 127)
		freq_offset_q = freq_offset_q - 256;

	if(freq_offset_i > 127)
		freq_offset_i = freq_offset_i - 256;

	if(freq_offset_q_pre > 127)
		freq_offset_q_pre = freq_offset_q_pre - 256;

	printf("calculateFreq_old : freq_offset_i = %d , freq_offset_q = %d , freq_offset_i_pre = %d , freq_offset_q_pre = %d \n",
		freq_offset_i, freq_offset_q , freq_offset_i_pre , freq_offset_q_pre);
	double temp_1 = atan((freq_offset_q * 1.0)/(freq_offset_i * 1.0));
	double temp_2 = atan((freq_offset_q_pre * 1.0)/(freq_offset_i_pre * 1.0));
	double diff = temp_1 - temp_2;
	double result = (diff / 5529.203) * 1000000000;
	return result;
	
	//freq = (atan(freq_offset_q/freq_offset_i)-atan(freq_offset_q_pre/freq_offset_i_pre))/(2*PI*55*2*10-9*8)  (Hz)
}





int main()
{
	char* temp = "0x11c02cce";
	printf("temp = %s , result = %f , old = %f  \n", temp , calculateFreq(temp) , calculateFreq_old(temp));


	return 0;
}

/*

ret = 0x13bf2acc
freq_offset = -71444.117500 
ret = 0x13bf29cc
freq_offset = -69319.359770 
ret = 0xcbd26ca
freq_offset = -78850.779023 
ret = 0xebf27cb
freq_offset = -76365.267738 
ret = 0x12bf2acd
freq_offset = -75738.074457 
ret = 0x11c02cce
freq_offset = -83560.907685 
ret = 0x12bf29cc
freq_offset = -71893.252768 
ret = 0x15c02acd
freq_offset = -67255.377494 
ret = 0xebd27ca
freq_offset = -75868.747008 
ret = 0x12be29cc
freq_offset = -72598.806635 
*/





