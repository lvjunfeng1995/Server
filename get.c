/*************************************************************************
    > File Name: get.c
    > Author: ma6174
    > Mail: ma6174@163.com 
    > Created Time: Thu Jul 19 11:06:41 2018
 ************************************************************************/

#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>

int main()
{
	/*struct addrinfo *p,*listp,hints;

	char buf[128];
	char servbuf[128];

	int rc,flags;

	memset(&hints,0,sizeof(struct addrinfo));

	hints.ai_family = AF_INET;	
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;  //used listen and using ip address
	hints.ai_flags = AI_NUMERICSERV;              //using port number
	
	if((rc = getaddrinfo(NULL,"12345",&hints,&listp)) != 0)
	{
		printf("getaddrinfo error %s",gai_strerror(rc));

		return 0;
	}

	flags = NI_NUMERICHOST;

	for(p = listp;p; p = p ->ai_next)
	{
		getnameinfo(p->ai_addr,p->ai_addrlen,buf,128,servbuf,128,flags);

		printf("%s\n",buf);
		printf("server:%s\n",servbuf);
	}

	freeaddrinfo(listp);*/

	char* str = "abcdeaasss";
	char s = 'd';

	int a = sizeof(str);
	int b = strlen(str);

	printf("%s\n%i\n%i\n",str,a,b);

	return 0;
}
