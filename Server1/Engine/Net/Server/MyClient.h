#pragma once
#include "mynet.h"
#include "MyPool.h"
#include <map>
#include <sstream>
#include <string>
#include "MyConnection.h"
#include "MyTick.h"
#include "pointer.h"
class MyClient:public MyConnection{
public:
	MyClient(const char *ip,unsigned short port):MyConnection()
	{
		peerIp = ip;
		this->port = port;
		init(ip,port);
		go();
	}
	virtual void send(void *cmd,unsigned int len)
	{
		sendCmd(cmd,len);
	}
	void go()
	{
		thePool.bindEvent(this,mynet::IN_EVT | mynet::OUT_EVT);
	}
	void reconnect()
    {
        init(peerIp.c_str(), port);
		if (checkValid())
		{
			go();flushSend();
			errorTimeOutCount = 0;
		}
    }
    std::string peerIp;
    unsigned short port;
    void init(const char *ip,unsigned short port)
	{
		socket = mynet::Client::init(ip,port);
		beInit();
	}
    void close()
	{
		this->destroy();
		socket = -1;
		errorTimeOutCount = 0;
	}
	bool checkValid(){return socket != -1;}

	bool beFinal()
	{
		socket = -1;
		return false;
	}
	~MyClient()
	{
		printf("我卸载啦");
	}
}; 

class Clients:public MyTick{
public:	
	Clients():MyTick(2000)
	{
		theTick.addTick(this);
	}
	std::map<std::string,MyClient*> clients;
	typedef std::map<std::string,MyClient*>::iterator CLIENTS_ITER;
	/**
	 * 端口 port
	 */
	template<typename T>
	MyClient* getClient(const char *ip,unsigned short port)
	{
		std::stringstream ss;
		ss << ip <<":" << port;
		
		CLIENTS_ITER iter = clients.find(ss.str());
		if (iter != clients.end())
		{
			if (iter->second && !iter->second->checkValid())
				iter->second->reconnect();
			return iter->second;
		}
		T *client = new T(ip,port);
		clients[ss.str()] = client;
		return client;
	}
	/**
	 * 服务号
	 */
	MyClient* getClientByServerID(unsigned int serverID)
	{
		return NULL;
	}
	/**
	 * 快速引索号
	 */
	MyClient* getClientByIndex(unsigned int index)
	{
		return NULL;
	}
	static Clients&getMe()
	{
		static Clients clients;
		return clients;
	}
	void tick()
	{
		for (CLIENTS_ITER iter = clients.begin(); iter != clients.end();++iter)
		{
			MyClient *client = iter->second;
			if (client && !client->checkValid())
			{
				client->reconnect();
				printf("尝试重连\n");
			}
			if (client && client->checkValid())
			{
				if (client->errorTimeOutCount >= 10)
				{
					client->close();
					printf("超时关闭自身 %d",client->errorTimeOutCount);
				}
				else
				{
					client->errorTimeOutCount ++;
					//MyConnection::R_recvTick(client,client->errorTimeOutCount);
					//printf("发出时钟滴答\n");
				}
			}
		}
	}
};

#define theClients Clients::getMe()

template<typename T>
class URL{
public:
	static MyConnection * Get(const std::string &ip,unsigned short port)
	{
		return theClients.getClient<T>(ip.c_str(),port);
	}
};

template<>
class URL<MyClient>{
public:
	static MyConnection * Get(const std::string &ip,unsigned short port)
	{
		return theClients.getClient<MyClient>(ip.c_str(),port);
	}
};