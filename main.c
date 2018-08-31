#include "server.h"

bool timeout = false;
INT32 main(INT32 argc,INT8 *argv[])
{
	INT8 *port = NULL;
	INT32 ch;

	bool bdaemon = false;

	if(argc < 2)
	{
		printf("please input the correct port number!");

		exit(0);
	}

	while(-1 != (ch = getopt(argc,argv,"p:d")))
	{
		switch(ch)
		{
			case 'd':

				bdaemon = true;

			case 'p':
				port = optarg;

				break;
		}
	}

	if(bdaemon)
		daemon_run();

	if( NULL == port )
	{
		port = "12345";
	}

	if(!create_server_listener(port))
	{
		printf("Unable to create listen server:port %s\n",port);

		return -1;
	}

	if(!DbConnect_init("127.0.0.1","ljf","123456","testdb"))
	{
		printf("Unable to connect mysql\n");

		return -1;
	}

	sign_init();

	TaskQ_init();

	cond_mutex_init();

	thread_init();

	alarm(SLOT_INTERVAL);

	while(!Stop)
	{
		struct epoll_event ev[EPOLL_EVENT_NUM];

		INT32 n = epoll_wait(epoll_fd,ev,EPOLL_EVENT_NUM,10);

		if(0 == n)
		{
			continue;
		}
		else if(n < 0)
		{
		//	printf("epoll_wait error!");

			continue;
		}

		//INT32 m = min(n,EPOLL_EVENT_NUM);

		INT32 i = 0;
		for(;i < n;i++)
		{
			if(ev[i].data.fd == listen_fd)
			{
			//	pthread_mutex_lock(&Accept_mutex);

			//	ND_SVSemPost();

				//pthread_cond_signal(&Accept_cond);

		//		pthread_mutex_unlock(&Accept_mutex);
				
				pthread_cond_signal(&Accept_cond);
			}
			else if(ev[i].data.fd == pipefd[0])
			{
				INT32 j = 0;
				INT8 signals[128];

				INT32 ret = read(pipefd[0],signals,sizeof(signals));

				if(ret < 1)
				{
					continue;
				}
				else
				{
					for(;j < ret;++j)
					{
						switch(signals[j])
						{
							case SIGALRM:
								timeout = true;
								break;

							case SIGINT:
								prog_exit();
								break;

							case SIGTERM:
								prog_exit();
								break;
						}
					}
				}
			}
			else
			{
 	 			pthread_mutex_lock(&Worker_mutex);

				ND_SVTaskPost(ev[i].data.fd);

				pthread_cond_signal(&Worker_cond);

				pthread_mutex_unlock(&Worker_mutex);

			}
		}

		if(timeout)
		{
			timer_handler();

			timeout = false;
		}

	}

	return 0;

}
