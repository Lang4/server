/**
 * xmlscript for C++   simple and powerful
 *****************************************************************      
 * <code name="11" lib="NPCLIB"><and><npc.talk value=""/></and></code>
 * or 
 * code(name="11",lib="NPCLIB"){and(){npc.talk(value="");}}
 *****************************************************************
 * class SceneNpc{
 * public:
 *	 int talk(Args *stub,Properties &propies)
 *   {
 *       printf("%s %u\n", "print",propies.getInt("value"));
 *       return CodeState::YES;
 *   }
 * };
 *****************************************************************
 * XML_SCRIPT(NPCLIB)
 * {
 *		bind(SceneNpc,talk);
 * }
 *****************************************************************
 * Args args;
 * SceneNpc *npc = new SceneNpc();
 * args.add("npc",npc);
 * theScript.exec("11",&args);
 *****************************************************************
 * Author jijinlong Date 2014/3/4
 * email:jjl_2009_hi@163.com
 **/
#pragma once
#include "LNode.h"
#include <map>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include "Code.h"
#include "LCalc.h"

#define _DEBUG 0 

class ObjectWithType{
public:
    void *object;
    int objectType;
	std::string objectName;
    ObjectWithType()
    {
        object = NULL;
        objectType = 0;
    }
    ObjectWithType(void *object,int objectType,const std::string &objectName):
        object(object),objectType(objectType),objectName(objectName)
    {}
};
enum InnerTypeValue{
    STRING = 0,
    NUMBER = 1,
};
class Code;
class CodeNode;
typedef std::vector<ObjectWithType> OBJECTS;

class ArgFrame{
public:
	std::vector<std::string> names;
	void push(const std::string &name){
		names.push_back(name);
	}
	void clear()
	{
		names.clear();
	}
	ArgFrame *clone()
	{
		ArgFrame *frame = new ArgFrame;
		frame->names = names;
		return frame;
	}
};
class Args{
public:
    std::map<std::string,OBJECTS> args;
    typedef std::map<std::string,OBJECTS>::iterator ARGS_ITER;
	std::map<std::string,std::vector<float> > numbers; // 临时数值
	typedef std::map<std::string,std::vector<float> >::iterator NUMBERS_ITER;
	
	std::map<std::string,OBJECTS > objectss; // 临时集合
	typedef std::map<std::string,OBJECTS>::iterator OBJECTSS_ITER;	
	OBJECTS * getObjects(const std::string& name)
	{
		OBJECTSS_ITER iter = objectss.find(name);
		if (iter != objectss.end())
		{
			return &iter->second;
		}
		return NULL;
	}
	OBJECTS * addObjects(const std::string &name)
	{
		return &objectss[name];	
	}
	void setNumber(const std::string &name,float value)
	{
		if (name =="") return;
		numbers[name].push_back(value);
		if (now)
		{
			now->push(name);
		}
	}
	bool getNumber(const std::string &name,float * &value)
	{
		NUMBERS_ITER iter = numbers.find(name);
		if (iter != numbers.end() && iter->second.size())
		{
			value = &iter->second.back();
			return true;
		}
		return false;
	}
	bool popNumber(const std::string &name)
	{
		NUMBERS_ITER iter = numbers.find(name);
		if (iter != numbers.end() && iter->second.size())
		{
			iter->second.pop_back();
			return true;
		}
		return false;
	}
	float getValue(const std::string &name)
	{
		if (name=="") return 0;	
		if (name[0] == '#' && name.size() > 2)
		{
			float *value = 0;
			getNumber(name.substr(1),value);
			if (value)
				return *value;
			return 0;
		}
		return atof(name.c_str());
	}
	bool checkNumber(const std::string &name)
	{
		NUMBERS_ITER iter = numbers.find(name);
		if (iter != numbers.end())
		{
			return iter->second.size();
		}
		return false;

	}
    void add(const char *name,void * arg,int objectType = 0,const std::string objectName="")
    {
        args[name].push_back(ObjectWithType(arg,objectType,objectName));
    }
	void push(const char *name,const ObjectWithType &type)
	{
		args[name].push_back(type);
	}
	void pop(const char *name)
	{
		if (args[name].size())
			args[name].pop_back();
	}
    template<typename CLASS>
    CLASS * get(const char *name,int objectType = 0,const std::string objectName="")
    {
        ARGS_ITER iter = args.find(name);
        if (iter != args.end() && iter->second.size())
        {
			ObjectWithType &object = iter->second.back();
			if (object.objectType == objectType && object.objectName == objectName)
			{
				return static_cast<CLASS*>(object.object);
			}
        }
        return NULL;
    }
    std::string objectName;
    template<typename CLASS>
    CLASS* getObject(int objectType = 0,const std::string &name="")
    {
        return get<CLASS>(objectName.c_str(),objectType,name);
    }
    void *getPointer(const char *name,int objectType = 0,const std::string & objectName="")
    {
        ARGS_ITER iter = args.find(name);
        if (iter != args.end() && iter->second.size())
        {
			ObjectWithType &object = iter->second.back();
			if (object.objectType == objectType && object.objectName == objectName)
			{
				return (object.object);
			}
        }
        return NULL;
    }
	Args()
	{
		codeEnv = NULL;
		now = NULL;
		nowNode = NULL;
	}
	CodeNode *nowNode;
	Args & operator=(const Args &arg)
	{
		codeEnv = arg.codeEnv;
		for (ARGNAMES_ITER iter = arg.argNames.begin(); iter != arg.argNames.end();++iter)
		{
			ArgFrame * temp= (*iter)->clone();
			argNames.push_back(temp);
		}
		if (argNames.size())
		now = argNames.back();
		objectName = arg.objectName;
		objectss = arg.objectss;
		args = arg.args;
		numbers = arg.numbers;
		nowNode = arg.nowNode;
		return *this;
	}
	Code *codeEnv;
	typedef std::vector<ArgFrame*>::const_iterator ARGNAMES_ITER;
	std::vector<ArgFrame*> argNames;
	ArgFrame * now;
	void pushFrame()
	{
		now = new ArgFrame();
		argNames.push_back(now);
	}
	~Args()
	{
		for (ARGNAMES_ITER iter = argNames.begin(); iter != argNames.end();++iter)
		{
			if(*iter) delete *iter;
		}
		argNames.clear();
	}
	void popFrame()
	{
		if (now) 
		{
			for (std::vector<std::string>::iterator iter = now->names.begin(); iter != now->names.end();++iter)
			{
				popNumber(*iter);
			}
			now->clear();
			delete now;
		}
		if (argNames.size())
			argNames.pop_back();
		if (argNames.size())
		{
			now = argNames.back();
		}
		else now = NULL;
	}
	int exec();
};
class Properties{
public:
    std::map<std::string,std::string> propies;

    int getInt(const std::string &name)
    {
        return atoi(propies[name].c_str());
    }
    std::string& getString(const std::string &name)
    {
        return propies[name];
    }
    std::string &get(const std::string &name)
    {
        return propies[name];
    }
    template<typename TYPE>
    TYPE get(const std::string &name)
    {
        std::stringstream ss;
        ss << propies[name];
        TYPE type;
        ss >> type;
        return type;
    }
	template<typename TYPE>
	void set(const std::string &name,TYPE T)
	{
		std::stringstream ss;
		ss << T;
		propies[name] = ss.str();
	}
	static bool checkNumber(const std::string value)
	{
		for (unsigned int index = 0; index <value.size();index++)
		{
			char tag = value[index];
			if (tag == '\0') continue;
			if (tag != '.' && tag != '-' && !(tag>='0' && tag <='9'))
			{
				return false;
			}
		}
		return true;
	}
	std::string getFirstName()
	{
		if (propies.size()) return propies.begin()->first;
		return "";
	}
};
class CodeState{
public:
    enum{
        YES = 1,
        NO = 0,
    };
};

class CodeNode:public CodeState{
public:
    std::vector<CodeNode* > childs; //  
    typedef std::vector<CodeNode* >::iterator CHILDS_ITER;
    std::string nodeName; // 
    virtual int exec(Args*stub){
        for (CHILDS_ITER iter = childs.begin(); iter != childs.end();++iter){
            CodeNode * node = *iter;
            if (node) node->exec(stub);
        }
       return YES;
    }
    virtual ~CodeNode()
    {
        for (CHILDS_ITER iter = childs.begin(); iter != childs.end();++iter){
            CodeNode* node = *iter;
            if (node) delete node;
        }
        childs.clear();
    }

    void addChild(CodeNode * code)
    {
        childs.push_back(code);
    }
	virtual int execChild(Args *stub)
	{
		return exec(stub);
	}
};


class AndNode:public CodeNode{
public:
    virtual int exec(Args *stub)
    {
        for (CHILDS_ITER iter = childs.begin(); iter != childs.end();++iter){
            CodeNode* node = *iter;
            if (node && NO == node->exec(stub)) return NO;
        }
        return YES;
    }
};


class ConditionNode:public AndNode{
public: 
};

class OrNode:public CodeNode{
public:
    virtual int exec(Args* stub)
    {
        for (CHILDS_ITER iter = childs.begin(); iter != childs.end();++iter){
            CodeNode * node = *iter;
            if (node && YES == node->exec(stub)) return YES;
        }
        return NO;
    }
};

class ForNode:public CodeNode{
public:
	std::string start;
	std::string end;
	std::string step;
    ForNode()
    {
    }
    virtual int exec(Args*stub){
		stub->setNumber("index",0);
        for (int index = (int)stub->getValue(start);index <= (int)stub->getValue(end); index += (int)stub->getValue(step))
        {
			float * value = NULL;
			if (stub->getNumber("index",value))
			{
				*value = (float) index;
			}
            for (CHILDS_ITER iter = childs.begin(); iter != childs.end();++iter){
                CodeNode * node = *iter;
                if (node) node->exec(stub);
            }
        }
		stub->popNumber("index");
       return YES;
    }
};
class WhileNode:public CodeNode{
public:
    CodeNode  *condition;
    int state; // YES or NO
    virtual int exec(Args *stub)
    {
        if (!condition) return NO;
        while (condition->exec(stub) == state)
        {
            // 
            for (CHILDS_ITER iter = childs.begin(); iter != childs.end();++iter){
                CodeNode  * node = *iter;
                if (node)
                    node->exec(stub);
            }
        }
       return NO;
    }
    WhileNode(){state =YES;condition =NULL;}
    ~WhileNode(){
       
    }
};

// <foreach targets="" object="" condition=""></foreach>
class ForEachNode:public CodeNode{
public:
	std::string targets; // 集合对象
	std::string objectName; // 对象名字
	CodeNode  *condition; // 条件
	int state; // YES or NO
	ForEachNode()
	{
		condition = NULL;
		state = YES;
	}
	virtual int exec(Args *stub)
	{
		OBJECTS * objects = stub->getObjects(targets);
		if (NULL == objects) return NO;
		for (OBJECTS::iterator iter = objects->begin(); iter != objects->end();++iter)
		{
			if (condition)
			{
				if (condition->exec(stub) != state) break;
			}
			stub->push(objectName.c_str(),*iter); // 压入栈变量
			CodeNode::exec(stub);	
			stub->pop(objectName.c_str()); // 弹出栈变量
		}
		return YES;
	}
};
class LogicFunctionHandler{
public:
    virtual int callback(Args* stub,Properties &propies) = 0;
	virtual ~LogicFunctionHandler(){}
};

class LogicFieldHandler:public LogicFunctionHandler{
public:
    virtual std::string get(Args *stub){return "";} 
    virtual void set(Args* stub,const std::string& value){return;}

    virtual void * getPointer(Args *stub){return NULL;}
    virtual void setPointer(Args *stub,void *pointer){}
	virtual ~LogicFieldHandler(){}
};

class LogicNode:public CodeNode{
public:
    LogicFunctionHandler *handler;
    std::string objectName;
    virtual int exec(Args *stub)  
    {
        Args * temp = static_cast<Args*>(stub);
        temp->objectName = objectName;
		temp->nowNode = this;
		temp->pushFrame(); // 增加临时帧
		if (YES == handler->callback(temp,propies))
		{
			CodeNode::exec(stub);
			temp->popFrame(); // 弹出临时帧
			return YES;
		}
		temp->popFrame(); // 弹出临时帧
		return CodeNode::NO;
    }
	virtual int execChild(Args *stub)
	{
		int state = CodeNode::exec(stub);
		stub->popFrame(); // 弹出临时帧
		return state;
	}
    Properties propies; //
    virtual ~LogicNode()
    {
       // if (handler) delete handler;
       // handler = NULL;
    }
	LogicNode()
	{
		handler = NULL;
	}
};

class CalcNode:public LogicNode,public LCalc{
public:
	Args *stub;
	CalcNode()
	{
	}
	virtual int exec(Args *stub)  
    {
		std::string varName = propies.getFirstName();
		this->stub = stub;
		std::string valueStr = propies.getString(varName);
		if (valueStr=="")
		{
			valueStr = varName;
			varName = "";
		}
		float value = CalcNode::parseString(valueStr.c_str());
		if (value != 0)
		{
			if (varName != "")
				stub->setNumber(varName,value);
			CodeNode::exec(stub);
			if (varName != "")
				stub->popNumber(varName);
			return YES;
		}
		return CodeNode::NO;
    }
	virtual float getValue(const std::string & name)
	{
		if (stub) return stub->getValue(name.c_str());
		return 0;
	}
};

class VarNode:public CalcNode{
public:
    VarNode(){}
    virtual float getValue(const std::string & name)
	{
		if (name =="") return 0; 
		if (name[0] != '$')
		{
			if (stub) return stub->getValue(name.c_str());
		}
		else 
		{
			LogicFieldHandler *dest = static_cast<LogicFieldHandler*>(stub->codeEnv->findHanderByName(name.substr(1)));
			if (dest && stub) return atof(dest->get(stub).c_str());
		}
		return 0;
	}
	virtual int exec(Args *stub)  
    {
		std::string argName = propies.getString("arg");
		stub->objectName = objectName;
		this->stub = stub;
		float value = CalcNode::parseString(propies.getString("value").c_str());
		LogicFieldHandler *src = static_cast<LogicFieldHandler*>(handler);
		if (!src) return NO;
		std::stringstream ss;
		ss << value;
		src->set(stub,ss.str());
		if (argName !="")
		{
			stub->setNumber(argName,value);
		}
		if (value != 0)
		{
			CodeNode::exec(stub);
			if (argName !="")
			{
				stub->popNumber(argName);
			}
			return YES;
		}
		if (argName !="")
		{
			stub->popNumber(argName);
		}
		return CodeNode::NO;
    }
};

class PrintNode:public VarNode{
public:
	// print(#arg hello world $a #index);
	virtual int exec(Args *stub)  
    {
		std::string varName = propies.getFirstName();
		this->stub = stub;
		std::string valueStr = propies.getString(varName);
		if (valueStr=="")
		{
			valueStr = varName;
		}
		printf("%s\n",parseString(valueStr.c_str(),*this).c_str());
		return CodeNode::YES;
    }
	template<typename T>
	static std::string parseString(const char *content,T &t)
	{
		std::string temp;
		std::stringstream ss;
		while(*content != '\0')
		{
			if (*content == '#' || *content == '$'){
				while(*content != ' ' && *content !='\0') 
				{
					temp.push_back(*content);
					content++;
				}
				ss << t.getValue(temp);
				temp.clear();
			}
			if (*content != '\0')
			{
				ss << *content; content++;
			}
		}
		ss<<'\0';
		temp = ss.str();
		return temp;
	}
};
class NotNode:public LogicNode{
public:
    virtual int exec(Args *stub) 
    {
        Args * temp = static_cast<Args*>(stub);
        temp->objectName = objectName;
        if (NO == handler->callback(temp,propies))
		{
			CodeNode::exec(stub);
			return CodeNode::YES;
		}
		return CodeNode::NO;
    }
};
class Util{
public:
    static bool split(const char *dest,char *splittag,std::vector<std::string> &out)
    {
         char *p = NULL;
         p = strtok((char*)dest,splittag);
         while(p)
         {
             out.push_back(p);
             p=strtok(NULL,splittag);
         } 
        return true;
    }
};


class WhenNode:public CodeNode{
public:
    CodeNode  *condition;
    int state; // YES or NO
    virtual int exec(Args *stub)
    {
        if (!condition) return NO;
        if (condition->exec(stub) == state)
        {
            // 顺序执行child
            for (CHILDS_ITER iter = childs.begin(); iter != childs.end();++iter){
                CodeNode  * node = *iter;
                if (node)
                    node->exec(stub);
            }
            return YES;
        }
       return NO;
    }
    WhenNode(){state =YES;condition =NULL;}
    ~WhenNode(){
        //if (condition) delete condition; condition = NULL;
    }
};

class CaseNode:public WhenNode{
public:
};

class SwitchNode:public CodeNode{
public:
    virtual int exec(Args *stub)
    {
        // 遍历子节点
        for (CHILDS_ITER iter = childs.begin(); iter != childs.end();++iter){
            CodeNode * node = *iter;
            if (node && node->exec(stub) == YES)
            {
                break;
            }
        }
        return YES;
    }   
};
template<typename TYPE>
class TypeHandler:public LogicFieldHandler 
{
public:
    TypeHandler(){}
    int callback(Args *stub,Properties &propies)
    {
        return CodeState::YES;
    }

    virtual std::string get(Args *stub){
        std::stringstream ss;
        ss << value;
        #if _DEBUG
        printf("get xml var %s\n",ss.str().c_str());
        #endif
        return ss.str(); 
    } 
    virtual void set(Args *stub,const std::string& value){
        std::stringstream ss;
        ss << value;
        ss >> this->value;
        #if _DEBUG
        printf("set xml var %s\n",ss.str().c_str());
        #endif
    }   
    virtual void * getPointer(Args *stub)
    {
        return &this->value;
    }
    virtual void setPointer(Args *stub,void *pointer)
    {
        value = *(TYPE*)pointer;
    }
    TYPE value;
};
template<typename STUB,typename TYPE>
class HandlerField:public LogicFieldHandler
{
public:
    typedef TYPE STUB::*HANDLE;
    HandlerField(HANDLE handle):handle(handle){}
    int callback(Args *stub,Properties &propies)
    {
        return CodeState::YES;
    }
    
    virtual std::string get(Args *stub){
        STUB * args = ((Args*)stub)->getObject<STUB>();
		if (!args) return "";
        TYPE& src = args->*handle;
        std::stringstream ss;
        ss << src;
        return ss.str(); 
    } 
    virtual void set(Args *stub,const std::string& value){
        std::stringstream ss;
        STUB * args = ((Args*)stub)->getObject<STUB>();
		if (!args) return;
        TYPE& src = args->*handle;
        ss << value;
        ss >> src;
    } 
    virtual void * getPointer(Args *stub)
    {
        STUB * args = ((Args*)stub)->getObject<STUB>();
		if (!args) return NULL;
        return & (args->*handle);
    }
    virtual void setPointer(Args *stub,void *pointer)
    {
        STUB * args = ((Args*)stub)->getObject<STUB>();
		if (!args) return;
        args->*handle = *(TYPE*)pointer;
    }  
    HANDLE handle;
};

#define BIND_FUNCTION(CLASS,FUNCTION)\
    addHandler(#FUNCTION,new HandlerFunction<CLASS>(&CLASS::FUNCTION))

#define BIND_FIELD(TYPE,CLASS,FIELD)\
     addHandler(#FIELD,new HandlerField<CLASS,TYPE>(&CLASS::FIELD));

#define BIND_FUNCTION_NAME(CLASS,FUNCTION,NAME)\
    addHandler(NAME,new HandlerFunction<CLASS>(&CLASS::FUNCTION))

template<typename CLASS>
class HandlerFunction:public LogicFunctionHandler
{
public:
    typedef int (CLASS::*HANDLE)(Args *stub,Properties &propies);
    HANDLE handle;
    HandlerFunction(HANDLE handle):handle(handle){}
    int callback(Args *stub,Properties &propies)
    {
        CLASS * args = stub->getObject<CLASS>();
		if (!args) return CodeState::NO; 
        return (args->*handle)(stub,propies);
    }
};




#define DEC_AI_FUNCTION(func) int func(Args * stub,Properties & propies)




#define DEC_LANG_FUNCTION(func) int func(Args * stub,Properties & propies)

