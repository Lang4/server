#include "MsgPack.h"

struct Test{
	int a;
	Test()
	{
		a = 0;
	}
	Test(int c):a(c){}
};

struct MSG{
	int b;
	std::vector<Test> tests;
	int c;
	char a[100];
};

IMP_MSG_OBJECT(Test)
{
	MSGBIND(a);
}

IMP_MSG_OBJECT(MSG)
{
	MSGBIND(b);
	MSGBIND(tests);
	MSGBIND(c);
	MSGBIND(a);
}


int main()
{
	std::string  content;
	MsgPack pack(content);
	MSG msg1;
	msg1.b = 1001;
	msg1.c = 1002;
	msg1.tests.push_back(Test(1));
	msg1.tests.push_back(Test(2));
	strncpy(msg1.a,"helloworld",11);
	pack.write(msg1);


	MSG msg2;
	pack.read(msg2);
	printf("%d %d\n",msg2.b,msg2.c);
}
