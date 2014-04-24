#include "db.h"


class TestMyTable:public Table{
public:
	int id;
//	int id1;
	std::string name;
	std::vector<unsigned char> content;
	DEC_TABLE
};

TABLE_BEGIN(TestMyTable)
	FIELD(id);
//	FIELD(id1);
	FIELD(name);
	FIELD(content);
TABLE_END

int main()
{
	DB db;
	db.init("test");

	TestMyTable *my = NULL;
	if (db.create(my))
	{
		my->load(db," where id = 100");
		//my->id = 100;
		//my->id1 = 1000111;
		//my->name = "hello0";
		//my->content.push_back(100);
		//my->content.push_back(200);
		//my->update(db);
	}
}