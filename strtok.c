/*************************************************************************
    > File Name: strtok.c
    > Author: lvjunfeng
    > Mail: 249861170@qq.com 
    > Created Time: Mon Aug  6 09:10:19 2018
 ************************************************************************/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>

int main(int argc,char *argv[])
{
	if(argc < 2)
		return 0;

	char *str = (char *)malloc(sizeof(char)*256);

	strcat(str,argv[1]);
	strcat(str,argv[2]);
	strcat(str,argv[3]);
	//char *str = "123;123;12345";

	int num = 0;

//	char *temp;

/*	while((temp = strtok(str,";") )!= NULL)
	{
		num ++;

		str = NULL;

		printf("%s,%i\n",temp,num);
	}
*/		
	char *temp = strtok(str,";");

	while(temp)
	{
		num ++;

		printf("%s\n",temp);

		temp = strtok(NULL,";");
	}

	free(str);

	return 0;
}
