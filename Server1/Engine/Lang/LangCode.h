/**********************************************************************************
 * Author jijinlong Date 2014/3/30
 * email:jjl_2009_hi@163.com
 */
#pragma once
#include "Lang.h"
#include "Code.h"
class LangCode:public Code{
public:
	
    /**
     * 从配置文件中读取node
     */
    CodeNode * readFromFile(const char *fileName)
    {
		innerVars.clear();
		conditions.clear();
  
		lang::NodeFile doc;
		lang::LNode * root = doc.read(fileName);
		if (root)
		{
			lang::LNode * child = root->FirstChild();
			if (child && child->name =="code")
			{
				CodeNode *node = traceNode(child);
				delete root;
				return node;
			}
			delete root;
		}
        return NULL;
    }
    CodeNode * traceNode(lang::LNode *root)
    {
        CodeNode *codeRoot = NULL;
        if (NULL == root) return NULL;
        lang::LNode *now = root;
        std::vector<lang::LNode*> parents;
        std::vector<CodeNode* > rootNodes;
		bool over = false;
        while(now && !over)
        {
            CodeNode* nowCode = parseNode(now,codeRoot);
            if (!nowCode){
#if 1 
                printf("%s\n", "cant find logic node");
#endif
                break;
            } 
            lang::LNode * child = now->FirstChild();
            if (!child)
            {
				if (now == root) {
					over = true;	
					break;
				}
                now = now->Next();
                while (!now)
                {
                    if (parents.empty()) {
						over = true;
                        break;
                    }
                    lang::LNode * parent = parents.back();
                    codeRoot = rootNodes.back();
                    parents.pop_back();
                    rootNodes.pop_back();
                    now = parent->Next();
                }
				if (now == root) {
					over = true;	
					break;
				}
				if (now && now->name == "code")
				{
					over = true;
					break;
				}
            }
            else
            {
                parents.push_back(now);
                rootNodes.push_back(codeRoot);
                now = child;
                codeRoot = nowCode;
            }
        }
        //printf("create root %x\n", codeRoot);
        return codeRoot;
    }
    CodeNode* parseNode(lang::LNode *now,CodeNode * & codeRoot)
    {
        if (NULL == now) return NULL;
        lang::LNode *element = now;
        if (element)
        {
            #if _DEBUG 
            printf("parse element %s\n", element->name.c_str());
            #endif
            if (element->name == "code")
            {
                if (!codeRoot)
                {
                    codeRoot = new CodeNode();
					codeRoot->nodeName = element->name;
                    return codeRoot;
                }
                return NULL;
            }
            else if ((element->name == "and") && codeRoot)
            {
                AndNode *andNode = new AndNode();
				andNode->nodeName = element->name;
                codeRoot->addChild(andNode);
                return andNode;
            }
            else if ((element->name == "block") && codeRoot)
            {
                CodeNode *codeNode = new CodeNode();
				codeNode->nodeName = element->name;
                codeRoot->addChild(codeNode);
                return codeNode;
            }
            else if ((element->name == "or") && codeRoot)
            {
                OrNode *orNode = new OrNode();
				orNode->nodeName = element->name;
                codeRoot->addChild(orNode);
                return orNode;
            }
            else if ((element->name == "when") || (element->name == "case") && codeRoot)
            {
                WhenNode *whenNode = new WhenNode();
				whenNode->nodeName = element->name;
                codeRoot->addChild(whenNode);
                std::string value = element->get("state");
                if (value == "NO") whenNode->state = CodeNode::NO;
                else whenNode->state = CodeNode::YES;

                // 初始化condition 变量
                whenNode->condition = findCondition(element->get("condition"));
                return whenNode;
            }
            else if (element->name == "while" && codeRoot)
            {
                WhileNode *whileNode = new WhileNode();
                codeRoot->addChild(whileNode);
                std::string value = element->get("state");
                if (value == "NO") whileNode->state = CodeNode::NO;
                else whileNode->state = CodeNode::YES;

                // 初始化condition 变量
                whileNode->condition = findCondition(element->get("condition"));
                return whileNode;
            }
			else if (element->name  == "foreach" && codeRoot)
			{
				ForEachNode *forNode = new ForEachNode();
				codeRoot->addChild(forNode);
                std::string value = element->get("state");
                if (value == "NO") forNode->state = CodeNode::NO;
                else forNode->state = CodeNode::YES;

                // 初始化condition 变量
				forNode->condition = findCondition(element->get("condition"));
				
				forNode->targets = element->get("targets") ;
				forNode->objectName = element->get("object");

				return forNode;
			}
            else if (element->name == "for" && codeRoot)
            {
                ForNode *forNode = new ForNode();
                codeRoot->addChild(forNode);
                forNode->start = element->get("start");
                forNode->end = element->get("end");
                forNode->step =element->get("step");
                return forNode;
            }
            else if (element->name == "condition" && codeRoot)
            {
                ConditionNode *cNode = new ConditionNode();
                //codeRoot->addChild(cNode);
                // 存储condition 
                std::string name = element->get("name");
                addCondition(name,cNode);
                return cNode;
            }
            else if ((element->name == "switch") && codeRoot)
            {
                SwitchNode *sNode = new SwitchNode();
                codeRoot->addChild(sNode);
                return sNode;
            }
            else if ((element->name == "not") && codeRoot)
            {
                NotNode *nNode = new NotNode();
                codeRoot->addChild(nNode);
                std::string actionName = element->get("action");
                parseLogic(actionName,nNode,element); 
                return nNode;
            }
            else if ((element->name  == "var") && codeRoot)
            {
                VarNode *varNode = new VarNode();
				varNode->nodeName = element->name;
                if (parseVar(varNode,element))
                {
                    codeRoot->addChild(varNode);
                    return varNode;
                }
                else
                {
                    printf("parse var error!!\n");
                    return varNode;
                }
            }
			else if (element->name == "calc" && codeRoot)
			{
				CalcNode *cNode = new CalcNode();
                codeRoot->addChild(cNode);
				parseAttribute(cNode,element);
                return cNode;
			}
			else if (element->name == "print" && codeRoot)
			{
				PrintNode *pNode = new PrintNode();
				codeRoot->addChild(pNode);
				parseAttribute(pNode,element);
                return pNode;
			}
            else 
            {
                LogicNode *logicNode = new LogicNode();
                std::string actionName = element->name;
                if (parseLogic(actionName,logicNode,element))
                {
                    codeRoot->addChild(logicNode);
                }
                return logicNode;
            }
        }
        return NULL;
    }
    void parseAttribute(LogicNode *logic,lang::LNode *element)
    {
		for (std::vector<lang::LValue>::iterator iter = element->lVales.begin(); iter != element->lVales.end();++iter)
		{
			logic->propies.propies[iter->name] = iter->value;
		}
    }
    bool parseLogic(const std::string &actionName,LogicNode *logic,lang::LNode *element)
    {
        std::vector<std::string> result;
        Util::split(actionName.c_str(),".",result);
        std::string name;
        if (result.size() == 2)
        {
            logic->objectName = result[0];
            name = result[1];
        }
        logic->handler = findHanderByName(name);
        if (!logic->handler) return false;
        #if _DEBUG
            printf("get handler name %s address:%u\n", name.c_str(),logic->handler);
        #endif
        parseAttribute(logic,element);
        return true;
    }
    bool parseVar(VarNode *logic,lang::LNode *element)
    {
        std::string varName = element->get("name");
        std::vector<std::string> result;
        Util::split(varName.c_str(),".",result);
        if (result.size() == 2)
        {
            logic->objectName = result[0];
            varName = result[1];
            logic->handler = static_cast<LogicFieldHandler*>(findHanderByName(varName.c_str()));
        }
        if (!logic->handler)
        {
            parseUserVar(logic,element,varName);
        }
        parseAttribute(logic,element);
        return true;
    }
    bool parseUserVar(LogicNode *logic,lang::LNode *element,const std::string& varName)
    {
        logic->handler = findInnerVarHanderByName(varName);
        if (!logic->handler) {
			const char* type = element->get("type").c_str();

            if (type && !strcmp(type,"number"))
            {
                logic->handler = new TypeHandler<double>();
            }
            else if (type && !strcmp(type,"int"))
            {
                logic->handler = new TypeHandler<int>();
            }
            else
            {
                logic->handler = new TypeHandler<std::string>();
            }
            addInnerVarHandler(varName,logic->handler);

            printf("new var %s\n",varName.c_str());
        }
        return true;
    }
};



