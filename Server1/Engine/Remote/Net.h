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
	PARSE_CMD doParseCmd; // ������Ϣ

	typedef Client* (*NEW_CLIENT)(LTCPClientTaskPool *clientPool,const std::string &ip,unsigned short port);
	NEW_CLIENT doNewClient; // ����ͻ���
	
	typedef LTCPTask* (*NEW_TASK)(LTCPTaskPool *taskPool,int sock,const struct sockaddr_in*);
	NEW_TASK doNewTask; // ����ͻ���

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
	 * ����Ϣ�ṹ
	 */
	std::vector<msg::function*> functions; // ��������
	msg::function * defaultFunction; // Ĭ�ϴ�����
	void bind(int ev,msg::function * function); // ������
	void call(int ev,remote::Connection* conn,void *cmd,int len); // ����
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