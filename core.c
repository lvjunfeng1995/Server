/*************************************************************************
    > File Name: core.c
    > Author: lvjunfeng
    > Mail: 249861170@qq.com 
    > Created Time: Wed Aug  1 15:33:47 2018
 ************************************************************************/

#include<stdio.h>
#include"server.h"

void release_client(INT32 clientfd)
{
	if(-1 == epoll_ctl(epoll_fd,EPOLL_CTL_DEL,clientfd,NULL))
		printf("release client socket failed as call epoll_ctl failed");

	close(clientfd);
}

void modfd(INT32 epollfd,INT32 fd,INT32 ev)
{
	struct epoll_event event;

	event.data.fd = fd;
	event.events = ev | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;

	epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&event);
}

ND_SV_DBC* getDBHandle()
{
	pthread_mutex_lock(&DBPool_mutex);

	while(DBConnect_pool.NbrFree == 0)
	{
		pthread_cond_wait(&DBPool_cond,&DBPool_mutex);
	}

	ND_SV_DBC *db = DBConnect_pool.NextPtr;

	DBConnect_pool.NextPtr = db ->NextPtr;
	DBConnect_pool.NbrFree --;
	DBConnect_pool.NbrUsed ++;

	db ->NextPtr = NULL;

	pthread_mutex_unlock(&DBPool_mutex);

	return db;
}

bool putDBHandle(ND_SV_DBC *db)
{
	pthread_mutex_lock(&DBPool_mutex);

	db ->NextPtr = DBConnect_pool.NextPtr;

	DBConnect_pool.NextPtr = db;
	DBConnect_pool.NbrFree ++;
	DBConnect_pool.NbrUsed --;

	if(DBConnect_pool.NbrFree == 1)
	{
		pthread_cond_signal(&DBPool_cond);
	}

	pthread_mutex_unlock(&DBPool_mutex);

	return true;
}

bool ND_SVTaskPost(INT32 fd)
{
	if(NULL == TaskQue.OutPtr)
	{
		TaskQue.OutPtr = &User[fd];
		TaskQue.InPtr  = &User[fd];
		TaskQue.NumEntries = 1;

		return true;
	}

	TaskQue.InPtr ->NextPtr = &User[fd];
	TaskQue.InPtr = &User[fd];

	TaskQue.NumEntries ++;

	return true;
}

ND_SV_USER* ND_SVTaskPend()
{
	if(0 == TaskQue.NumEntries)
	{
		return NULL;
	}

	ND_SV_USER *user = TaskQue.OutPtr;

	TaskQue.OutPtr = TaskQue.OutPtr ->NextPtr;

	TaskQue.NumEntries --;

	return user;
}

bool ND_SVSemPost()
{
	ListenSemCtr ++;

	return true;
}

bool ND_SVSemPend()
{
	if(0 == ListenSemCtr)
	{
		return false;
	}

	ListenSemCtr --;

	return true;
}

void ND_SVFdInit(ND_SV_USER* const user,INT32 newfd)
{
	if(newfd >= MAX_USER_NUM)
	{
		close(newfd);
	}

		INT32 oldflag = fcntl(newfd,F_GETFL);
		INT32 newflag = oldflag | O_NONBLOCK;

		if(-1 == fcntl(newfd,F_SETFL,newflag))
		{
			printf("fctnl error");

			close(newfd);

			return;
		}

		struct epoll_event e;

		memset(&e,0,sizeof(e));
		e.events = EPOLLIN | EPOLLRDHUP | EPOLLET | EPOLLONESHOT;
		e.data.fd = newfd;

		if(-1 == epoll_ctl(epoll_fd,EPOLL_CTL_ADD,newfd,&e))
		{
			printf("epoll_ctl error");

			close(newfd);

			return;
		}

		ND_SV_TIMER* timer;

		if((timer = add_timer(SLOT_NUM * SLOT_INTERVAL)) == NULL)
		{
			printf("create timer error");

			release_client(newfd);

			return;
		}

		timer ->User_Data = user;
		user ->Timer = timer;

		user ->Fd = newfd;

		user ->Readbuf = &ReadBuf[newfd][0];
		memset(user ->Readbuf,0,BUF_SIZE);

		user ->Readiov = &ReadIovs[newfd][0];
		user ->NumReadiov = 0;

		user ->Writebuf = &WriteBuf[newfd][0];
		memset(user ->Writebuf,0,BUF_SIZE);

		user ->Writeiov = &WriteIovs[newfd][0];
		user ->NumWriteiov = 0;
		user ->NextPtr = NULL;
}

bool RecvFromClient(ND_SV_USER* const user)
{
		INT8  buf[256];
		INT8* Readbuf = user ->Readbuf;
		INT32 clientfd = user ->Fd;

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
					del_timer(user->Timer);
					bError = true;

					break;
				}
			}
			//client close socket,so server also close socket
			else if(0 == nRcv)
			{
  				printf("peer closed,client disconnected,fd = %i",clientfd);

				release_client(clientfd);
				del_timer(user->Timer);
				bError = true;

				break;
			}

			printf("%s\n",buf);
			strcat(Readbuf,buf);

		}

		if(bError)
		{
			return bError;
		}

		INT8* str = Readbuf;

		while((user ->Readiov[user ->NumReadiov].iov_base 
					= strtok(str,";")) != NULL)
		{
			user ->Readiov[user ->NumReadiov].iov_len = strlen(
					(INT8 *)user ->Readiov[user ->NumReadiov].iov_base);

			user ->NumReadiov ++;

			str = NULL;
		}
		printf("%i\n",user->NumReadiov);

		user->Timer->Rotation = 1;

		return bError;
}

void WriteToClient(ND_SV_USER* const user)
{
	INT32 clientfd = user ->Fd;
	
	bool bError = false; 

/*	if(user ->NumReadiov == 0)
	{
		memset(user ->Readbuf,0,BUF_SIZE);
		memset(user ->Writebuf,0,BUF_SIZE);

		modfd(epoll_fd,clientfd,EPOLLIN);

		return;
	}*/

	while(user ->NumWriteiov)
	{
		int nSent = send(clientfd,user ->Writeiov[user ->NumWriteiov - 1].iov_base,
				user ->Writeiov[user ->NumWriteiov - 1].iov_len,0);

		if(nSent == -1)
		{
			if(errno == EWOULDBLOCK)
				continue;
			else
			{
 			 	bError = true;

				break;
 			}
		}

		user ->NumWriteiov --;
	}

	if(bError)
	{
		release_client(clientfd);
		del_timer(user->Timer);

		return;
	}

	memset(user ->Readbuf,0,BUF_SIZE);
	memset(user ->Writebuf,0,BUF_SIZE);

	user ->NumReadiov = 0;
	user ->NumWriteiov = 0;

	modfd(epoll_fd,clientfd,EPOLLIN);
}

INT32 AddStrToWrite(INT8 *writebuf,
					ND_SV_USER * const user)
{
	INT32 writelen = strlen(writebuf);

	user ->Writeiov[user ->NumWriteiov].iov_base = writebuf;
	user ->Writeiov[user ->NumWriteiov].iov_len = writelen;

	user ->NumWriteiov ++;

	return writelen + 1;
}
void SendToDB(ND_SV_USER * const user)
{
	if(user ->NumReadiov == 0)
	{
		return;
	}

	ND_SV_DBC *dbc = getDBHandle();

	MYSQL *db = dbc ->DBC;
	MYSQL_RES *res;
	MYSQL_ROW row;

	INT32 offset = 0;

	INT8 *writebuf = user ->Writebuf;

	while(user ->NumReadiov)
	{
		INT8 *str = (INT8 *)user ->Readiov[user ->NumReadiov - 1].iov_base;
		INT32 strlen = user ->Readiov[user ->NumReadiov - 1].iov_len;

		if(mysql_real_query(db,str,strlen))
		{
			INT8 *err = (INT8 *)mysql_error(db);

			strcat(writebuf+offset,err);

			offset += AddStrToWrite(writebuf+offset,user);
		}
		else
		{
			res = mysql_store_result(db);

			if(res == NULL)
			{
	   			INT8 *str = "Successful operation!";

				strcat(writebuf+offset,str);

				offset += AddStrToWrite(writebuf+offset,user);
			}
			else
			{
 	 			INT32 columns = mysql_num_fields(res);
				INT32 rows = mysql_num_rows(res);

				INT32 i = 0;
				INT32 j = 0;

				for(;i < rows;i++)
				{
 					row = mysql_fetch_row(res);

					for(j = 0;j < columns;j++)
					{
 						strcat(writebuf+offset,row[j]);
						strcat(writebuf+offset,":");
					}

					strcat(writebuf+offset,";");
				}

				offset += AddStrToWrite(writebuf+offset,user); 

				mysql_free_result(res);
			} 
		}

		user ->NumReadiov --;
	}

	putDBHandle(dbc);

}
