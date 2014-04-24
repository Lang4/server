#pragma once
#include "tinyxml2.h"
#include <string>
#include <vector>
#include <sstream>
#include <map>
#define DEC_XML_BASE_TYPE(type)\
void write(type &a,const std::string &tt)\
{\
	write_base(a,tt);\
}\
void read(type &s,const std::string &tt){ \
	read_base(s,tt); \
};

#define IMP_XML_OBJECT(__class__)\
void operator & (XmlPack &ss,__class__ &o)\

#define WRITE(o) write(o,#o)

#define XMLBIND(field)\
	if (ss.isTag(XmlPack::OUT))\
	{\
		ss.write(o.field,#field);\
	}\
	else ss.read(o.field,#field);

class XmlPack{
public:	
	enum INOUT{
		IN = 2,
		OUT = 3,
	};
	INOUT tag;
	bool isTag(INOUT src) {return tag == src;}
	void setTag(INOUT src){tag = src;}
	XmlPack(tinyxml2::XMLDocument *doc):doc(doc){
		isDestroy = false;
	}
	tinyxml2::XMLElement* root;
	tinyxml2::XMLDocument* doc;
	bool isDestroy;
	std::string xmlFile;
	XmlPack(const std::string &xmlname)
	{
		xmlFile = xmlname;
		doc = new tinyxml2::XMLDocument();
		if (!doc->LoadFile(xmlname.c_str()))
		{
			root = doc->RootElement();
		} 
		else
		{
			root = doc->NewElement("config"); 
			doc->LinkEndChild(root);	
		}
		isDestroy = true;
	}
	~XmlPack()
	{
		if (isDestroy && doc) delete doc;
		doc = NULL; 
	}
	void save()
	{
		doc->SaveFile(xmlFile.c_str());	
	}
	template<typename type>
	void write_base(type a,const std::string &name)
	{
		if (root)
		{
			root->SetAttribute(name.c_str(),(int)a);
		}	
	}
	template<typename type>
	void read_base(type& a,const std::string &name)
	{
		if (root)
		{
			a = root->IntAttribute(name.c_str());
		}	
	}

	DEC_XML_BASE_TYPE(int);
	DEC_XML_BASE_TYPE(char);
	DEC_XML_BASE_TYPE(short);
	DEC_XML_BASE_TYPE(unsigned int);
	DEC_XML_BASE_TYPE(unsigned char);
	DEC_XML_BASE_TYPE(unsigned short);
	DEC_XML_BASE_TYPE(float);


	template<typename type>
	void write(type& a,const std::string& tag)
	{
		XmlPack ss(doc);
		ss.root = doc->NewElement(tag.c_str()); 	
		ss.setTag(OUT);
		ss & a;
		write(ss,tag);
	}
	void write(XmlPack &a,const std::string& tag)
	{
		if (root && a.root)
		{
			root->InsertEndChild(a.root);
		}
	}
	template<typename type>
	void read(type& a,const std::string& tag)
	{
		XmlPack ss(doc);
		ss.setTag(IN);
		read(ss,tag);
		ss & a;
	}
	void read(XmlPack &a,const std::string& tag)
	{
		a.root = root->FirstChildElement();	
	}
	void read(std::string &a,const  std::string &tag)
	{
		if (root)
		{
			a = root->Attribute(tag.c_str()) ? root->Attribute(tag.c_str()):"";
		}		
	}
	void write(std::string &a,const std::string &tag)
	{
		if (root)
		{
			root->SetAttribute(tag.c_str(),a.c_str());
		}	
		
	}
};

template<typename __type__>
void operator & (XmlPack &ss,std::vector<__type__>& o)
{
	if (ss.isTag(XmlPack::OUT))
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
void operator & (XmlPack &ss,std::map<__key__,__value__>& o)
{
	if (ss.isTag(XmlPack::OUT))
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
	
