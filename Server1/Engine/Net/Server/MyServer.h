#pragma once
#include "mynet.h"
#include "MyPool.h"
#include "MyConnection.h"
#include "mythread.h"
#include <map>
#include <string>
template<typename CONNECTION>
class MyServer:public mynet::stGetPackage,public thread::Thread{
public:
	virtual void beInit() {};
	virtual void beFinal(){};
	void init(const char*ip,unsigned short port)
	{
		mynet::Server server(ip,port);
		for (int i =0;i < 10;i++)
		thePool.bindEvent(&server,mynet::ACCEPT_EVT);
		stop = false;
	}
	void final()
	{
		stop = true;
	}
	MyServer()
	{
		stop = false;
		nodeType = 0;
	}
	void go()
	{
		run();
	}
	bool stop;
	void run()
	{
		beInit();
		while (!stop)
		{
			mynet::EventBase *evt = thePool.pullEvent();
			if (evt)
			{
				if (evt->isAccept()) 
				{
					CONNECTION *conn =  new CONNECTION();
					conn->setHandle(evt->getPeerHandle());
					thePool.bindEvent(conn,mynet::IN_EVT|mynet::OUT_EVT);
					conn->beInit();
					thePool.add(conn);
					continue;
				}
				if (evt->isErr())
				{
					CONNECTION *conn = (CONNECTION*) evt->target;
					if (conn)
					{
						//printf("删除网络连接%p\n",evt->target);
						conn->destroy();
						if (conn->beFinal())
						{
							
						}
						thePool.remove(conn);
					}
					
					continue;
				}
				if (evt->isOut()) 
				{
					mynet::Connection *conn = (mynet::Connection*) evt->target;
					if (conn)
					{
						conn->doSend(evt);
					}
					//printf("out");
				}
				if (evt->isIn()) 
				{
					mynet::Connection *conn = (mynet::Connection*) evt->target;
					if (conn)
					{
						conn->doRead(evt,this);
					}
					//printf("in");
				}
				
			}
		}
		beFinal();
	}
	/**
	 * 文件需要请求两次才能请求到实际数据
	 */
	virtual void doGetCommand(mynet::Target *target,void *cmd,unsigned int len)
	{
		MyConnection *conn = static_cast<MyConnection*>(target);
		if (conn)
		{
			conn->doCmd(cmd,len);
		}
	}
	unsigned int nodeType;
};

class Logic{
public:
	std::string className;
	virtual void __init__() = 0;
	
	net::Connection * getConnection()
	{
		return conn;
	}
	net::Connection *conn;
	Logic()
	{
		conn = NULL;
	}
};

class Logics:public LogicCenter{
public:
	virtual void dispatch(MyConnection *connection,const std::string &className,char *cmd,unsigned int len)
	{
		LOGICS_ITER iter = logics.find(className);
		if (iter != logics.end())
		{
			iter->second->conn = connection;
			remote::call(iter->second,cmd,len);
		}
	}
	Logics()
	{
	
	}

	static Logics& getMe()
	{
		static Logics me;
		return me;
	}

	void setup()
	{
		thePool.init();
		theTick.start();
		init();
	}

	void init()
	{}
	std::map<std::string,Logic*> logics;
	typedef std::map<std::string,Logic*>::iterator LOGICS_ITER;
};

#define theLogics Logics::getMe()

struct AutoRun_NetLogic
{
	AutoRun_NetLogic(const std::string &name,Logic *logic)
	{
		theLogics.logics[name] = logic;
		logic->__init__();
	}
};


#define CLASS_MAP(NAME) AutoRun_NetLogic NAME##AutoRun_NetLogic(#NAME,new NAME()); void NAME::__init__()