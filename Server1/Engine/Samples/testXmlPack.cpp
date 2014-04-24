#include "XmlPack.h"
class Object1{
public:
	int d;
	Object1()
	{
		d = 0;
	}
};

class Object{
public:
	int a;
	int c;
	Object1 object1;
	Object()
	{
		a = 1;
		c = 2;
	}
	std::string d;
	std::vector<int> es;
	std::vector<Object1> os;
	std::map<std::string,Object1> ms;
};
IMP_XML_OBJECT(Object1)
{
	XMLBIND(d);
}
IMP_XML_OBJECT(Object)
{
	XMLBIND(a);
	XMLBIND(object1);
	XMLBIND(c);
	XMLBIND(d);
	XMLBIND(es);
	XMLBIND(os);
	XMLBIND(ms);
}

int main()
{
	XmlPack p("tt");
	Object o;
	o.es.push_back(2);
	o.os.push_back(Object1());
	o.ms["tt"] = Object1(); 
	o.ms["tt1"] = Object1();
	p.WRITE(o);

	Object o1;
	p.read(o1,"o1");
	p.save();
}
