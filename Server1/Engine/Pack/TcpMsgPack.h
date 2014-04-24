#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <Connect.h>

/**
 * 该消息将被读取
 * class Object{
 *  	int a;
 * 	void onRead(Connection *connection)
 * 	{
 *			
 *	}
 *};
 * Object o1;
 * TcpMsgPack pack(conection);
 * pack.read(o1,o1);
 */

#define DEC_TCPMSG_BASE_TYPE(type)\
void write(type &a)\
{\
	write_base(a);\
}\
void read(type &a)\
{\
	read_base(a);\
}

#define IMP_TCPMSG_OBJECT(__class__)\
void operator & (TcpMsgPack &ss,__class__ &o)\

#define TCPMSGBIND(field)\
	if (ss.isTag(TcpMsgPack::OUT))\
	{\
		ss.write(o.field);\
	}\
	else ss.read(o.field);


/**
 * TCP Sync ReadWrite 可以读取极大数据 
 */
class TcpMsgPack{
public:
	enum INOUT{
		IN = 2,
		OUT = 3,
	};
	INOUT tag;
	bool isTag(INOUT src) {return tag == src;}
	void setTag(INOUT src){tag = src;}
	Connection * connection;
	TcpMsgPack(Connection *connection)
		:connection(connection)
	{
	}
	~TcpMsgPack()
	{
	}
	template<typename type>
	void write_base(type a)
	{
		connection->send(&a,sizeof(type));
	}
	template<typename type>
	void read_base(type& a)
	{
		connection->read(&a,sizeof(type));
	}
	DEC_MSG_BASE_TYPE(int);
	DEC_MSG_BASE_TYPE(char);
	DEC_MSG_BASE_TYPE(short);
	DEC_MSG_BASE_TYPE(unsigned int);
	DEC_MSG_BASE_TYPE(unsigned char);
	DEC_MSG_BASE_TYPE(unsigned short);
	DEC_MSG_BASE_TYPE(float);


	template<typename type>
	void write(type& a)
	{
		MsgPack ss(connection);
		ss.setTag(OUT);
		ss & a;
	}
	template<typename type>
	void read(type& a)
	{
		MsgPack ss(connection);
		ss.setTag(IN);
		ss & a;
	}
	void read(std::string &a)
	{
		unsigned short len = 0;
		struct stCallback:public ReadCallback{
			void callback(Connection * connection,void *content,unsigned int len)
			{
				unsigned short * len = *(unsigned short*) content;
				a.resize(len);
				connection->read(&a[0],len); // 读数据 到缓存
			}
			std::string & a;
			stCallback(std::string &a):a(a){}
		};
		stCallback *callback = new stCallback(a);
		connection->read(sizeof(unsigned short),callback); // 读数据后产生回调 
	}
	void write(std::string &a)
	{
		unsigned short len = a.size();
		write(len);
		if (len)
		{
			unsigned int offset = content.size();
			content.resize(content.size()+len);
			memcpy(&content[offset],&a[0],len);
			content->send(&content[offset],len);
		}	
	}
	template<typename __type__>
	void operator & (TcpMsgPack &ss,std::vector<__type__>& o)
	{
		if (ss.isTag(MsgPack::OUT))
		{
			int size = o.size();
			ss.write(size);
			for (typename std::vector<__type__>::iterator iter = o.begin(); iter != o.end();++iter)
			{
				__type__ v = *iter;
				ss.write(v);
			}
		}
		else
		{
			struct stCallback:public ReadCallback{
				void callback(Connection *connection,void *content,unsigned int len)
				{
					unsigned int * size = *(unsigned int*) content;
					while (size-- >0)
					{
						__type__ object;
						ss.read(object);
						o.push_back(object);
					}
				}
				TcpMsgPack & ss;
				std::vector<__type__> & o;
				stCallback(TcpMsgPack &ss,std::vector<__type__> &o):ss(ss),o(o){}
			};
			stCallback *callback = new stCallback(ss,o);
			ss.connection->read(sizeof(int),callback);
		}
	}

};
