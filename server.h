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
#include <stdlib.h>
#include <mysql/mysql.h>
#include <signal.h>
	
#define   WORKER_THREAD_NUM     5
#define   EPOLL_EVENT_NUM       1024
#define   ACCEPT_THREAD_NUM     1
#define   MAX_USER_NUM          1024
#define   MAX_IOVEC_NUM			5
#define   BUF_SIZE				1024
#define   DBCONNECT_NUM			3
#define	  SLOT_NUM				60
#define	  SLOT_INTERVAL			1
#define   false                 0
#define   true                  1

	typedef struct nd_sv_user		ND_SV_USER;
	typedef struct nd_sv_q			ND_SV_Q;
	typedef struct nd_sv_dbc		ND_SV_DBC;
	typedef struct nd_sv_dbc_pool	ND_SV_DBC_POOL;
	typedef struct nd_sv_timer		ND_SV_TIMER;
	typedef struct nd_sv_wheel		ND_SV_WHEEL;
	
		
	struct nd_sv_user
	{
		INT32			 Fd;

		INT8			 *Readbuf;

		struct iovec	 *Readiov;

		INT8U			 NumReadiov;

		INT8			 *Writebuf;

		struct iovec	 *Writeiov;

		INT8U			 NumWriteiov;

		ND_SV_USER		 *NextPtr;

		ND_SV_TIMER		 *Timer;

	};

	struct nd_sv_q
	{
		ND_SV_USER		*InPtr;

		ND_SV_USER		*OutPtr;

		INT32U			NumEntries;
	};

	struct nd_sv_dbc_pool
	{
		ND_SV_DBC		*NextPtr;

		INT8			NbrFree;

		INT8			NbrUsed;
	};

	struct nd_sv_dbc
	{
		MYSQL			*DBC;

		MYSQL_RES		*Res;

		MYSQL_ROW		Row;

		ND_SV_DBC		*NextPtr;
	};

	struct nd_sv_timer
	{
		INT32			Rotation;

		INT32			Time_Slot;

		ND_SV_USER		*User_Data;

		ND_SV_TIMER		*NextPtr;

		ND_SV_TIMER		*PrevPtr;

		void(*Cb_Func)(ND_SV_USER*);
	};

	struct nd_sv_wheel
	{
		ND_SV_TIMER**	SlotArrayPtr;

		INT32			Cur_Slot;
	};

	extern bool Stop;

	extern pthread_mutex_t	 Accept_mutex;
	extern pthread_mutex_t	 Worker_mutex;
	extern pthread_mutex_t	 DBPool_mutex;
	extern pthread_mutex_t	 Wheel_mutex;

	extern pthread_cond_t	 Accept_cond;
	extern pthread_cond_t	 Worker_cond;
	extern pthread_cond_t	 DBPool_cond;

	extern pthread_t	 AcceptThread_id[ACCEPT_THREAD_NUM];
	extern pthread_t	 WorkerThread_id[WORKER_THREAD_NUM];

	extern INT32		 listen_fd;
	extern INT32 		 epoll_fd;
	
	extern INT32U		 ListenSemCtr;

	extern ND_SV_USER	 User[MAX_USER_NUM];

	extern ND_SV_Q		 TaskQue;

	extern struct iovec	 ReadIovs[MAX_USER_NUM][MAX_IOVEC_NUM];
	extern struct iovec  WriteIovs[MAX_USER_NUM][MAX_IOVEC_NUM];

	extern INT8			 ReadBuf[MAX_USER_NUM][BUF_SIZE];
	extern INT8			 WriteBuf[MAX_USER_NUM][BUF_SIZE];

	extern ND_SV_DBC_POOL	DBConnect_pool;

	extern ND_SV_WHEEL		TimeWheel;

	extern INT32			pipefd[2];
	

	extern void	       *accept_thread_func   (void*);

	extern void		   *worker_thread_func   (void*);

	extern bool		   create_server_listener(INT8*);

	extern void		   cond_mutex_init       (void);

	extern void		   thread_init           (void);

	extern void        sign_init             (void);
	
	extern void        TaskQ_init            (void);

	extern bool		   ND_SVSemPost		     (void);

	extern ND_SV_USER* ND_SVTaskPend	     (void);

	extern bool		   ND_SVSemPend	   	   	(void);
	
	extern void        ND_SVFdInit			(ND_SV_USER* const,INT32);

	extern bool        ND_SVTaskPost        (INT32);

	extern bool        RecvFromClient       (ND_SV_USER* const);

	extern void        SendToDB				(ND_SV_USER* const);

	extern void        WriteToClient        (ND_SV_USER* const);

	extern bool		   DbConnect_init       (INT8 *address,
 			                                 INT8 *username,
											 INT8 *password,
											 INT8 *dbname);

	extern void		   release_client	    (INT32);

	extern void		   tick					(void);

	extern void		   timer_handler		(void);

	extern ND_SV_TIMER*	 add_timer			(INT32);

	extern void		   del_timer			(ND_SV_TIMER*);

	extern void		   wheel_init			(void);

	extern void		   prog_exit			(void);

	extern void		   daemon_run			(void);

	

#ifdef __cplusplus
}
#endif

#endif
