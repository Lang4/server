#pragma once
extern "C"{
	#include "cJSON.h"
}
#include <string>
#include <vector>
#include <sstream>
#include <map>
#define DEC_JSON_BASE_TYPE(type)\
void write(type &a,const std::string &tt)\
{\
	write_base(a,tt);\
}\
void read(type &s,const std::string &tt){ \
	read_base(s,tt); \
};

#define IMP_JSON_OBJECT(__class__)\
void operator & (JsonPack &ss,__class__ &o)\

#define WRITE(o) write(o,#o)

#define JSONBIND(field)\
	if (ss.isTag(JsonPack::OUT))\
	{\
		ss.write(o.field,#field);\
	}\
	else ss.read(o.field,#field);

class JsonPack{
public:	
	enum INOUT{
		IN = 2,
		OUT = 3,
	};
	INOUT tag;
	bool isTag(INOUT src) {return tag == src;}
	void setTag(INOUT src){tag = src;}
	JsonPack(cJSON * root):root(root){
		isDestroy = false;
	}
	cJSON* root;
	bool isDestroy;
	JsonPack(const std::string &xmlname)
	{
		root =cJSON_Parse(xmlname.c_str());
		isDestroy = true;
	}
	JsonPack()
	{
		root = cJSON_CreateObject();
		isDestroy = true;
	}
	~JsonPack()
	{
		if (isDestroy && root) cJSON_Delete(root);
		root = NULL; 
	}
	std::string toString()	
	{
		char *out=cJSON_Print(root);
		std::string tempstr = out;
		free(out);
		return tempstr;
	}
	template<typename type>
	void write_base(type a,const std::string &name)
	{
		if (root)
		{
			cJSON_AddNumberToObject(root,name.c_str(),(int)a);
		}	
	}
	template<typename type>
	void read_base(type& a,const std::string &name)
	{
		cJSON * temp = cJSON_GetObjectItem(root,name.c_str());
		if (temp)
		{
			char *out=cJSON_Print(temp);
			a = atoi(out);
			free(out);
		}
	}

	DEC_JSON_BASE_TYPE(int);
	DEC_JSON_BASE_TYPE(char);
	DEC_JSON_BASE_TYPE(short);
	DEC_JSON_BASE_TYPE(unsigned int);
	DEC_JSON_BASE_TYPE(unsigned char);
	DEC_JSON_BASE_TYPE(unsigned short);
	DEC_JSON_BASE_TYPE(float);


	template<typename type>
	void write(type& a,const std::string& tag)
	{
		JsonPack ss(cJSON_CreateObject());	
		ss.setTag(OUT);
		ss & a;
		write(ss,tag);
	}
	void write(JsonPack &a,const std::string& tag)
	{
		if (root && a.root)
		{
			cJSON_AddItemToObject(root,tag.c_str(),a.root);
		}
	}
	template<typename type>
	void read(type& a,const std::string& tag)
	{
		JsonPack ss(root);
		ss.setTag(IN);
		read(ss,tag);
		ss & a;
	}
	void read(JsonPack &a,const std::string& tag)
	{
		a.root = cJSON_GetObjectItem(root,tag.c_str()); 
	}
	void read(std::string &a,const  std::string &tag)
	{
		if (root)
		{
			cJSON * temp = cJSON_GetObjectItem(root,tag.c_str());
			if (temp)
			{
				char *out=cJSON_Print(temp);
				a = out;
				free(out);
			}	
		}		
	}
	void write(std::string &a,const std::string &tag)
	{
		if (root)
		{
			cJSON_AddStringToObject(root,tag.c_str(),a.c_str());
		}	
		
	}
};

template<typename __type__>
void operator & (JsonPack &ss,std::vector<__type__>& o)
{
	if (ss.isTag(JsonPack::OUT))
	{
		int size = o.size();
		ss.write(size,"size");
		int index = 0;
		for (typename std::vector<__type__>::iterator iter = o.begin(); iter != o.end();++iter)
		{
			__type__ v = *iter;
			std::stringstream name;
			name <<"element"<<index ++;
			ss.write(v,name.str().c_str());
		}
	}
	else
	{
		__type__ object;
		int size = 0;
		ss.read(size,"size");
		int index = 0;
		while (size-- >0)
		{
			std::stringstream name;
			name <<"element"<<index ++ ;
			ss.read(object,name.str().c_str());
			o.push_back(object);
		}
	}
}

template<typename __key__,typename __value__>
void operator & (JsonPack &ss,std::map<__key__,__value__>& o)
{
	if (ss.isTag(JsonPack::OUT))
	{
		int size = o.size();
		ss.write(size,"size");
		unsigned short index = 0;
		for (typename std::map<__key__,__value__>::iterator iter = o.begin(); iter != o.end();++iter)
		{
			__key__ v = iter->first;
			std::stringstream key;
			key << "key" << index;
			std::stringstream value;
			value << "value" << index++;

			ss.write(v,key.str());
			ss.write(iter->second,value.str());
		}
	}
	else
	{
		__key__ key;
		__value__ object;
		int size = 0;
		unsigned short index = 0;
		ss.read(size,"size");
		while (size-- >0)
		{
			std::stringstream keyn;
			keyn << "key" << index;
			std::stringstream valuen;
			valuen << "value" << index++;

			ss.read(key,keyn.str().c_str());
			ss.read(object,valuen.str().c_str());
			o[key] = object;
		}
	}
}
	
