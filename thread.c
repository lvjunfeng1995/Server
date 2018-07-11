#include "server.h"

bool Stop = false;

void* accept_thread_func(void* arg)
{
	while(!Stop)
	{
		pthread_mutex_lock(&Accept_mutex);
		pthread_cond_wait(&Accept_cond,&Accept_mutex);

		struct sockaddr_in clientaddr;
		socklent_t addrlen;

		INT32 newfd = accept(listenfd,(struct sockaddr_in *)&clientaddr,&addrlen);

		pthread_mutex_unlock(&Accept_mutex);

		if(-1 == newfd)
			continue;
	}
}
