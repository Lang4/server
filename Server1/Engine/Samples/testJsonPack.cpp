#include "JsonPack.h"

class Object1{
public:
	std::string a;
	int b;
	std::vector<int> cs;
	std::map<std::string,std::string> ds;
};

IMP_JSON_OBJECT(Object1)
{
	JSONBIND(a);
	JSONBIND(b);
	JSONBIND(cs);
	JSONBIND(ds);
}

int main()
{
	JsonPack p;
	Object1 o;
	o.b = 1000;
	o.cs.push_back(1);
	o.cs.push_back(2);
	o.ds["1"] = "2";
	o.ds["2"] = "3";
	p.write(o,"tt");
	
	std::string tt = p.toString().c_str();		
	printf("json:%s\n",tt.c_str());
	
	Object1 o2;
	p.read(o2,"tt");
	printf("o2.a=%d\n",o2.b);
}
