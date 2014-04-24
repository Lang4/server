#pragma once
/**
 * Author jijinlong 
 **/
extern "C"{
#include "sqlite3.h"
}
#include <sstream>
#include <vector>
#include <map>
struct stFieldInfo{
    std::string name;
    std::string type;
    std::string desc;
    stFieldInfo(const char *idStr,int id)
    {
        name = idStr;
        type = "int(10)";
    }
	stFieldInfo(const char *idStr,const std::string& id)
    {
        name = idStr;
        type = "text";
    }
	stFieldInfo(const char *idStr,const std::vector<unsigned char> &content)
	{
		name = idStr;
		type = "blob";
	}
    stFieldInfo()
    {}
    bool equal(const stFieldInfo &fieldInfo)
    {
        return fieldInfo.type == type;
    }
	std::string getFieldDesc()
	{
		std::stringstream ss;
		ss << " "<<name<<"  " << type <<" " << desc;
		return ss.str();
	}
};
struct stEachFieldInfo{
    virtual void callback(stFieldInfo &field)
    {

    }
    virtual void end()
	{}
	virtual void begin()
	{}
};
struct stTableInfo{
    std::string tableName;
    std::vector<stFieldInfo> fields;
    stFieldInfo *findField(const std::string &fieldName)
    {
        for (std::vector<stFieldInfo>::iterator iter = fields.begin(); iter != fields.end();++iter)
        {
            if (iter->name == fieldName)
            {
                return &(*iter);
            }
        }
        return NULL;
    }
	void execAll(stEachFieldInfo *exec)
	{
		if (!exec) return;
		for (std::vector<stFieldInfo>::iterator iter = fields.begin(); iter != fields.end();++iter)
        {
            exec->callback(*iter);
        }
	}
	std::string getKeyName()
	{
		if (keyName == "" && fields.size())
		{
			return fields[0].name;
		}
		return keyName;
	}
	std::string keyName;
};

class DB{
public:
	DB()
	{
		sqlite = NULL;
	}
    sqlite3 *sqlite; // sqlite3 操作对象
    void destroy()
	{
		sqlite3_close(sqlite);
	}
    bool init(const char *name)
    {
        const char *errMsg = NULL;
        sqlite3_stmt* pStmt = NULL;
        int nResult = sqlite3_open(name,&sqlite);
        if (nResult != SQLITE_OK)
        {
          //  std::cout<<"打开数据库失败："<<sqlite3_errmsg(sqlite)<<std::endl;
            return false;
        }
        int nRtn = sqlite3_prepare_v2(sqlite,"select tbl_name from SQLITE_MASTER where type='table'",-1,&pStmt,&errMsg);
        if(nRtn == SQLITE_OK)
        {
            nRtn = sqlite3_step(pStmt);
            while (SQLITE_ROW == nRtn)
            {
                int nCol = sqlite3_column_count(pStmt);
                
                for( int i=0; i<nCol; i++){
                      const char* name = (char *)sqlite3_column_text(pStmt, i);
                    stTableInfo tableInfo;
                    tableInfo.tableName = name;
                    // 获取字段名字
                    sqlite3_stmt* pStmt = NULL;
                    char buff[1023]={'\0'};
                   sprintf(buff,"select * from %s;",name);
                //    printf("now table is:%s\n",name);
                    int nRtn = sqlite3_prepare_v2(sqlite,buff,-1,&pStmt,&errMsg);
                    if(nRtn == SQLITE_OK)
                    {
                        nRtn = sqlite3_step(pStmt);
                        if (SQLITE_DONE == nRtn || nRtn == SQLITE_ROW)
                        {
                            int nCol = sqlite3_column_count(pStmt);
                            for( int i=0; i<nCol; i++)
                            {
                                const char *name = (char *)sqlite3_column_name(pStmt, i);
                                std::string stype = sqlite3_column_decltype(pStmt,i);
                                std::string v_field(name);
                                stFieldInfo fieldInfo;
                                fieldInfo.name = name;
                                fieldInfo.type = stype;
                                tableInfo.fields.push_back(fieldInfo);
                             //   printf("now row is:<%s,%s>",name?name:"null",stype.c_str());
                            }
                        }
                        else{
                          //  printf("sqlite down error:%s\n",name);
                        }
                    }
                    else{
                        printf("get fields failed! %s\n",name);
                    }
                    sqlite3_finalize(pStmt);
                    tables[name] = tableInfo;
                }
                
                nRtn = sqlite3_step(pStmt);
            }
            sqlite3_finalize(pStmt);
        }
        else
        {
            return false;
        }
        return true;
    }
    stFieldInfo *findField(const std::string &tableName,const std::string &fileName)
    {
        stTableInfo *tableInfo = findTable(tableName);
        if (tableInfo)
        {
            return tableInfo->findField(fileName);
        }
        return NULL;
    }

    stTableInfo * findTable(const std::string &tableName)
    {
        TABLES_ITER iter = tables.find(tableName);
        if (iter != tables.end())
        {
            return &iter->second;
        }
        return NULL;
    }
    std::map<std::string,stTableInfo> tables;
	bool execSql(const std::string &name)
	{
		char * errmsg = NULL;
		int result = sqlite3_exec( sqlite,name.c_str(), NULL, NULL, &errmsg );
		if(result == SQLITE_OK )
		{
			return true;
		}
		return false;

	}

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
    typedef std::map<std::string,stTableInfo>::iterator TABLES_ITER;
};

template<class Table>
struct stCreateTableInfo:public stEachFieldInfo{
    void callback(stFieldInfo &field)
    {
        createInfo << field.getFieldDesc() <<",";
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
    Table &table;
	stCreateTableInfo(DB &db,Table &table):db(db),table(table)
	{}
};
template<class Table>
struct stModifyTableInfo:public stEachFieldInfo{
	void begin(){}
    void callback(stFieldInfo &field)
    {
        stFieldInfo *target = db.findField(table.getTableName(),field.name);
		std::stringstream modifyInfo;
        if (!target)
        {
             modifyInfo <<"ALTER TABLE " <<   table.getTableName() <<" add " << field.getFieldDesc() <<";";
        }
        else if (!target->equal(field))
        {
           
        }
		if (modifyInfo.str() != "")
		db.execSql(modifyInfo.str());
    }
	void end()
	{
		
	}
    stModifyTableInfo(DB &db,Table &table):db(db),table(table)
	{}
	DB &db;
    Table &table;
};
template<class Table>
struct stMakeInsertTableInfo:public stEachFieldInfo
{
	void begin()
	{
		count = 0;
		makeStrBuffer<<"insert into " <<  table.getTableName() << " (" ;
	}
	void end()
	{
	
	}
	void callback(stFieldInfo &field)
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
	stMakeInsertTableInfo(DB &db,Table &table):db(db),table(table)
	{
		count = 0;
	}
	std::stringstream makeStrBuffer;
	DB &db;
    Table &table;
	int count;
};
template<class Table>
struct stMakeUpdateTableInfo:public stEachFieldInfo{
	void begin()
	{
		count = 0;
		updateStrBuffer<<"update " << table.getTableName() << " set " ;
	}
	void end()
	{
	
	}
	void callback(stFieldInfo &field)
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
	stMakeUpdateTableInfo(DB &db,Table &table):db(db),table(table)
	{
		count = 0;
	}
	std::stringstream updateStrBuffer;
	DB &db;
    Table &table;
	int count;
	
};
class Table{
public:
	virtual std::string getTableName()
	{
		return "";
	}
	virtual std::string getKeyName()
	{
		return "";
	}
    void init()
    {
        traceInfo(ADD_FIELD,NULL);
    }
    enum TRACE_TYPE{
        ADD_FIELD = 0,
        BIND_READ_FILED = 1,
		TRACE_FIELD = 2,
		BIND_WRITE_FILED = 3, // 写字�?
    };
    static Table * create(DB &db)
    {
        Table *table = new Table();
		table->init();
        table->updateInfo(db);
        return table;
    }
	void updateInfo(DB &db)
	{
		 // 通过db 创建table
		stTableInfo *tableInfo = db.findTable(getTableName());
        if (!tableInfo) // 无表
        {
            // 构建创建表信�?创建�?
            stCreateTableInfo<Table> createTable(db,*this);
            execAllField(&createTable);
        }
        else
        {
            stModifyTableInfo<Table> eachExecFiled(db,*this);
            execAllField(&eachExecFiled);
        }
	}
    void execAllField(stEachFieldInfo *info)
    {
        traceInfo(TRACE_FIELD,NULL,info);
    }

    virtual int traceInfo(TRACE_TYPE __traceType__,sqlite3_stmt *__pStmt__,stEachFieldInfo *exec = NULL,const char *keyName = NULL,int __pos_arg__ = 0)
    {
		return 0;
    }
    bool load(DB &db,const char *whereStr)
    {
        // select 
        sqlite3_stmt *pStmt = NULL;
        std::stringstream ss;
        ss << "select * from "<<getTableName() <<" "<< whereStr<<" ";
        printf("\nselect :%s\n",ss.str().c_str());
        int nRtn = sqlite3_prepare_v2(db.sqlite,ss.str().c_str(),-1,&pStmt,NULL);
        if (nRtn == SQLITE_OK)
        {
            nRtn = sqlite3_step(pStmt);
            if (nRtn == SQLITE_ROW)
            {
                traceInfo(BIND_READ_FILED,pStmt);
                nRtn = sqlite3_step(pStmt);
				sqlite3_finalize(pStmt);
				return true;
            }
            sqlite3_finalize(pStmt);
        }
		return false;
    }
	void save(DB &db)
	{
		// 走insert 流程	
		stMakeInsertTableInfo<Table> makeInfo(db,*this);
		execAllField(&makeInfo);
		sqlite3_stmt *pStmt = NULL;
		makeInfo.makeStrBuffer << ") values (";
		for (unsigned int index = 0; index < makeInfo.count; ++index)
		{
			if (index != 0)
				makeInfo.makeStrBuffer << ",?";
			else
				makeInfo.makeStrBuffer << "?";
		}
		makeInfo.makeStrBuffer <<")";

		int nRtn = sqlite3_prepare(db.sqlite,makeInfo.makeStrBuffer.str().c_str(),-1,&pStmt,0);
		if (nRtn == SQLITE_OK)
		{
			traceInfo(BIND_WRITE_FILED,pStmt);
			nRtn = sqlite3_step(pStmt);
			if (nRtn == SQLITE_DONE)
			{
				sqlite3_finalize(pStmt);
				return;
			}
		}
		if (pStmt)
			sqlite3_finalize(pStmt);

		return ;
	}
	void update(DB &db)
	{
		// 执行update 流程
		stMakeUpdateTableInfo<Table> updateInfo(db,*this);
		execAllField(&updateInfo);
		sqlite3_stmt *pStmt = NULL;
		std::string keyName = getKeyName();
		if (keyName == "") return;
		updateInfo.updateStrBuffer<<" where "<<keyName<<" = ?";
		
		// 执行 stmt
		int nRtn = sqlite3_prepare(db.sqlite,updateInfo.updateStrBuffer.str().c_str(),-1,&pStmt,0);
		if (nRtn == SQLITE_OK)
		{
			int pos = traceInfo(BIND_WRITE_FILED,pStmt);
			traceInfo(BIND_WRITE_FILED,pStmt,NULL,keyName.c_str(),pos);
			nRtn = sqlite3_step(pStmt);
			if (nRtn == SQLITE_DONE)
			{
				sqlite3_finalize(pStmt);
				return;
			}
			
		}
		sqlite3_finalize(pStmt);
		return;
		
	}
	template<class CLASS>
    void bind(CLASS &value,int pos,sqlite3_stmt *pStmt,TRACE_TYPE traceType)
    {
        if (!pStmt) return;
        switch(traceType)
        {
            case BIND_READ_FILED:
            {
                value = sqlite3_column_int(pStmt,pos);
            }break;
			case BIND_WRITE_FILED:
			{
				sqlite3_bind_int(pStmt,pos,value);
			}break;
        }
    }

	/**
	 * 绑定string
	 */
	void bind(std::string &values,int pos,sqlite3_stmt *pStmt,TRACE_TYPE traceType)
	{
		if (!pStmt) return;
		switch(traceType)
        {
            case BIND_READ_FILED:
            {
                const char * txt = (const char *) sqlite3_column_text(pStmt,pos);
				int tlen = sqlite3_column_bytes(pStmt,pos);
				values = txt;
            }break;
			case BIND_WRITE_FILED:
			{
				sqlite3_bind_text(pStmt,pos,values.c_str(),values.size(),0);
			}break;
        }
	}
	/**
	 * 绑定字符�?
	 */
	void bind(std::vector<unsigned char > &content,int pos,sqlite3_stmt *pStmt,TRACE_TYPE traceType)
	{
		if (!pStmt) return;
		switch(traceType)
        {
            case BIND_READ_FILED:
            {
               const char *data = (const char *) sqlite3_column_blob(pStmt,pos);
				int tlen = sqlite3_column_bytes(pStmt,pos);
				if (tlen)
				{
					content.resize(tlen);
					memcpy(&content[0],data,tlen);
				}
            }break;
			case BIND_WRITE_FILED:
			{
				if (content.size())
					sqlite3_bind_blob(pStmt,pos,&content[0],content.size(),0);
			}break;
        }
	}
};

#define DEC_TABLE \
	virtual std::string getTableName()\
	{\
		return tableInfo.tableName;\
	}\
	virtual std::string getKeyName()\
	{\
		return tableInfo.getKeyName();\
	}\
	static stTableInfo tableInfo;\
    int traceInfo(TRACE_TYPE __traceType__,sqlite3_stmt *__pStmt__,stEachFieldInfo *exec = NULL,const char *keyName = NULL,int __pos_arg__ = 0);

#define TABLE_BEGIN(CLASS)\
	stTableInfo CLASS::tableInfo;\
	int CLASS::traceInfo(TRACE_TYPE __traceType__,sqlite3_stmt *__pStmt__,stEachFieldInfo *exec,const char *keyName ,int __pos_arg__)\
	{if (!tableInfo.fields.empty() && __traceType__ == ADD_FIELD) return 0;\
		else if (tableInfo.fields.empty())\
		tableInfo.tableName = #CLASS;\
        int __pos__ = __pos_arg__;\
		if (exec) exec->begin();\

#define FIELD(fieldName) \
	  switch(__traceType__)\
        {\
            case ADD_FIELD:\
            { \
				tableInfo.fields.push_back(stFieldInfo(#fieldName,fieldName));\
            }break;\
            case BIND_READ_FILED:\
			{\
				if (!keyName || !strcmp(keyName,#fieldName))\
					bind(fieldName,__pos__++,__pStmt__,__traceType__);\
            }break;\
			case BIND_WRITE_FILED:\
			{\
				if (!keyName || !strcmp(keyName,#fieldName))\
					bind(fieldName,++__pos__,__pStmt__,__traceType__);\
			}break;\
        }\

#define TABLE_END\
		tableInfo.execAll(exec);\
		if (exec)exec->end();\
		return __pos__;}\

