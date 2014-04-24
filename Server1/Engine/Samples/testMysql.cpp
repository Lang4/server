#include "mysqlDB.h"
class Object:public mysql::Table{
public:
	int id;
	std::string name;
	std::vector<char> buffer;
	char name1[10];
	DEC_TABLE
};

TABLE_BEGIN(Object)
	FIELD(id);
	FIELD(name);
	FIELD(name1);
	FIELD(buffer);	
TABLE_END


class Test1:public mysql::Table{
public:
	int a;
	int b;
	int c;
	DEC_TABLE
};
TABLE_BEGIN(Test1)
	FIELD(a);
	FIELD(b);
	FIELD(c);
TABLE_END
int main()
{

	mysql::DB db;
	db.init("127.0.0.1","test","root","123456");
	Object *object = NULL;
	if (db.create(object))
	{
#if (0)
		printf("create object success\n");	
		object->name = "hello";
		object->id = 10001;
		object->buffer.resize(10);
		strncpy(object->name1,"jin",4);
		strncpy(&object->buffer[0],"jhewewe",8);
		object->add(db);
#else
		object->load(db,"where `a`=10001");
#endif
		delete object;
	}

	Test1 * test1 = NULL;
	if (db.create(test1))
	{
#if (0)
		test1->a = test1->b = test1->c = 10001;
		test1->add(db);
#else
		test1->load(db,"");
#endif
		delete test1;
	}
}
