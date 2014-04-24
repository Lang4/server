/****************************************************************
 * Date 2014-1-3
 * Author jijinlong the new serialize 对象进行非侵入式持久化 v1.0 
 ************ example*****************************************
 *	class Object{
	public:
		int a;
		int b;
		int c;
	};
	class Object1{
	public:
		Object b;
		std::vector<Object> os;
		std::vector<int> is;
		std::map<int,int> ms;
	};
	IMP_SERIALIZE_OBJECT(Object)
	{
		BIND(a,1);
		BIND(b,2);
		BIND(c,3);
	}
	IMP_SERILIZA_OBJECT(Object1)
	{
		BIND(b,1);
		BIND(os,5);
		BIND(is,6);
		BIND(ms,3);
	}
	BYTE_ARRAY *out;
	Object1 o1;
	BinPack ss(o1,&out);

	Object1 o2;
	BinPack ss2(o2,out);
	delete out;
 */
#pragma once

#include <iostream>
#include <vector>
//#include <strings.h>
#include <string>
#include <list>
#include <set>
#include <map>

typedef std::vector<unsigned char> BYTE_ARRAY;
class Field{
public:
	enum{
		EXT_FIELD_TAG = 5,
		BYTE_1_TAG = 1,
		BYTE_2_TAG = 2,
		BYTE_4_TAG = 3,
		BYTE_8_TAG = 4,
	};

	Field()
	{
		fieldHead = 0;
	}
	Field & operator=(const Field &field)
	{
		fieldHead = field.fieldHead;
		contents = field.contents;
		return *this;
	}
	unsigned char fieldHead;
	BYTE_ARRAY contents;

	bool get(BYTE_ARRAY &binary,int &offset)
	{
		if ((unsigned int)offset >= binary.size()) return false;
		fieldHead = binary[offset];
		offset ++;
		if (fieldHead == 0xff) return false;
		if ((unsigned int)offset >= binary.size()) return false;
		unsigned short content_size = getSize(fieldHead & 0x0f);
		if (content_size == 0xffff) return false;
		unsigned short headoffset = 0;
		if (content_size == 0)
		{
			headoffset = sizeof(unsigned short);
			if ((unsigned int)offset >= binary.size()) return false;
			memcpy(&content_size,&binary[offset],sizeof(unsigned short));
		}
		contents.resize(content_size);
		memcpy((void*) &contents[0],&binary[offset+headoffset],content_size);
		offset += content_size + headoffset;
		return true;
	}
	static unsigned char getTag(size_t size)
	{
		switch(size){
			case sizeof(char) :return BYTE_1_TAG;
			case sizeof(short):return BYTE_2_TAG;
			case sizeof(int):return BYTE_4_TAG;
			case sizeof(long long):return BYTE_8_TAG;
		}
		return EXT_FIELD_TAG;
	}
	static unsigned short getSize(int tag)
	{
		switch(tag)
		{
			case BYTE_1_TAG:return 1;
			case BYTE_2_TAG:return 2;
			case BYTE_4_TAG:return 4;
			case BYTE_8_TAG:return 8;
			case EXT_FIELD_TAG:return 0;
		}
		return 0xffff;
	}
}; 

class Block{
public:
	Block()
	{
		blockid = 0;
	}
	unsigned char blockid;
	std::vector<Field> fields;
	typedef std::vector<Field>::iterator FIELDS_ITER;
	
	template<typename FIELD>
	void write(unsigned char &fieldHead,FIELD a)
	{
		unsigned char index = (fieldHead & 0xf0) >> 4;
		if (index>= fields.size())
		{
			fields.resize(index + 1);
		}
		Field &field = fields[index];
		field.fieldHead = fieldHead;
		field.contents.resize(sizeof(FIELD));
		memcpy(&field.contents[0],&a,sizeof(FIELD));
	}

	void write(unsigned char &fieldHead,BYTE_ARRAY &a)
	{
		unsigned char index = (fieldHead & 0xf0) >> 4;
		if (index >= fields.size())
		{
			fields.resize(index + 1);
		}
		Field &field = fields[index];
		field.fieldHead = fieldHead;
		unsigned short headSize = sizeof(unsigned short);
		unsigned short size = a.size();
		field.contents.resize(headSize);
		memcpy(&field.contents[0],&size,headSize);
		field.contents.resize(a.size() + headSize);
		memcpy(&field.contents[headSize],(void*)&a[0],a.size());
	}

	bool read(unsigned char &fieldHead,BYTE_ARRAY &a)
	{
		unsigned char index = (fieldHead & 0xf0) >> 4;
		if (index >= fields.size())
		{
			return false;
		}
		Field &field = fields[index];
		if (field.contents.size() >= sizeof(unsigned short))
		{
			unsigned short size = field.contents.size();
			a.resize(size);
			memcpy(&a[0],&field.contents[0],size);
			return true;
		}
		return false;
	}

	template<typename FIELD>
	bool read(unsigned char &fieldHead,FIELD &a)
	{
		unsigned char index = (fieldHead & 0xf0) >> 4;
		if (index >= fields.size())
		{
			return false;
		}
		Field &field = fields[index];
		if (field.contents.size() >= sizeof(FIELD))
		{
			memcpy((void*)&a,&field.contents[0],sizeof(FIELD));
			return true;
		}
		return false;
	}

	void put(BYTE_ARRAY &binary)
	{
		if (fields.empty()) return;
		binary.push_back(blockid);
		for (FIELDS_ITER iter = fields.begin(); iter != fields.end();++iter)
		{
			if (iter->contents.empty()) continue;
			binary.push_back(iter->fieldHead);
			binary.insert(binary.end(),iter->contents.begin(),iter->contents.end());
		}
		binary.push_back(0xff);
	}

	bool get(BYTE_ARRAY &binary,int &offset)
	{
		if ((unsigned int)offset >= binary.size()) return false;
		blockid = binary[offset];
		offset++;

		if ((unsigned int)offset >= binary.size()) return false;
		while ((unsigned int)offset < binary.size())
		{
			Field field;
			if (!field.get(binary,offset)) return false;
			unsigned char index = (field.fieldHead & 0xf0) >> 4;
			if (index >= fields.size()) fields.resize(index + 1);
			fields[index] = field;
		}
		return true;
	}
};

#define DEC_SERIALIZE_BASE_TYPE(type)\
void write(type &a,unsigned short tag)\
{\
	write_base(a,tag);\
}\
void read(type &a,unsigned short tag)\
{\
	read_base(a,tag);\
}

enum INOUT{
	IN =  2,
	OUT = 3,
};
class BinPack{
public:
	INOUT tag;
	bool isTag(INOUT src) {return tag == src;}
	void setTag(INOUT src){tag = src;}
	BinPack(){}
	BinPack(BYTE_ARRAY *out)
	{
		setTag(IN);
		fromBinary(out);
	}
	BinPack(unsigned char *content,unsigned int len)
	{
		setTag(IN);
		BYTE_ARRAY contents;
		contents.resize(len);
		memcpy(&contents[0],content,len);
		fromBinary(&contents);
	}
	template<typename type>
	BinPack(type& o)
	{
		setTag(OUT);
		(*this)&o;
	}
	template<typename type>
	BinPack(type& o,BYTE_ARRAY **out)
	{
		setTag(OUT);
		(*this)&o;
		*out = toBinary();
	}
	template<typename type>
	BinPack(type& o,BYTE_ARRAY *out)
	{
		setTag(IN);
		fromBinary(out);
		(*this)&o;
	}
	template<typename type>
	BinPack(type& o,unsigned char *content,unsigned int len)
	{
		setTag(IN);
		BYTE_ARRAY contents;
		contents.resize(len);
		memcpy(&contents[0],content,len);
		fromBinary(&contents);
		(*this)&o;
	}
	template<typename type>
	void write_base(type a,unsigned short tag)
	{
		unsigned char blockid = (tag & 0x0ff0) >> 4;
		if (blockid >= blocks.size())
		{
			blocks.resize(blockid + 2);
		}
		Block &block = blocks[blockid];
		block.blockid = blockid;
		unsigned char fieldHead = (tag & 0x000f) << 4;
		fieldHead |= Field::getTag(sizeof(type));
		block.write(fieldHead,a);
	}
	template<typename type>
	void read_base(type& a,unsigned short tag)
	{
		unsigned char blockid = (tag & 0x0ff0) >> 4;
		if (blockid >= blocks.size())
		{
			return;
		}
		Block &block = blocks[blockid];
		unsigned char fieldHead = (tag & 0x000f) << 4;
		fieldHead |= Field::getTag(sizeof(type));
		block.read(fieldHead,a);
	}
	std::vector<Block> blocks;
	typedef std::vector<Block>::iterator BLOCKS_ITER;

	DEC_SERIALIZE_BASE_TYPE(int);
	DEC_SERIALIZE_BASE_TYPE(unsigned int);
	DEC_SERIALIZE_BASE_TYPE(char);
	DEC_SERIALIZE_BASE_TYPE(unsigned char);
	DEC_SERIALIZE_BASE_TYPE(long);
	DEC_SERIALIZE_BASE_TYPE(unsigned long);
	DEC_SERIALIZE_BASE_TYPE(long long);
	DEC_SERIALIZE_BASE_TYPE(unsigned long long);
	DEC_SERIALIZE_BASE_TYPE(float);
	DEC_SERIALIZE_BASE_TYPE(unsigned short);
	DEC_SERIALIZE_BASE_TYPE(short);
	DEC_SERIALIZE_BASE_TYPE(double);

	template<typename type>
	void write(type& a,unsigned short tag)
	{
		BinPack ss;
		ss.setTag(OUT);
		ss & a;
		write(ss,tag);
	}
	template<typename type>
	void read(type &a,unsigned short tag)
	{
		BinPack ss;
		ss.setTag(IN);
		read(ss,tag);
		ss & a;
	}
	bool read(BinPack &a,unsigned short tag)
	{
		unsigned char blockid = (tag & 0x0ff0) >> 4;
		if (blockid >= blocks.size())
		{
			return false;
		}
		Block &block = blocks[blockid];
		unsigned char fieldHead = (tag & 0x000f) << 4;
		fieldHead |= Field::EXT_FIELD_TAG;
		BYTE_ARRAY content;
		if (block.read(fieldHead,content))
		{
			a.fromBinary(&content);
			return true;
		}
		return false;
	}

	bool write(BinPack &a,unsigned short tag)
	{
		unsigned char blockid = (tag & 0x0ff0) >> 4;
		if (blockid >= blocks.size())
		{
			blocks.resize(blockid + 2);
		}
		Block &block = blocks[blockid];
		unsigned char fieldHead = (tag & 0x000f) << 4;
		fieldHead |= Field::EXT_FIELD_TAG;

		BYTE_ARRAY *out = a.toBinary();
		block.write(fieldHead,*out);
		delete out;
		return true;
	}
	bool read(std::string &a,unsigned short tag)
	{
		unsigned char blockid = (tag & 0x0ff0) >> 4;
		if (blockid >= blocks.size())
		{
			return false;
		}
		Block &block = blocks[blockid];
		unsigned char fieldHead = (tag & 0x000f) << 4;
		fieldHead |= Field::EXT_FIELD_TAG;
		BYTE_ARRAY content;
		if (block.read(fieldHead,content))
		{
			a.resize(content.size());
			memcpy(&a[0],&content[0],a.size());
			return true;
		}
		return false;
	}

	bool write(std::string &a,unsigned short tag)
	{
		unsigned char blockid = (tag & 0x0ff0) >> 4;
		if (blockid >= blocks.size())
		{
			blocks.resize(blockid + 2);
		}
		Block &block = blocks[blockid];
		unsigned char fieldHead = (tag & 0x000f) << 4;
		fieldHead |= Field::EXT_FIELD_TAG;

		BYTE_ARRAY out;
		out.resize(a.size());
		memcpy(&out[0],&a[0],a.size());
		block.write(fieldHead,out);
		return true;
	}

	BYTE_ARRAY * toBinary()
	{
		BYTE_ARRAY * temp = new BYTE_ARRAY;
		for (BLOCKS_ITER iter = blocks.begin(); iter != blocks.end();++iter)
		{
			iter->put(*temp);
		}
		return temp;
	}

	bool fromBinary(BYTE_ARRAY *binary)
	{
		if (!binary) return false;
		int offset = 0;
		while ((unsigned int)offset < binary->size())
		{
			Block block;
			bool hadBlock = block.get(*binary,offset);
			if (block.blockid >= blocks.size()) blocks.resize(block.blockid + 2);
			blocks[block.blockid] = block;
			if (hadBlock) break;
		}
		return true;
	}
};

#define IMP_SERIALIZE_OBJECT(__class__)\
void operator & (BinPack &ss,__class__ &o)\

#define BIND(field,version)\
	if (ss.isTag(OUT))\
	{\
		ss.write(o.field,version);\
	}\
	else\
	{\
		ss.read(o.field,version);\
	}

template<typename __type__>
void operator & (BinPack &ss,std::vector<__type__>& o)
{
	if (ss.isTag(OUT))
	{
		size_t size = o.size();
		ss.write(size,0);
		int index = 1;
		for (typename std::vector<__type__>::iterator iter = o.begin(); iter != o.end();++iter)
		{
			__type__ v = *iter;
			ss.write(v,index++);
		}
	}
	else
	{
		__type__ object;
		int size = 0;
		int index = 1;
		ss.read(size,0);
		while (size-- >0)
		{
			ss.read(object,index++);
			o.push_back(object);
		}
	}
}

template<typename __type__>
void operator & (BinPack &ss,std::list<__type__>& o)
{
	if (ss.isTag(OUT))
	{
		size_t size = o.size();
		ss.write(size,0);
		int index = 1;
		for (typename std::list<__type__>::iterator iter = o.begin(); iter != o.end();++iter)
		{
			__type__ v = *iter;
			ss.write(v,index++);
		}
	}
	else
	{
		__type__ object;
		int size = 0;
		int index = 1;
		ss.read(size,0);
		while (size-- >0)
		{
			ss.read(object,index++);
			o.push_back(object);
		}
	}
}

template<typename __type__>
void operator & (BinPack &ss,std::set<__type__>& o)
{
	if (ss.isTag(OUT))
	{
		size_t size = o.size();
		ss.write(size,0);
		int index = 1;
		for (typename std::set<__type__>::iterator iter = o.begin(); iter != o.end();++iter)
		{
			__type__ v = *iter;
			ss.write(v,index++);
		}
	}
	else
	{
		__type__ object;
		int size = 0;
		int index = 1;
		ss.read(size,0);
		while (size-- >0)
		{
			ss.read(object,index++);
			o.insert(object);
		}
	}
}

template<typename __key__,typename __value__>
void operator & (BinPack &ss,std::map<__key__,__value__>& o)
{
	if (ss.isTag(OUT))
	{
		size_t size = o.size();
		ss.write(size,0);
		unsigned short index = 1;
		for (typename std::map<__key__,__value__>::iterator iter = o.begin(); iter != o.end();++iter)
		{
			__key__ v = iter->first;
			ss.write(v,index++);
			ss.write(iter->second,index++);
		}
	}
	else
	{
		__key__ key;
		__value__ object;
		int size = 0;
		unsigned short index = 1;
		ss.read(size,0);
		while (size-- >0)
		{
			ss.read(key,index++);
			ss.read(object,index++);
			o[key] = object;
		}
	}
}
