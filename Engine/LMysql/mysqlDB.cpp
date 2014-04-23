#include "mysqlDB.h"
#include <sstream>
namespace mysql{
	std::string FieldInfo::getDesc()
	{
		std::string ss;
		ss += name;
		switch(type)
		{
			case FIELD_TYPE_BLOB:
			{
				ss += " blob ";
			}break;
			case FIELD_TYPE_STRING:
			{
				ss +=" text ";
			}break;	
			case FIELD_TYPE_LONG:
			{
				ss +=" int(10) ";
			}break;
		}
		return ss;
	}
	bool FieldInfo::equal(FieldInfo &info)
	{
		std::string i = info.getDesc();
		return i == getDesc();
	}
	FieldInfo::FieldInfo(const std::string &name,int type)
	{
		this->name = name;
		this->type = FIELD_TYPE_LONG;	
	}
	FieldInfo::FieldInfo(const std::string &name,std::string& value)
	{
		this->name = name;
		this->type = FIELD_TYPE_STRING;
	}
	FieldInfo::FieldInfo(const std::string &name,std::vector<char> &buffer)
	{
		this->name = name;
		this->type = FIELD_TYPE_BLOB;
	}
	FieldInfo * TableInfo::findField(const std::string &fieldName)
	{
		for (std::vector<FieldInfo>::iterator iter = fields.begin(); iter != fields.end();++iter)
		{
			if (iter->name == fieldName)
			{
				return &(*iter);
			}
		}
		return NULL;
	}	
	void TableInfo::execAll(EachFieldInfo *exec)
	{
		if (!exec) return;
		for (std::vector<FieldInfo>::iterator iter = fields.begin(); iter != fields.end();++iter)
		{
			exec->callback(*iter);
		}
	}
	bool Connection::connect()
	{
		if (mysql)
		{
			close();
		}

		mysql = mysql_init(NULL);
		if (!mysql)
		{
			printf("MYSQL mysql_init failed\n");
			return false;
		}	

		if (!mysql_real_connect(mysql, connections->host.c_str(), 
						connections->username.c_str(), 
						connections->passwd.c_str(), 
						connections->dbname.c_str(), 
						3306, 
						NULL,
						CLIENT_COMPRESS | CLIENT_MULTI_STATEMENTS | CLIENT_INTERACTIVE))
		{
			printf("%s\n", mysql_error(mysql));
			printf("MYSQL connect failed\n");
			return false;
		}

		state = IDLE;
		
		printf("MYSQL connect success. version:%s, thread_safe:%s\n", mysql_get_client_info(), mysql_thread_safe()?"YES":"NO");
		return true;
	}
	Connection* Connections::getConnect()
	{
		Connection *conn = NULL;

//		printf("%s enter\n", __PRETTY_FUNCTION__);
		std::list<Connection *>::iterator iter = conns.begin(), tmp = iter;
		for ( ; iter != conns.end(); )
		{
			Connection *c = *iter;
			if (!c) continue;

			if (c->getState() == USING)
			{
				
			}
			else if (c->getState() == IDLE)
			{
				if (c->ping())
				{
					//重连
					if (false == c->connect())
					{
						tmp = iter++;
//						printf("%s continue %p", __PRETTY_FUNCTION__, *tmp);
						delete (*tmp);
						conns.erase(tmp);
						continue;
					}
//					printf("%s reconnect %p", __PRETTY_FUNCTION__, c);
				}
//				printf("%s ping %p", __PRETTY_FUNCTION__, c);

				c->setState(USING);
				return c;
			}	
			iter++;
		}
		if (!conn)
		{
			conn = new Connection(this);
			if (conn->connect())
			{
				conns.push_back(conn);
			}
			else
			{
				delete conn;
				conn = NULL;
			}
//			printf("%s !conn:%p\n", __PRETTY_FUNCTION__, conn);
		}
		if (conn)
		{	
//			printf("%s conn:%p\n", __PRETTY_FUNCTION__, conn);
		}	

//		printf("%s return conn:%p\n", __PRETTY_FUNCTION__, conn);

		return conn;
	}
	void Connections::putConnect(Connection *conn)
	{
		if (conn)
			conn->setState(IDLE);
	}
	MYSQL * DB::getHandle()
	{
		if (nowConnection)
		{
			return nowConnection->mysql;
		}else{
			nowConnection = connections.getConnect();
			if (nowConnection)
			{
				return nowConnection->mysql;
			}
		}
		return NULL;
	}
	void DB::putHandle()
	{
		if (nowConnection){
			connections.putConnect(nowConnection);
		}
		nowConnection = NULL;
	}
	bool DB::init(const char *host,const char * dbname,const char *username,const char *pwd)
	{
		nowConnection = NULL;
		connections.host = host;
		connections.dbname = dbname;
		connections.username = username;
		connections.passwd = pwd;
		MYSQL * mysql = getHandle();
		MYSQL_RES *tables = NULL;
		tables = mysql_list_tables(mysql,NULL);
		if (tables)
		{
			MYSQL_ROW table;
			while ((table = mysql_fetch_row(tables)))
			{
				TableInfo tableInfo;
				tableInfo.name = table[0];
				MYSQL_RES *fields = mysql_list_fields(mysql,table[0],NULL);
				unsigned int fields_num = 0;
				if (fields)
				{
					fields_num = mysql_num_fields(fields);
					MYSQL_FIELD * mysql_fields = mysql_fetch_fields(fields);
					if (!mysql_fields)
					{
						mysql_free_result(fields);
						putHandle();
						return false;
					}
					for (unsigned int i = 0; i < fields_num;i++)
					{
						MYSQL_FIELD field = mysql_fields[i];
						FieldInfo fieldInfo;
						fieldInfo.name = field.name;
						fieldInfo.type = field.type;
						tableInfo.fields.push_back(fieldInfo);
						printf("table:%s get field: name:%s,type:%u\n",tableInfo.name.c_str(),field.name,field.type);
					}
					mysql_free_result(fields);
				}
				std::transform(tableInfo.name.begin(),tableInfo.name.end(),tableInfo.name.begin(),toupper);
				tablesInfo[tableInfo.name] = tableInfo;
			}
			mysql_free_result(tables);
		}
		putHandle();
		return true;
	}
	bool DB::execSql(const std::string& sql)
	{
		MYSQL * mysql = getHandle();
		mysql_query(mysql,sql.c_str());
		putHandle();
		return true;
	}
	TableInfo * DB::getTable(const std::string &name)
	{
		TABLES_INFO_ITER iter = tablesInfo.find(name);
		if (iter != tablesInfo.end())
		{
			return &iter->second;
		}
		return NULL;
	}
	FieldInfo * DB::findField(const std::string &tableName,const std::string &fileName)
	{
		TableInfo *tableInfo = findTable(tableName);
		if (tableInfo)
		{
			return tableInfo->findField(fileName);
		}
		return NULL;
	}
	TableInfo * DB::findTable(const std::string &tableName)
	{
		TABLES_INFO_ITER iter = tablesInfo.find(tableName);
		if (iter != tablesInfo.end())
		{
			return &iter->second;
		}
		return NULL;
	}
	void Table::init()
	{
		__traceInfo__(ADD_FIELD,NULL);
		updateBinds();
	}
	void Table::updateInfo(DB &db)
	{
		TableInfo *tableInfo = db.findTable(getTableName());
		if (!tableInfo)
		{
			CreateTableInfo<Table> createTable(db,*this);
			execAllField(&createTable);
		}
		else
		{
			ModifyTableInfo<Table> eachExecFiled(db,*this);
			execAllField(&eachExecFiled);
		}
	}
	void Table::execAllField(EachFieldInfo *info)
	{
		__traceInfo__(TRACE_FIELD,info);	
	}
	bool Table::load(DB &db,const char *whereStr,ExecEachTable *callback)
	{
		MYSQL_STMT *select_stmt = mysql_stmt_init(db.getHandle());
        	MakeSelectTableInfo<Table> makeInfo(db,*this);
		execAllField(&makeInfo);	
		printf("\nselect :%s\n",makeInfo.makeStrBuffer.str().c_str());
		mysql_stmt_prepare(select_stmt, makeInfo.makeStrBuffer.str().c_str(), makeInfo.makeStrBuffer.str().size());
		
		MYSQL_RES *prepare_meta_result = mysql_stmt_result_metadata(select_stmt);
		if (!prepare_meta_result)
		{
			printf( "1 %s\n", mysql_stmt_error(select_stmt));
			mysql_stmt_close(select_stmt);
			db.putHandle();
			return false;
		}

		if (mysql_stmt_execute(select_stmt))
		{
			printf( "2 %s\n", mysql_stmt_error(select_stmt));
			mysql_free_result(prepare_meta_result);
			mysql_stmt_close(select_stmt);
			db.putHandle();
			return false;
		}
		__traceInfo__(BIND_FIELD,NULL);
		if (binds.empty()) {
			db.putHandle();
			mysql_free_result(prepare_meta_result);
			mysql_stmt_close(select_stmt);
			return false;
		}
		if(mysql_stmt_bind_result(select_stmt, &binds[0]))
		{
			printf("3 %s\n", mysql_stmt_error(select_stmt));
			mysql_free_result(prepare_meta_result);
			mysql_stmt_close(select_stmt);
			db.putHandle();
			return false;
		}	
		int column_count = mysql_num_fields(prepare_meta_result);
		printf("colume count:%d \n",column_count);
		/* Now buffer all results to client (optional step) */
		if (mysql_stmt_store_result(select_stmt))
		{
			printf( "4 %s\n", mysql_stmt_error(select_stmt));
			mysql_free_result(prepare_meta_result);
			mysql_stmt_close(select_stmt);
			db.putHandle();
			return false;
		}
		int count = 0;
		for (;;)
    	{
       		int ret = mysql_stmt_fetch(select_stmt);
			printf("5 %s \n",mysql_stmt_error(select_stmt));
			if (ret == MYSQL_NO_DATA) break;
      		if (ret!=0 && ret!=MYSQL_DATA_TRUNCATED) break;
			__traceInfo__(READ_FIELD,NULL,select_stmt);	
			if (callback)
			{
				callback->exec(this);
			}
			count++;
		}
		printf("load count %u\n",count);
		mysql_free_result(prepare_meta_result);
		mysql_stmt_close(select_stmt);
		db.putHandle();
		if (count)return true;
		return false;
	}
	void Table::updateBinds(unsigned int index)
	{
		memset(&binds[index],0,sizeof(MYSQL_BIND));
		lengths[index] = 0;
		bools[index] = 0;
	}
	void Table::updateBinds()
	{
		binds.resize(getFieldSize());
		lengths.resize(getFieldSize());
		bools.resize(getFieldSize());
	}
	bool Table::add(DB &db)
	{
		MYSQL_STMT *insert_stmt =  mysql_stmt_init(db.getHandle());
		MakeInsertTableInfo<Table> makeInfo(db,*this);
		execAllField(&makeInfo);
		makeInfo.makeStrBuffer << ") values (";
		for (unsigned int index = 0; index < makeInfo.count; ++index)
		{
			if (index != 0)
				makeInfo.makeStrBuffer << ",?";
			else
				makeInfo.makeStrBuffer << "?";
		}
		makeInfo.makeStrBuffer <<")";
		
		mysql_stmt_prepare(insert_stmt, makeInfo.makeStrBuffer.str().c_str(), makeInfo.makeStrBuffer.str().size());
		
		__traceInfo__(BIND_FIELD,NULL);	
		
		if (mysql_stmt_bind_param(insert_stmt, &binds[0]))
		{
			printf(" %s\n", mysql_stmt_error(insert_stmt));
			db.putHandle();
			return false;
		}
		int param_count= mysql_stmt_param_count(insert_stmt);
		fprintf(stdout, " total parameters in SELECT: %d\n", param_count);			
		__traceInfo__(WRITE_FIELD,NULL,insert_stmt);
		/* Execute the INSERT statement - 1*/
		if (mysql_stmt_execute(insert_stmt))
		{
			fprintf(stderr, " %s\n", mysql_stmt_error(insert_stmt));
			db.putHandle();
			return false;
		}

		/* Get the number of affected rows */
		my_ulonglong affected_rows = mysql_stmt_affected_rows(insert_stmt);
		if (affected_rows != 1) /* validate affected rows */
		{
			fprintf(stderr, " %s\n", mysql_stmt_error(insert_stmt));
			db.putHandle();
			return false;
		}
		db.putHandle();
		return true;
	}
	bool Table::update(DB &db,const char *keyName)
	{
		MakeUpdateTableInfo<Table> updateInfo(db,*this);
		execAllField(&updateInfo);
		updateInfo.updateStrBuffer<<" where "<<keyName<<" = ?";
		
		MYSQL_STMT *update_stmt = mysql_stmt_init(db.getHandle());
		mysql_stmt_prepare(update_stmt, updateInfo.updateStrBuffer.str().c_str(), updateInfo.updateStrBuffer.str().size());
		__traceInfo__(BIND_FIELD,NULL);
		
		if (mysql_stmt_bind_param(update_stmt,&binds[0]))
		{
			fprintf(stderr, "1 %s\n", mysql_stmt_error(update_stmt));
			db.putHandle();
			return false;
		}
		__traceInfo__(WRITE_FIELD,NULL,update_stmt);
		__traceInfo__(WRITE_FIELD_BYNAME,NULL,update_stmt,keyName);
		/* Execute the INSERT statement - 1*/
		if (mysql_stmt_execute(update_stmt))
		{
		  	fprintf(stderr, "2 %s\n", mysql_stmt_error(update_stmt));
			db.putHandle();
			return false;
		}
		my_ulonglong affected_rows = mysql_stmt_affected_rows(update_stmt);
		if (affected_rows != 1) /* validate affected rows */
		{
			printf( "affect row %s %u\n", mysql_stmt_error(update_stmt),affected_rows);
			db.putHandle();
			return false;
		}
		else
		{
			printf("affact row :%u\n",affected_rows);
		}
		db.putHandle();
		return true;
	}
	void Table::bind(std::string &value,unsigned int index)
	{
		updateBinds(index);
		MYSQL_BIND * bind = &binds[index];
		bind->buffer_type = MYSQL_TYPE_STRING;
		bind->buffer = buffer;//value.size() ? &value[0]:NULL;
		bind->length = &lengths[index];
		bind->is_null = &bools[index];
		bind->buffer_length = 10;//value.size()?value.size():0;
		printf("%u bind string \n",index);
	}
	void Table::write(std::string &value,unsigned int index,MYSQL_STMT *stmt)
	{
		if (value.size())
		mysql_stmt_send_long_data(stmt,index ,&value[0],value.size());
	}
	void Table::read(std::string &value,unsigned int index,MYSQL_STMT *stmt)
	{
		if (lengths[index])
		{
			MYSQL_BIND * result = &binds[index];
			result->buffer_length = lengths[index];
			value.resize(result->buffer_length);
			result->buffer = &value[0];
			int ret = mysql_stmt_fetch_column(stmt, result, index, 0);
			if (ret!=0)
			{
			}
		}
	}
	void Table::bind(std::vector<char> &value,unsigned int index)
	{
		updateBinds(index);
		MYSQL_BIND * bind = &binds[index];
		bind->buffer_type = MYSQL_TYPE_LONG_BLOB;
		bind->buffer = buffer;//value.size() ? &value[0]:NULL;
		bind->length = &lengths[index];
		bind->is_null = &bools[index];
		bind->buffer_length = 10;//value.size() ? value.size():0;
		printf("%u bind vector \n",index);
	}
	void Table::write(std::vector<char> &value,unsigned int index,MYSQL_STMT *stmt)
	{
		if (value.size())
			mysql_stmt_send_long_data(stmt,index , &value[0],value.size());
	}
	void Table::read(std::vector<char> &value,unsigned int index,MYSQL_STMT *stmt)
	{
		if (lengths[index])
		{
			MYSQL_BIND * result = &binds[index];
			//char * value = new char[lengths[index]];
			value.resize(lengths[index]);
			result->buffer = &value[0];
			result->buffer_length = lengths[index];

			int ret = mysql_stmt_fetch_column(stmt, result, index, 0);
			if (ret!=0)
			{
			}
			printf("read binary :%u\n",lengths[index]);
		}
	}

};
