#pragma once
#include "mynet.h"
#include "network.h"
#include "MyTick.h"
class MyConnection;
class LogicCenter{
public:
	virtual void dispatch(MyConnection *connection,const std::string &className,char *cmd,unsigned int len) = 0;
};
class MyConnection:public mynet::Connection,public net::Task<MyConnection>,public TickLogic<MyConnection>{
public:
	MyConnection():net::Task<MyConnection>(this),TickLogic<MyConnection>(100)
	{
		errorTimeOutCount = 0;
		logics = NULL;
		addTickLogic(2000,&MyConnection::doTick);
		BIND_NET_FUNCTION(MyConnection,recvTick);
	}
	LogicCenter *logics;
	virtual void send(void *cmd,unsigned int len)
	{
		sendCmd(cmd,len);
	}
	virtual void beInit()
	{
	
	}
	virtual bool beFinal(){return true;}

	REMOTE_CLASS(MyConnection);
	REMOTE_FUNCTION_1(recvTick,unsigned int)
	{
		if (this->checkValid())
			printf("时钟滴答确定网络活跃\n");
		errorTimeOutCount = 0;
	}
	unsigned int errorTimeOutCount;

	mynet::MyList<std::string> buffers;


	void doCmd(void*cmd,unsigned int len)
	{
		std::string className = remote::getClass(cmd,len);
		if(className == "MyConnection")
		{
			remote::call(this,(char*) cmd,len);
		}
		else
		{
			std::string temp;
			temp.resize(len);
			memcpy(&temp[0],cmd,len);
			
			buffers.write(temp);
		}
	}

	void tick()
	{
		TickLogic<MyConnection>::tick();
		std::string temp;
		while(buffers.readOnly(temp))
		{
			if (temp.size())
			{
				std::string className = remote::getClass(temp);
				if (logics)
					logics->dispatch(this,className,(char*)(&temp[0]),temp.size());
			}
			buffers.pop();
		}
	}
	
	void doTick()
	{
		MyConnection::R_recvTick(this,errorTimeOutCount);
		printf("Connection 时钟滴答\n");
	}
	void registerLogics(LogicCenter *l)
	{
		logics = l;
	}
};
/**
 * 子类对象在记录Connection 时使用index 记录
 */
class Connections{
public:

};

