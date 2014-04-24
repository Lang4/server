#pragma once
/**
 * 基于消息的方式
 * class Login:public remote::Object
 * {
 *		void doRead(remote::Connection *connection)
 *		{
 *		}
 *		DEC_REMOTE(Login,1,2);
 * }
 * REMOTE_OBJECT(Login)
 * {}
 */
#include <vector>
#include "MsgPack.h"
#define REMOTE_OBJECT(STUB,CLASS) \
remote::AutoRegisterObject CLASS##OO(STUB,new CLASS());\
void CLASS::__parse__(MsgPack &pack)\
{\
	pack & (*this);\
}\
void operator & (MsgPack &ss,CLASS &o)

#define DEC_REMOTE(CLASS,t,p) \
	CLASS()\
	{\
		type = t;\
		para = p;\
		__init__();\
	}\
	friend void operator & (MsgPack &ss,CLASS &o);\
	void __parse__(MsgPack &pack);\
	void __init__()
	
namespace remote{
	enum{
		CONNECTION = 0,
	};
	class Connection;
	class Object{
	public:
		virtual void doRead(Connection *connection) = 0;
		void parse(void *cmd,unsigned int size){
			std::string content;
			content.resize(size);
			memcpy(&content[0],cmd,size);
			MsgPack pack(content);
			pack.setTag(MsgPack::IN);
			__parse__(pack);
		}
		virtual void __parse__(MsgPack &pack) = 0;
		virtual std::string toString()
		{
			std::string content;
			MsgPack pack(content);
			pack.setTag(MsgPack::OUT);
			pack.write(type);
			pack.write(para);
			__parse__(pack);
			return content;
		}
		unsigned char type;
		unsigned char para;
		Object()
		{
			type = para = 0;
		}
	};
	class Connection{
	public:
		virtual void sendObject(Object *object) = 0;
		virtual void sendMsg(const void *cmd,int size) = 0;
	};
	class Objects{
	public:
		virtual ~Objects(){}
		void parse(remote::Connection *connection,void *cmd,unsigned int size)
		{
			unsigned char * pointer = (unsigned char *) cmd;
			unsigned char type = *pointer;
			unsigned char para = *(pointer + 1);
			//printf("parse %u %u %lu %lu %p\n",type,para,events.size(),events[type].size(),events[type][para]);
			if (type < events.size() && para < events[type].size())
			{
				Object* ev = events[type][para];
				if (ev)
				{
					ev->parse((void*)(pointer+2),size-2);
					printf("do parse object %p\n",ev);
					ev->doRead(connection);
				}
			}
		}
		void add(Object *object)
		{
			if (object->type >= events.size())
				events.resize(object->type + 1);
			if (object->para >= events[object->type].size())
				events[object->type].resize(object->para + 1);
			events[object->type][object->para] = object;
		}
		std::vector<std::vector<Object*> > events;
	};
	class AllObjects{
	public:
		static AllObjects & getMe()
		{
			static AllObjects objects;
			return objects;
		}
		Objects * get(unsigned int index)
		{
			if (index >= objects.size()) objects.resize(index+1);
			return &objects[index];
		}
	private:
		std::vector<Objects> objects;
	};
	
	#define theAllObj remote::AllObjects::getMe()
	
	class AutoRegisterObject{
	public:
		AutoRegisterObject(unsigned int index,Object *object)
		{
			theAllObj.get(index)->add(object);
		}
	};
}

