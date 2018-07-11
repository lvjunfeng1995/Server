#ifndef _SERVER_H
#define _SERVER_H

#ifdef __cplusplus
extern "c"{
#endif

#include "server_type.h"	
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fctnl.h>
#include <unistd.h>

#define   WORKER_THREAD_NUM     5

#define   min(a,b) ((a <= b) ? (a) : (b))

	extern bool Stop;

	extern pthread_mutex_t Accept_mutex;
	extern pthread_mutex_t Worker_mutex;

	extern pthread_cond_t Accept_cond;
	extern pthread_cond_t Worker_cond;

	extern pthread_t AcceptThread_id;
	extern pthread_t WorkerThread_id[WORKER_THREAD_NUM];

	extern INT32 listen_fd;
	extern INT32 epoll_fd;

#ifdef __cplusplus
}
#endif

#endif
