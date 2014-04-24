#pragma once
#include "RemoteObject.h"
#include "message.h"
class LTCPTask;
class LTCPClientTaskPool;
class LTCPTaskPool;
class Client:public remote::Connection{
public:
	Client(){}
};
class Server{
public:
	virtual Client * getClient(const char *ip,unsigned short port) = 0;
	virtual bool init(unsigned short port) = 0;
	virtual void go() = 0;
};
extern Server * GetServer();

class Events{
public:
	typedef void (*PARSE_CMD)(remote::Connection*,void*,int);
	PARSE_CMD doParseCmd; // 处理消息

	typedef Client* (*NEW_CLIENT)(LTCPClientTaskPool *clientPool,const std::string &ip,unsigned short port);
	NEW_CLIENT doNewClient; // 处理客户端
	
	typedef LTCPTask* (*NEW_TASK)(LTCPTaskPool *taskPool,int sock,const struct sockaddr_in*);
	NEW_TASK doNewTask; // 处理客户端

	typedef void (*RE_CONNECT)(Client *client);
	RE_CONNECT doReconnect;
	Events()
	{
		doParseCmd = NULL;
		doNewClient = NULL;
		doNewTask = NULL;
		doReconnect = NULL;
	}
	static Events& getMe()
	{
		static Events es; return es;
	}
	/**
	 * 单消息结构
	 */
	std::vector<msg::function*> functions; // 函数集合
	msg::function * defaultFunction; // 默认处理函数
	void bind(int ev,msg::function * function); // 绑定数据
	void call(int ev,remote::Connection* conn,void *cmd,int len); // 调用
	msg::function * getFunction(int ev);
};

extern Events* GetEvents();

#define EVENTS_MAP(CLASS)\
class EVNTS_MAP##CLASS:public InitEventFunction{\
	void init();\
};\
AutoRunEvents auto_run_events##CLASS(new EVNTS_MAP##CLASS);\
void EVNTS_MAP##CLASS::init()\

#define EVENT(__id__,__function__) GetEvents()->bind(__id__,msg::bind(&__function__));


class InitEventFunction{
public:
	virtual void init()  = 0;
	virtual ~InitEventFunction(){}
};

class AutoRunEvents{
public:
	InitEventFunction *func;
	AutoRunEvents(InitEventFunction *func)
	{
		if (func) func->init();
		this->func = func;
	}
	~AutoRunEvents()
	{
		if (this->func) delete this->func;
		this->func = NULL;
	}
};