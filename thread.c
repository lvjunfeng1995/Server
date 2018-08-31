#include "server.h"

void release_client(int clientfd);
INT32 getClientFd();
INT32 setClientFd();

bool Stop = false;

void *accept_thread_func(void *arg)
{
	while(!Stop)
	{
		pthread_mutex_lock(&Accept_mutex);	
		
		pthread_cond_wait(&Accept_cond,&Accept_mutex);

	/*	while(!ND_SVSemPend())
		{
			pthread_cond_wait(&Accept_cond,&Accept_mutex);
		}*/

		struct sockaddr_in clientaddr;
		socklen_t addrlen;

		INT32 newfd = accept(listen_fd,(struct sockaddr *)&clientaddr,(socklen_t *)&addrlen);

		pthread_mutex_unlock(&Accept_mutex);

		if(-1 == newfd)
			continue;

		ND_SVFdInit(&User[newfd],newfd);

		printf("new client connected,ip: %s port: %i",inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));
		
	
	}

	return NULL;
}




void *worker_thread_func(void *arg)
{
	while(!Stop)
	{
		pthread_mutex_lock(&Worker_mutex);

		ND_SV_USER *user;
		
		while((user = ND_SVTaskPend()) == NULL)
			pthread_cond_wait(&Worker_cond,&Worker_mutex);

		pthread_mutex_unlock(&Worker_mutex);

		if(!RecvFromClient(user))
		{
			SendToDB(user);

			WriteToClient(user);
		}

	}
}








