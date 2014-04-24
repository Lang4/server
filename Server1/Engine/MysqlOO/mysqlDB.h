#include "mysql.h"
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <list>
#include <algorithm>
namespace mysql{
	enum State
	{
		IDLE = 0,	 
		USING = 1,
	};
	class Connections;
	class Connection{
	public:
		State state;
		int ping()
		{
			return mysql_ping(mysql);
		}
		bool connect();
		MYSQL *mysql;
		Connections *connections;
		Connection(Connections *connections):connections(connections)
		{
			connections = NULL;
			state = IDLE;
			mysql = NULL;
		}
		void close()
		{
			mysql_close(mysql);
			mysql = NULL;
		}
		State getState()
		{
			return state;
		}
		void setState(State state){this->state = state;}
	};
	class Connections{
	public:
		Connection* getConnect();
		void putConnect(Connection *conn);
		std::string host;
		unsigned short port;
		std::string username;
		std::string passwd;
		std::string dbname;
		std::list<Connection *> conns;
	};
	class FieldInfo{
	public:
		FieldInfo()
		{
			type = -1;
		}
		std::string name;
		int type;	
		std::string getDesc();
		bool equal(FieldInfo &info);
		
		FieldInfo(const std::string &name,int type); // FIELD_TYPE_LONG
		FieldInfo(const std::string &name,std::string& value); //FIELD_TYPE_STRING
		template<int size>
		FieldInfo(const std::string &name,char (& a)[size]) // FIELD_TYPE_LONG
		{
			this->name = name;
			this->type = FIELD_TYPE_STRING;
		}
		FieldInfo(const std::string &name,std::vector<char> &buffer); // FIELD_TYPE_BLOB
	};
	class EachFieldInfo{
	public:
		virtual void callback(FieldInfo &field)
		{}
		virtual void end()
		{}
		virtual void begin()
		{}
	};
	class TableInfo{
	public:
		std::string name;
		std::vector<FieldInfo> fields; 
		FieldInfo *findField(const std::string &fieldName);
		void execAll(EachFieldInfo *exec);
	};
	class DB{
	public:
		bool init(const char *host,const char * dbname,const char *username,const char *pwd);
		bool execSql(const std::string& sql);

		TableInfo * getTable(const std::string &name);
		FieldInfo *findField(const std::string &tableName,const std::string &fileName);
		TableInfo *findTable(const std::string &tableName);

		template<typename CLASS>
		bool create(CLASS * &object)
		{
			object = new CLASS();
			if (object)
			{
				object->init();
				object->updateInfo(*this);
				return true;
			}
			return false;
		}
		MYSQL * getHandle();
		void putHandle();
	private:
		std::map<std::string,TableInfo> tablesInfo;
		typedef std::map<std::string,TableInfo>::iterator TABLES_INFO_ITER;
		Connections connections;
		Connection * nowConnection;
	};

	template<class TABLE>
	class CreateTableInfo:public EachFieldInfo{
	public:
		void callback(FieldInfo &field)
		{
			createInfo << field.getDesc() <<",";
		}
		void end()
		{
			std::string temp = createInfo.str();
			if (temp.size() < 2) return;
			temp[temp.size() - 1] = ')';
			temp.append(";");
			db.execSql(temp);
		}
		void  begin()
		{
			createInfo<<"CREATE TABLE " <<  table.getTableName() << " (";
		}
		std::stringstream createInfo;
		DB &db;
		TABLE &table;
		CreateTableInfo(DB &db,TABLE &table):db(db),table(table)
		{}
	};
	template<class TABLE>
	class ModifyTableInfo:public EachFieldInfo{
	public:
		void begin(){}
		void callback(FieldInfo &field)
		{
			FieldInfo *target = db.findField(table.getTableName(),field.name);
			std::stringstream modifyInfo;
			if (!target)
			{
				modifyInfo <<"ALTER TABLE " <<   table.getTableName() <<" add " << field.getDesc() <<";";
			}
			else if (!target->equal(field))
			{
				modifyInfo <<"ALTER TABLE " <<   table.getTableName() <<" add " << field.getDesc() <<";";
			}
			if (modifyInfo.str() != "")
				db.execSql(modifyInfo.str());
		}
		void end()
		{

		}
		ModifyTableInfo(DB &db,TABLE &table):db(db),table(table)
		{}
		DB &db;
		TABLE &table;
	};
	template<class TABLE>
	struct MakeInsertTableInfo:public EachFieldInfo
	{
		void begin()
		{
			count = 0;
			makeStrBuffer<<"insert into " <<  table.getTableName() << " (" ;
		}
		void end()
		{

		}
		void callback(FieldInfo &field)
		{
			if (count != 0)
			{
				makeStrBuffer << "," << field.name;
			}
			else
			{
				makeStrBuffer <<field.name;
			}
			count++;
		}
		MakeInsertTableInfo(DB &db,TABLE &table):db(db),table(table)
		{
			count = 0;
		}
		std::stringstream makeStrBuffer;
		DB &db;
		TABLE &table;
		int count;
	};
	template<class TABLE>
	struct MakeSelectTableInfo:public EachFieldInfo{
		void begin()
		{
			count = 0;
			makeStrBuffer << "SELECT ";
		}
		void end()
		{
			makeStrBuffer<<" FROM `"<<table.getTableName()<<"`";
		}
		void callback(FieldInfo &field)
		{
			if (count != 0)
			{
				makeStrBuffer <<",`"<< field.name<<"`";
			}
			else
			{
				makeStrBuffer<<"`"<<field.name<<"`";
			}
			count ++;
		}
		MakeSelectTableInfo(DB &db,TABLE &table):db(db),table(table)
		{

		}
		std::stringstream makeStrBuffer;
		DB &db;
		TABLE &table;
		int count;
	};
	template<class TABLE>
	struct MakeUpdateTableInfo:public EachFieldInfo{
		void begin()
		{
			count = 0;
			updateStrBuffer<<"update " << table.getTableName() << " set " ;
		}
		void end()
		{

		}
		void callback(FieldInfo &field)
		{
			if (count != 0)
			{
				updateStrBuffer << "," << field.name<<"=?";
			}
			else
			{
				updateStrBuffer <<""<<field.name<<"=?";
			}
			count ++;
		}
		MakeUpdateTableInfo(DB &db,TABLE &table):db(db),table(table)
		{
			count = 0;
		}
		std::stringstream updateStrBuffer;
		DB &db;
		TABLE &table;
		int count;

	};
	class Table;
	class ExecEachTable{
	public:
		virtual void exec(Table *table) = 0;	
	};
	class Table{
	public:
		enum {
			ADD_FIELD = 0,
			TRACE_FIELD = 1,
			BIND_FIELD = 2, 
			WRITE_FIELD = 3,
			READ_FIELD = 4,
			WRITE_FIELD_BYNAME = 5,
		};
		virtual void __traceInfo__(int __traceType__,EachFieldInfo *exec = NULL,MYSQL_STMT *stmt = NULL,const std::string &name=""){};
		template<typename TABLE>
		static Table * create(DB &db)
		{
			TABLE *table = new TABLE();
			table->init();
			table->updateInfo(db);
			return table;
		}
		void init();
		void updateInfo(DB &db);
		virtual std::string getTableName(){return "";} 
	 	void execAllField(EachFieldInfo *info);
		Table(){};
		virtual ~Table(){}
		std::vector<MYSQL_BIND> binds;
		std::vector<unsigned long> lengths;
		std::vector<my_bool> bools;
		void updateBinds(unsigned int index);
		void updateBinds();
		virtual void bind(int &value,unsigned int index)
		{
			updateBinds(index);
			MYSQL_BIND * bind = &binds[index];
			if (!bind) return;
			bind->buffer_type= MYSQL_TYPE_LONG;
			bind->buffer= (void *)&value;
			bind->length = &lengths[index];
			bind->is_null = &bools[index];
			printf("%u bind int \n",index);
		}
		virtual void write(int &value,unsigned int index,MYSQL_STMT *stmt)
		{}
		virtual void read(int &value,unsigned int index,MYSQL_STMT *stmt)
		{}
		virtual void bind(std::string &value,unsigned int index);
		virtual void write(std::string &value,unsigned int index,MYSQL_STMT *stmt);
		virtual void read(std::string &value,unsigned int index,MYSQL_STMT *stmt);
		virtual void bind(std::vector<char> &value,unsigned int index);
		virtual void write(std::vector<char> &value,unsigned int index,MYSQL_STMT *stmt);
		virtual void read(std::vector<char> &value,unsigned int index,MYSQL_STMT *stmt);
		template<int size>
		void bind(char (& value)[size],unsigned int index) // FIELD_TYPE_LONG
		{
			updateBinds(index);
			MYSQL_BIND * bind = &binds[index];
			bind->buffer_type= MYSQL_TYPE_STRING;
			lengths[index] = size;
			bind->buffer = value;
			bind->length = &lengths[index];
			bind->is_null = &bools[index];
			bind->buffer_length = size;
			printf("%u bind array \n",index);
		}
		char buffer[10];
		template<int size>
		void write(char (& value)[size],unsigned int index,MYSQL_STMT *stmt)
		{
			return;
		}
		template<int size>
		void read(char (& value)[size],unsigned int index,MYSQL_STMT *stmt)
		{
			return;
		}
		bool load(DB &db,const char *whereStr,ExecEachTable *callback = NULL);
		bool add(DB &db);
		bool update(DB &db,const char *whereStr);
		virtual int getFieldSize(){return 1;}
	};
	#define DEC_TABLE \
		static mysql::TableInfo tableInfo;\
		int getFieldSize(){return tableInfo.fields.size();}\
		void __traceInfo__(int __traceType__,mysql::EachFieldInfo *exec = NULL,MYSQL_STMT *stmt = NULL,const std::string &targetName= "");\
		std::string getTableName();
	#define TABLE_BEGIN(CLASS)\
		mysql::TableInfo CLASS::tableInfo;\
		std::string CLASS::getTableName()\
		{\
			std::string str = #CLASS;\
			std::transform(str.begin(),str.end(),str.begin(),toupper);\
			return str;\
		}\
		void CLASS::__traceInfo__(int __traceType__,mysql::EachFieldInfo *exec,MYSQL_STMT *stmt,const std::string &targetName)\
		{if (!tableInfo.fields.empty() && __traceType__ == ADD_FIELD) return; \
		tableInfo.name = #CLASS;\
		if (exec) exec->begin();\
		unsigned int index = 0;
	#define FIELD(fieldName) \
	switch(__traceType__)\
	{\
	       case ADD_FIELD:\
	       { \
		       tableInfo.fields.push_back(mysql::FieldInfo(#fieldName,fieldName));\
	       }break;\
		case BIND_FIELD:\
		{\
			bind(fieldName,index);\
		}break;\
		case WRITE_FIELD:\
		{\
			write(fieldName,index,stmt);\
		}break;\
		case READ_FIELD:\
		{\
			read(fieldName,index,stmt);\
		}break;\
		case WRITE_FIELD_BYNAME:\
		{\
			if (targetName == #fieldName) write(fieldName,index,stmt);\
		}break;\
	}index++;
	#define TABLE_END \
	tableInfo.execAll(exec);\
	if (exec)exec->end();}
};

