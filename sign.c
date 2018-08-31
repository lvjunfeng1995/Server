/*************************************************************************
    > File Name: sign.c
    > Author: lvjunfeng
    > Mail: 249861170@qq.com 
    > Created Time: Fri Aug 24 16:03:38 2018
 ************************************************************************/

#include<stdio.h>
#include"server.h"
#include"assert.h"

INT32 pipefd[2];

INT32 setnonblocking(INT32 fd)
{
	INT32 old = fcntl(fd,F_GETFL);
	INT32 new = old | O_NONBLOCK;

	fcntl(fd,F_SETFL,new);

	return old;
}

void addfd(INT32 epollfd,INT32 fd)
{
	struct epoll_event event;

	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;

	epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
	
	setnonblocking(fd);
}

void sig_handler(INT32 sig)
{
	INT32 save_errno = errno;
	INT32 msg = sig;

	write(pipefd[1],(INT8*)&msg,1);
	
	errno = save_errno;
}

void addsig(INT32 sig)
{
	struct sigaction sa;

	memset(&sa,0,sizeof(sa));

	sa.sa_handler = sig_handler;
	sa.sa_flags |= SA_RESTART;
	sigfillset(&sa.sa_mask);

	assert(sigaction(sig,&sa,NULL) != -1);	
}

void timer_handler()
{
	tick();

	alarm(SLOT_INTERVAL);
}


void sign_init()
{

	wheel_init();

	assert(pipe(pipefd) != -1);

	setnonblocking(pipefd[1]);

	addfd(epoll_fd,pipefd[0]);

	addsig(SIGINT);
	addsig(SIGTERM);
	addsig(SIGALRM);

	signal(SIGCHLD,SIG_DFL);
	signal(SIGPIPE,SIG_IGN);
}

void prog_exit()
{
	Stop = true;

	epoll_ctl(epoll_fd,EPOLL_CTL_DEL,listen_fd,NULL);

	close(listen_fd);
	close(epoll_fd);

//	pthread_cond_destroy(&Accept_cond);
	pthread_mutex_destroy(&Accept_mutex);

//	pthread_cond_destroy(&Worker_cond);
	pthread_mutex_destroy(&Worker_mutex);

//	pthread_cond_destroy(&DBPool_cond);
	pthread_mutex_destroy(&DBPool_mutex);

	pthread_mutex_destroy(&Wheel_mutex);
}

void daemon_run()
{
	INT32 pid;

	signal(SIGCHLD,SIG_IGN);

	pid = fork();
	
	if(pid < 0)
	{
		exit(-1);
	}
	else if(pid > 0)
	{
		exit(0);
	}

	assert(setsid() != -1);

	INT32 fd;
	fd = open("/dev/null",O_RDWR,0);
	assert(fd != -1);

	if(fd != -1)
	{
		dup2(fd,STDIN_FILENO);
		dup2(fd,STDOUT_FILENO);
		dup2(fd,STDERR_FILENO);
	}

	if(fd > 2)
		close(fd);
}
