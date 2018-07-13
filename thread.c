#include "server.h"

void release_client(int clientfd);
INT32 getClientFd();
INT32 setClientFd();

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

		printf("new client connected,ip: %s port: %i",inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));
		
		INT32 oldflag = fctnl(newdf,F_GETFL,O);
		INT32 newflag = oldflag | O_NONBLOCK;

		if(-1 == fctnl(newfd,F_SETFL,newflag))
		{
			printf("fctnl error");

			close(newfd);

			continue;
		}

		struct epoll_event e;

		memset(&e,0,sizeof(e));
		e.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
		e.data.fd = newfd;

		if(-1 == epoll_ctl(epollfd,EPOLL_CTL_ADD,newfd,&e))
		{
			printf("epoll_ctl error");

			close(newfd);
		}
	}

	return NULL;
}




void* worker_thread_func(void* arg)
{
	while(!Stop)
	{
		pthread_mutex_lock(&Worker_mutex);
		
		while(!Fd_List.NumActiveFd)
			pthread_cond_wait(&Worker_cond,&Worker_mutex);
		
		int clientfd = getClientFd();

		pthread_mutex_unlock(&Worker_mutex);
		
		INT8 buf[256];
		bool bError = false;

		while(true)
		{
			memset(buf,0,sizeof(buf));

			INT32 nRcv = recv(clientfd,buf,sizeof(buf),0);

			if(-1 == nRcv)
			{
				if(EWOULDBLOCK == errno)
					break;
				else
				{
					printf("recv error,client disconnected,fd = %i",clientfd);

					release_client(clientfd);
					bError = true;

					break;
				}
			}
			//client close socket,so server close socket also
			else if(0 == nRcv)
			{
				printf("peer closed,client disconnected,fd = %i",clientfd);

				release_client(clientfd);
				bError = true;

				break;
			}

			printf("server recv data: %s",buf);
		}

		if(bError)
			continue;
	}
}

void release_client(int clientfd)
{
	if(-1 == epoll_ctl(epollfd,EPOLL_CTL_DEL,clientfd,NULL))
		printf("release client socket failed as call epoll_ctl failed");

	close(clientfd);
}

INT32 getClientFd()
{
	if(0 == Fd_List.NumActiveFd)
	{
		printf("GetClientFd error!");

		return -1;
	}

	INT32 newfd = Fd_List.FdPtr[Fd_List.NumActiveFd - 1];

	Fd_List.NumActiveFd --;
	Fd_List.NumFreeMem  ++;

	return newfd;
}


INT32 setClientFd(INT32 newfd)
{
	if(0 == Fd_List.NumFreeMem)
	{
		printf("SetClientFd error!")

		return -1;
	}

	Fd_List.NumActiveFd ++;
	Fd_List.NumFreeMem  --;

	Fd_List.FdPtr[Fd_List.NumActiveFd - 1] = newfd;

	return 0;
}


