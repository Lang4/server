#include "connect.h"
#include "win32helper.h"
namespace mynet{
	void Connection::destroy()
	{
		Target::destroy();
		closesocket(socket);
	}
	Connection:: Connection()
	{
		directDealCmd = true;
	}
	 unsigned long Connection::getHandle(){return (unsigned long)socket;}
	 void Connection::setHandle(unsigned long socket){this->socket = socket;}
	 
	 /**
	  *  
	  */
	 void Connection::sendCmd(void *cmd,unsigned int len)
	 {
		if (checkValid())
		{
			Decoder  decoder;
			decoder.encode(cmd,len);
			sends.write(decoder.getRecord());

			if (outEvt)
				doSend(outEvt,false);
		}
	 }
	 void Connection::recvCmdCallback(void *cmd,unsigned int len)
	 {
		//  
	 }
	 /** 
	  * 
	  */
	 unsigned int Connection::getCmd(void *cmd,unsigned int len)
	 {
		return decoder.decode(this,cmd,len);
	 }
	 /**
	  *  
	  **/
	 unsigned int Connection::recv(void *cmd,unsigned int size)
	 {
		unsigned int realcopy = 0;
	//	while (recvs.size())
		while (!recvs.empty())
		{
			Record *record = NULL;
			if (recvs.readOnly(record))
			{
				realcopy = record->recv(cmd,size);
				if (record->empty())
				{
					delete record;
					recvs.pop();
				}
				if (realcopy == size)
				{
					return size;
				}
			}else break;
		}
		return realcopy;
	}
		
    /**
     * Client
     **/
    int Client::init(const char *ip,unsigned short port)
    {
        SOCKET socket = (SOCKET)WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);//::socket(AF_INET,SOCK_STREAM,0);
        if(socket == -1)
        {
            // TODO error
        }
        struct sockaddr_in addrServer;

        memset(&addrServer,0,sizeof(sockaddr_in));
        addrServer.sin_family = AF_INET;
        addrServer.sin_addr.s_addr = inet_addr(ip);
        addrServer.sin_port = htons(port);
        
        if(connect(socket,(const struct sockaddr *)&addrServer,sizeof(sockaddr)) != 0)
        {
            // TODO error
            socket = -1;
        }
       // setnonblock(socket);
		return socket;
    }
    void Client::close()
    {
        
        socket = -1;
    }
    /**
     * Server
     */
	Server::Server(const char *ip,WORD port)
	{
		init(ip,port);
	}
	void Server::init(const char *ip,WORD port)
	{
		struct sockaddr_in ServerAddress;

		//  

		socket = (unsigned int)WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

		//  
		ZeroMemory((char *)&ServerAddress, sizeof(ServerAddress));
		ServerAddress.sin_family = AF_INET;
		//ServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);                      
		ServerAddress.sin_addr.s_addr = inet_addr(ip);         
		ServerAddress.sin_port = htons(port);                          

		//  
		if (SOCKET_ERROR == ::bind((SOCKET)socket, (struct sockaddr *) &ServerAddress, sizeof(ServerAddress))) 
		{
			return;
		}
		else
		{
		}

		//  
		if (SOCKET_ERROR == listen((SOCKET)socket,SOMAXCONN))
		{
			return;
		}
		else
		{
		}
	}
};