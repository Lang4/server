#include "message.h"
#include "string.h"
class Object1{
public:
	void init()
	{
		printf("hello,world\n");
	}
	void sayHello(int a)
	{
		printf("o sayHello :%u\n",a);
	}
	void doArg2(int a,const std::string &b)
	{
		printf("o doArg2 %u %s\n",a,b.c_str());
	}
	void doArg3(int a,const std::string &b,float c)
	{
		printf("o doArg3 %u %s %f\n",a,b.c_str(),c);
	}

	void doArg4(int a,const std::string &b,float c,int d)
	{
		printf("o doArg4 %u %s %f %d\n",a,b.c_str(),c,d);
	}
};

void init()
{
	printf("hello,world2\n");
}

void sayHello(int a)
{
	printf("sayHello :%u\n",a);
}

int main()
{
	Object1 object;
	msg::call<void>(&object,msg::bind(&Object1::init));

	msg::call<void>(msg::bind(init));

	msg::call<void>(msg::bind(sayHello),100);

	msg::call<void>(&object,msg::bind(&Object1::sayHello),100);


	msg::call<void>(&object,msg::bind(&Object1::doArg2),100,"test2");

	msg::call<void>(&object,msg::bind(&Object1::doArg3),100,"test3",(float)1000.1);

	msg::call<void>(&object,msg::bind(&Object1::doArg4),100,"test4",(float)1000.1,5000);

	return 0;

}

