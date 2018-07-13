#ifndef _SERVER_H
#define _SERVER_H

#ifdef __cplusplus
extern "C"{
#endif

#include "server_type.h"	
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>

#define   WORKER_THREAD_NUM     5

#define   EPOLL_EVENT_NUM       256

#define   min(a,b) ((a <= b) ? (a) : (b))

	typedef struct clientfd_list Clientfd_List;

	struct clientfd_list
	{
		INT32*		FdPtr;

		INT32U		NumActiveFd;

		INT32U		NumFreeMem;
	};		

	extern bool Stop;

	extern pthread_mutex_t	 Accept_mutex;
	extern pthread_mutex_t	 Worker_mutex;

	extern pthread_cond_t	 Accept_cond;
	extern pthread_cond_t	 Worker_cond;

	extern pthread_t	 AcceptThread_id;
	extern pthread_t	 WorkerThread_id[WORKER_THREAD_NUM];

	extern INT32		 listen_fd;
	extern INT32 		 epoll_fd;

	extern Clientfd_List	 FdList;

#ifdef __cplusplus
}
#endif

#endif
