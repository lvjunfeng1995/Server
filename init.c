#include "server.h"

INT32 epoll_fd;
INT32 listen_fd;

pthread_mutex_t Accept_mutex;
pthread_mutex_t Worker_mutex;
pthread_mutex_t DBPool_mutex;
pthread_mutex_t Wheel_mutex;

pthread_cond_t Accept_cond;
pthread_cond_t Worker_cond;
pthread_cond_t DBPool_cond;

pthread_t accept_thread_id[ACCEPT_THREAD_NUM] = {0};
pthread_t worker_thread_id[WORKER_THREAD_NUM] = {0};

INT32U ListenSemCtr = 0;

ND_SV_USER User[MAX_USER_NUM] = {0};

INT8 ReadBuf[MAX_USER_NUM][BUF_SIZE] = {0};
INT8 WriteBuf[MAX_USER_NUM][BUF_SIZE] = {0};

MYSQL		DBCs[DBCONNECT_NUM] = {0};
MYSQL_RES	Ress[DBCONNECT_NUM] = {0};
MYSQL_ROW	Rows[DBCONNECT_NUM] = {0};

ND_SV_DBC	DBC_struct[DBCONNECT_NUM] = {0};

ND_SV_DBC_POOL DBConnect_pool;

ND_SV_Q TaskQue;

ND_SV_WHEEL		TimeWheel;
ND_SV_TIMER*	Slots[SLOT_NUM] = {0};

struct iovec ReadIovs[MAX_USER_NUM][MAX_IOVEC_NUM] = {0};
struct iovec WriteIovs[MAX_USER_NUM][MAX_IOVEC_NUM] = {0};

INT32 open_listenfd(INT8*);


bool create_server_listener(INT8 *port)
{
   	listen_fd = open_listenfd(port);

	/*listen_fd = socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK,0);

	if(-1 == listen_fd)
	{
		return false;
	}

	INT32 on = 1;

	setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,(INT8 *)&on,sizeof(on));
	setsockopt(listen_fd,SOL_SOCKET,SO_REUSEPORT,(INT8 *)&on,sizeof(on));

	struct sockaddr_in servaddr;

	memset(&servaddr,0,sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("0.0.0.0");
	servaddr.sin_port = htons(12345);

	if(bind(listen_fd,(struct sockaddr *)&servaddr,sizeof(servaddr)) == -1)
		return false;

	if(listen(listen_fd,50) == -1)
		return false;*/

	if(listen_fd < 0 )
		return false;

	epoll_fd = epoll_create(1);
	
	if(-1 == epoll_fd)
	{
		return false;
	}	

	struct epoll_event e;

	memset(&e,0,sizeof(e));

	e.events = EPOLLIN | EPOLLRDHUP;
	e.data.fd = listen_fd;

	if(-1 == epoll_ctl(epoll_fd,EPOLL_CTL_ADD,listen_fd,&e))
		return false;

	return true;
}

void cond_mutex_init(void)
{
	pthread_cond_init(&Accept_cond,NULL);
	pthread_mutex_init(&Accept_mutex,NULL);

	pthread_cond_init(&Worker_cond,NULL);
	pthread_mutex_init(&Worker_mutex,NULL);

	pthread_cond_init(&DBPool_cond,NULL);
	pthread_mutex_init(&DBPool_mutex,NULL);

	pthread_mutex_init(&Wheel_mutex,NULL);

}

void thread_init(void)
{
	INT32 i = 0;

	for(;i < ACCEPT_THREAD_NUM;i++)
	{
		pthread_create(&accept_thread_id[i],NULL,accept_thread_func,NULL);
	}

	i = 0;

	for(;i < WORKER_THREAD_NUM;i++)
	{
		pthread_create(&worker_thread_id[i],NULL,worker_thread_func,NULL);
	}
}

INT32 open_listenfd(INT8 *port)
{
	struct addrinfo hints,*listp,*p;

	INT32 listenfd,optval = 1;

	//char buf[128];
	//char ser[128];

	//get a list of potential server address
	memset(&hints,0,sizeof(struct addrinfo));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;              //used stable connect
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;  //used listen and using ip address
	hints.ai_flags = AI_NUMERICSERV;              //using port number
	
	INT32 err = getaddrinfo("0.0.0.0",port,&hints,&listp);
	
	if(err != 0)
	{
		if(err == EAI_FAIL || err == EAI_AGAIN)
		{
			printf("Name server cannot be used,exiting!\n");
		}
		printf("err = %d\n",err);
		printf("getaddrinfo error:%s\n",gai_strerror(err));

		return -1;
	}

	//walk the list for one that we can bind to
	for(p = listp ; p ; p = p -> ai_next)
	{
		if((listenfd = socket(p -> ai_family,p -> ai_socktype|SOCK_NONBLOCK,p -> ai_protocol)) < 0)
			continue;

		/*if(-1 == fcntl(listen_fd,F_SETFL,fcntl(listen_fd,F_GETFL)|O_NONBLOCK))
		{
			printf("fctnl error");

			close(listen_fd);

			continue;
		}*/
		
		setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR             //elminates 'Address already in use' error from bind           
			        ,(const void *)&optval,sizeof(INT32));
		setsockopt(listenfd,SOL_SOCKET,SO_REUSEPORT		//be binded by other process
				,(const void *)&optval,sizeof(INT32)); 

		if(bind(listenfd,p -> ai_addr,p -> ai_addrlen) == 0)
		{	
			break;
		}

		
		close(listenfd);
	//	getnameinfo(p->ai_addr,p->ai_addrlen,buf,128,ser,128,NI_NUMERICHOST);

	//	printf("buf:%s ser:%s\n ",buf,ser);
	}

	freeaddrinfo(listp);

	if(!p)
		return -1;

	if(listen(listenfd,MAX_USER_NUM) < 0)
	{
		close(listenfd);

		return -1;
	}

	return listenfd;
}


void TaskQ_init()
{
	TaskQue.InPtr      = NULL;
	TaskQue.OutPtr     = NULL;
	TaskQue.NumEntries = 0;
}

bool DbConnect_init(INT8 *address,
		            INT8 *username,
					INT8 *password,
					INT8 *dbname)
{
	int i = 0;

	for(;i < DBCONNECT_NUM;i ++)
	{
		mysql_init(&DBCs[i]);

		if(!mysql_real_connect(&DBCs[i],address,username,password,dbname,0,NULL,0))
		{
			return false;
		}

		DBC_struct[i].DBC = &DBCs[i];
		DBC_struct[i].Res = &Ress[i];
		DBC_struct[i].Row = Rows[i];

		if(i < DBCONNECT_NUM -1)
		{
			DBC_struct[i].NextPtr = &DBC_struct[i+1];
		}
		else
		{
			DBC_struct[i].NextPtr = NULL;
		}
	}

	DBConnect_pool.NextPtr = &DBC_struct[0];
	DBConnect_pool.NbrFree = DBCONNECT_NUM;
	DBConnect_pool.NbrUsed = 0;
	
	return true;

}

void wheel_init()
{
	TimeWheel.Cur_Slot = 0;
	TimeWheel.SlotArrayPtr = Slots;
}

