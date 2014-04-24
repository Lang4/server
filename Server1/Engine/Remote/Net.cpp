#include "Net.h"
#include "LNetService.h"
#include "LTCPClientTaskPool.h"
#include "LTCPClientTask.h"
#include "LTCPTask.h"
#include "LTCPTaskPool.h"
class LClient : public LTCPClientTask,public Client
{
    public:
		LClient(const char *ip,unsigned short port)
            :LTCPClientTask(ip,port){}
	bool msgParse(const void*cmd, const unsigned int len)
	{
		if (GetEvents()->doParseCmd)
		{
			(*(GetEvents()->doParseCmd))(this,(void*)cmd,len);
		}
		else theAllObj.get(remote::CONNECTION)->parse(this,(void*)cmd,len);
		printf("Client::msgParse\n");
		return true;
	}
	void sendObject(remote::Object *object)
	{
		std::string content = object->toString();
		if (!sendCmd(content.c_str(),content.size()))
		{
			cmds.push_back(content);
		}
	}
	void sendMsg(const void *cmd,int size)
	{
		sendCmd((void*)cmd,size);
	}
	std::vector<std::string> cmds;
	void addToContainer ()
	{
		for (unsigned int index = 0; index < cmds.size();++index)
		{
			sendCmd(cmds[index].c_str(),cmds[index].size());
		}
		cmds.clear();
		if (GetEvents()->doReconnect)
		{
			(*(GetEvents()->doReconnect))(this);
		}
	}
};
class LTask:public LTCPTask,public remote::Connection{
public:
	LTask(LTCPTaskPool *pool,const int sock,const struct sockaddr_in *addr=NULL,bool compress=false):LTCPTask(pool,sock,addr,compress,false)	
	{
		printf("new TASK\n");	
	}
	bool msgParse(const void* cmd, const unsigned int len)
	{
		if (GetEvents()->doParseCmd)
		{
			(*(GetEvents()->doParseCmd))(this,(void*)cmd,len);
		}
		else
			theAllObj.get(remote::CONNECTION)->parse(this,(void*)cmd,len);
		return true;
	}
	void sendObject(remote::Object *object)
	{
		std::string content = object->toString();
		sendCmd(content.c_str(),content.size());
	}
	void sendMsg(const void *cmd,int size)
	{
		sendCmd((void*)cmd,size);
	}
};
class LServer:public LNetService,public Server
{
public:
	LServer():LNetService("LSERVER"){
#ifdef _MSC_VER
		WSADATA wsaData;
		int nResult;
		nResult = WSAStartup(MAKEWORD(2,2), &wsaData);
#endif
	}
#ifdef _MSC_VER
	void newTCPTask(const SOCKET sock, const struct sockaddr_in *addr)
#else
	void newTCPTask(const int sock, const struct sockaddr_in *addr)	
#endif
	{
		LTCPTask *task = NULL;
		if (GetEvents()->doNewTask)
		{
			task = (*(GetEvents()->doNewTask))(taskPool,sock,addr);
		}
		else 
			task = new LTask(taskPool,sock,addr);
		if (task)
			taskPool->addOkay(task);
	}
	bool init(unsigned short port)
	{
		serverClientPool = new LTCPClientTaskPool(800);
		if(NULL==serverClientPool ||!serverClientPool->init())		
		{}
		int state = 0;
		taskPool = new LTCPTaskPool(1000,state,512);
		if(NULL==taskPool||!taskPool->init())
			return false;
		if(!LNetService::init(port))
			return false;
		printf("do Init\n");
		return true;
	}	
	LTCPTaskPool *taskPool;
	LTCPClientTaskPool * serverClientPool;
	std::map<std::string,Client*> clients;
	typedef std::map<std::string,Client*>::iterator CLIENTS_ITER;
	Client * getClient(const char *ip,unsigned short port)
	{
		std::stringstream ss;
		ss << ip << ":" << port;
		CLIENTS_ITER iter = clients.find(ss.str());
		if (iter != clients.end())
		{
			return iter->second;
		}
		else
		{
			Client * client = NULL;
			if (GetEvents()->doNewClient)
			{
				client = (*(GetEvents()->doNewClient))(serverClientPool,ip,port);
			}
			else 
				client = new LClient(ip,port);
			LClient *l = (LClient*) client;
			serverClientPool->put(l);
			clients[ss.str()] = client;
			return client;
		}
		return NULL;
	}
	void go()
	{
		main();
	}
};
LServer server;
Server * GetServer()
{
	return &server;
}
Events events;
Events* GetEvents()
{
	return &Events::getMe();;
}


void Events::bind(int ev,msg::function * function)
{
	if (ev >= functions.size()) functions.resize(ev+2);
	functions[ev] = function;
}
void Events::call(int ev,remote::Connection* conn,void *cmd,int len)
{
	if (ev >= functions.size())
	{
		if (defaultFunction) defaultFunction->call(conn,(const char*)cmd,len);
	}
	else if (functions[ev])
	{
		functions[ev]->call(conn,(const char*)cmd,len);
	}
}

msg::function * Events::getFunction(int ev)
{
	if (ev >= functions.size())
	{
		return defaultFunction;
	}
	else if (functions[ev])
	{
		return functions[ev];
	}
	return defaultFunction;
}