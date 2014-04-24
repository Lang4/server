/**********************************************************************************
 * Author jijinlong Date 2014/3/30
 * email:jjl_2009_hi@163.com
 */
#pragma once
class LogicFunctionHandler;
class ConditionNode;
class CodeNode;
class Args;
#include <string>
#include <map>
class Code{
public:
	CodeNode *code;
	Code()
	{
		code = NULL;
	}
	virtual ~Code(){}
    std::map<std::string,LogicFunctionHandler* > actions;
    typedef std::map<std::string,LogicFunctionHandler* >::iterator ACTIONS_ITER;
    bool addHandler(const std::string &name,LogicFunctionHandler* stub)
    {
        if (findHanderByName(name)) return false;
        actions[name] = stub;
        return true;
    }
    LogicFunctionHandler* findHanderByName(const std::string &name)
    {
        ACTIONS_ITER iter = actions.find(name);
        if (iter != actions.end()) return iter->second;
        return NULL;
    }
    std::map<std::string,LogicFunctionHandler* > innerVars;
    bool addInnerVarHandler(const std::string &name,LogicFunctionHandler* stub)
    {
        if (findInnerVarHanderByName(name)) return false;
        innerVars[name] = stub;
        return true;
    }
    LogicFunctionHandler* findInnerVarHanderByName(const std::string &name)
    {
        ACTIONS_ITER iter = innerVars.find(name);
        if (iter != innerVars.end()) return iter->second;
        return NULL;
    }

    std::map<std::string,ConditionNode* > conditions;
    typedef std::map<std::string,ConditionNode* >::iterator CONDITIONS_ITER;
    bool addCondition(const std::string &name,ConditionNode* stub)
    {
        if (findCondition(name)) return false;
        conditions[name] = stub;
		return true;
    }
    ConditionNode* findCondition(const std::string &name)
    {
        CONDITIONS_ITER iter = conditions.find(name);
        if (iter != conditions.end()) return iter->second;
        return NULL;
    }

    void destroy();
	Code &operator=(const Code &node)
	{
		this->conditions = node.conditions;
		this->actions = node.actions;
		this->innerVars = node.innerVars;
		return *this;
	}
	virtual Code * clone();
	virtual CodeNode * readFromFile(const char *fileName){return NULL;}
	bool execFile(const char *fileName,Args *args);
	
	int exec(Args *args);
};

