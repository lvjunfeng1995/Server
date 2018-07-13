#include "server.h"


bool create_server_listener(const INT8* ip,short port)
{
	
}

INT32 open_listenfd(INT8* port)
{
	struct addrinfo hints,* listp,*p;

	INT32 listenfd,optval = 1;

	//get a list of potential server address
	memset(&hints,0,sizeof(struct addrinfo));

	hints.ai_socktype = SOCK_STREAM;              //used stable connect
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;  //used listen and using ip address
	hints.ai_flags = AI_NUMERICSERV;              //using port number
	
	getaddrinfo(NULL,port,&hints,&listp);

	//walk the list for one that we can bind to
	for(p = listp ; p ; p = p -> ai_next)
	{
		if((listenfd = socket(p -> ai_family,p -> ai_socktype,p -> ai_protocol)) < 0)
			continue;

		setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR
			        ,(const void *)&optval,sizeof(INT32));
		setsockopt(listenfd,SOL_SOCKET,SO_REUSEPORT
				,(const void *)&optval,sizeof(INT32));
	}
}
