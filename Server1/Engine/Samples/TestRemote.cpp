#include "Net.h"
#include "message.h"
class Object:public remote::Object{
public:
	DEC_REMOTE(Object,1,2)
	{
		a = 100;
		memset(name,0,10);
		strncpy(name,"hello,worl",9);
	}
	int a;
	void doRead(remote::Connection *connection)
	{
		printf("%d %s\n",a,name);
	}
	char name[10];
};

REMOTE_OBJECT(remote::CONNECTION,Object)
{
	MSGBIND(a);
	MSGBIND(name);
}
#pragma pack(1)
struct Msg{
	int a;
	Msg()
	{
		a = 1;
	}
};
#pragma pack()

class Object1{
public:
	void sayHello(const Msg& a)
	{
		
	}
};

EVENTS_MAP(Object1) // Events 自带的协议
{
	EVENT(1,Object1::sayHello);
}

void DoParseCmd(remote::Connection *connection,void *cmd,int len)
{
	int id = *(int*) cmd;
	Object1 object;
	if (GetEvents()->getFunction(id))
		GetEvents()->getFunction(id)->call(&object,(const char *)cmd,len); // 处理消息
}
void DoReConnect(Client *client)
{
	Msg msg;
	client->sendMsg(&msg,sizeof(msg));
}
int main()
{
	GetServer()->init(5051);
	GetEvents()->doParseCmd = DoParseCmd; // 处理消息回调
	GetEvents()->doReconnect = DoReConnect; // 处理重连
	Object login;
	GetServer()->getClient("127.0.0.1",5050)->sendObject(&login);
	GetServer()->go();
}

