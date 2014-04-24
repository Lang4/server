#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <map>

#define DEC_MSG_BASE_TYPE(type)\
void write(type &a)\
{\
	write_base(a);\
}\
void read(type &a)\
{\
	read_base(a);\
}

#define IMP_MSG_OBJECT(__class__)\
void operator & (MsgPack &ss,__class__ &o)\

#define MSGBIND(field)\
	if (ss.isTag(MsgPack::OUT))\
	{\
		ss.write(o.field);\
	}\
	else ss.read(o.field);

class MsgPack{
public:	
	enum INOUT{
		IN = 2,
		OUT = 3,
	};
	INOUT tag;
	bool isTag(INOUT src) {return tag == src;}
	void setTag(INOUT src){tag = src;}
	std::string& content;
	unsigned int read_offset;
	MsgPack(std::string &content,unsigned int read_offset = 0):content(content),read_offset(read_offset)
	{
	}
	~MsgPack()
	{
	}
	template<typename type>
	void write_base(type a)
	{
		unsigned int offset = content.size();
		content.resize(content.size() + sizeof(type));
		memcpy(&content[offset],&a,sizeof(type));
	}
	template<typename type>
	void read_base(type& a)
	{
		memcpy((char*)&a,&content[read_offset],sizeof(type));
		read_offset += sizeof(type);		
	}
	DEC_MSG_BASE_TYPE(int);
	DEC_MSG_BASE_TYPE(char);
	DEC_MSG_BASE_TYPE(short);
	DEC_MSG_BASE_TYPE(unsigned int);
	DEC_MSG_BASE_TYPE(unsigned char);
	DEC_MSG_BASE_TYPE(unsigned short);
	DEC_MSG_BASE_TYPE(float);
	
	template<typename type,int size>
	void write(type (& a)[size])
	{
		if (size)
		{
			unsigned int offset = content.size();
			content.resize(content.size()+size);
			memcpy(&content[offset],&a[0],size);
		}	
	}
	template<typename type,int size>
	void read(type (& a)[size])
	{
		if (size)
		{
			memcpy(&a[0],&content[read_offset],size);
			read_offset += size;
		}
	}

	template<typename type>
	void write(type& a)
	{
		MsgPack ss(content,read_offset);
		ss.setTag(OUT);
		ss & a;
	}
	template<typename type>
	void read(type& a)
	{
		MsgPack ss(content,read_offset);
		ss.setTag(IN);
		ss & a;
		read_offset = ss.read_offset;
	}
	void read(std::string &a)
	{
		unsigned short len = 0;
		read(len);
		if (len)
		{
			a.resize(len);
			memcpy(&a[0],&content[read_offset],len);
			read_offset += len;
		}
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
		}	
	}
};
template<typename __type__>
void operator & (MsgPack &ss,std::vector<__type__>& o)
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
		__type__ object;
		int size = 0;
		ss.read(size);
		while (size-- >0)
		{
			ss.read(object);
			o.push_back(object);
		}
	}
}


