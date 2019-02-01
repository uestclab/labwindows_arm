#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#define ARSIZE 1024


int main()
{
	char numbers[ARSIZE] = { 0 };
	const char *file = "numbers.text";
	int i;
	long time1, time2, time3;
	FILE *iofile;
	int err;

	for (i = 0; i < ARSIZE; i++)
		numbers[i] = i;
	if ((iofile = fopen(file, "wb")) ==NULL){
		printf("Could not open %s for output.\n", file);
		exit(EXIT_FAILURE);
	}
	else
		printf("The file %s was opened\n",file);

	time1 = clock();
	for (int j = 0; j < 1024*1024*10;j++)
		fwrite(numbers, sizeof(char), ARSIZE, iofile);
	time2 = clock();

	time3 = time2 - time1;

	printf("all time:%d", time3);
	fclose(iofile);


	return 0;
}

