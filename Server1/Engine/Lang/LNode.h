/**********************************************************************************
 * Author jijinlong Date 2014/3/30
 * email:jjl_2009_hi@163.com
 * 我的脚本语言 扩展C++系统中的逻辑
 * code(name="1235",lib="skill") //关于技能1235 的定制
 *{
 *	var(a="100"); // 定义一个变量
 *	var(a="a+100");
 *	or(){
 *		cartoon.atFrame(2)
 *		{
 *			action(); // 在第二帧的时候 处理相关逻辑
 * 		}
 *		cartoon.atFrame(3) 
 *		{
 *			gotoFrame(6); //在第三帧的时候 跳到第6帧 
 *		}
 *		cartoon.atFrame(10)
 *		{
 *			cartoon.create("other.ani",1000,arg="other");
 *			other.setPosition(100,100);
 *			inner("inner") // 在codeNode 中存在该代码且不执行
 *			{
 *				other.destory();// 10 分钟删除自身
 *			}
 *			other.atFrame(10,"inner"); // inner是一个回调模块 需要定制回调模块的环境
 *		}
 *	}
 *}
 *code()
 *{}
 *************************************************************************************
 * 这里的语法采用树的结构 使用C++的函数作为基本的逻辑单元
 *************************************************************************************
 * 极其轻量
 **/

#pragma once
#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
namespace lang{
	struct Cell{
		int type;
		enum{
			NULL_ = 0,
			NAME_ = 1, // accii
			LEFT_ = '(',  
			EQUAL_ = '=',  
			SPLIT_ = ',',  
			RIGHT_ = ')', 
			OPEN_ = '{',
			CLOSE_ = '}', 
			LINE_ = 254,
			LINE_END_ = ';',
		};
		std::string name;
		Cell()
		{
			type = NULL_;
		}
		static unsigned int getIndex(const int &type )
		{
			switch(type)
			{
			case NAME_:return 0;
			case LEFT_:return 1;
			case EQUAL_:return 2;
			case SPLIT_:return 3;
			case RIGHT_:return 4;
			case OPEN_:return 5;
			case CLOSE_:return 6;
			case LINE_:return 7;
			case LINE_END_:return 8;
			}
			return -1;
		}
	};
	class Cells{ // 词法单元
	public:
		std::vector<Cell> cells;
	};
	class LValue{ // 属性列表
	public:
		std::string name;
		std::string value;
	};
	/**
	 * 执行节点
	 */
	class LNode{
	public:
		std::vector<LValue> lVales;
		std::vector<LNode*> childs;
		std::string name;
		unsigned long line;
		LNode *next;
		LNode()
		{
			line = 0;
			next = NULL;
		}
		std::string get(const std::string &name)
		{
			for (std::vector<LValue>::iterator iter = lVales.begin(); iter != lVales.end();++iter)
			{
				if (iter->name == name) return iter->value;
			}
			return "";
		}
		LNode *FirstChild()
		{
			if (childs.size())
			{
				for (unsigned int index = 0; index < childs.size();index++)
				{
					if (index +1 < childs.size())
					{
						childs[index]->next = childs[index+1];
					}
				}
				return childs[0];
			}
			return NULL;
		}
		LNode * Next()
		{
			return next;
		}
		LNode *Index(unsigned int index)
		{
			if (index < childs.size()) return childs[index];
			return NULL;
		}
		virtual ~LNode()
		{
			for (unsigned int index = 0; index < childs.size();index++)
			{
				if (childs[index]) delete childs[index];
			}
			childs.clear();
		}
	};
	/**
	 * Document
	 */
	class NodeFile{
	public:
		Cells cells;
		Cell cell;
		enum STATE{
			WAIT_END,
			WAIT_NULL,
		}state;
		NodeFile()
		{
			state = WAIT_NULL;
		}
		LNode * read(const char *name)
		{
			FILE *hF = fopen(name,"r");
			char buf[1024]={'\0'};
			if (hF)
			{
				while(fgets(buf,1023,hF))
				{
					parseString(buf);
					memset(buf,0,1024);
				}
				fclose(hF);
			}
			else
			{
				return NULL;
			}
			LNode * root = parseCell();
			//printf("%x",(int)root);
			return root;
		}
		bool checkSplit(const char &pointer)
		{
			switch(pointer)
			{
				case ' ':
				case '\t':
				case '(':
				case ')':
				case '{':
				case '}':
				case '=':
				case '\n':
				case '"':
				case ',':
				case ';':
				case '/':
					return true;
			}
			return false;
		}
		bool checkSpace(const char &pointer)
		{
			switch(pointer)
			{
				case ' ':
				case '\t':
				case '\r':
					return true;
			}
			return false;
		}
		bool checkLineEnd(const char &pointer)
		{
			switch(pointer)
			{
				case '\n':
					return true;
			}
			return false;
		}
		bool checkBreak(const char &pointer)
		{
			if (state == WAIT_END && pointer =='"') return true;
			return false;
		}
		/**
		 * 词法分析器 简单的硬编码
		 **/
		void parseString(const char *pointer)
		{
			while (*pointer !='\0')
			{
				while (!checkSplit(*pointer) || state == WAIT_END) // 不是分隔符号的话
				{
					if (*pointer == '\0') return;
					if (*pointer != '"')
						cell.name.push_back(*pointer);
					if (*pointer =='"' && state == WAIT_END)
					{
						if (cell.name == "") cell.name = ("null");
						state = WAIT_NULL;
					}
					pointer++;
				}
				if (*pointer == '/')
				{
					pointer++;
					if (*pointer=='/')
					{
						while (!checkLineEnd(*pointer)) *pointer++;
					}
					else
					{
						// todo error
						break;
					}
				}
				
				if (*pointer =='"')
				{
					if (state == WAIT_NULL)
						state = WAIT_END; 
					if (cell.name !="")
					{
						cell.type = Cell::NAME_;
						cells.cells.push_back(cell);
					}
					cell.name.clear();
				}
				else
				{
					if (cell.name != "")
					{
						cell.type = Cell::NAME_;
						cells.cells.push_back(cell);
					}
					cell.name.clear();
					if ( checkSplit(*pointer))
					{
						if ( !checkSpace(*pointer))
						{
							cell.type = *pointer;
							if (checkLineEnd(*pointer))
							{
								cell.type = Cell::LINE_;
							}
							cells.cells.push_back(cell);
						}
						cell.name.clear();
					}
					else 
					{
						cell.name.push_back(*pointer);
					}
				}
				++pointer;
			}
			if (cell.name != "")
			{
				cell.type = Cell::NAME_;
				cells.cells.push_back(cell);
			}
		}
		std::vector<LNode*> parents;
		LNode * nowNode;
		unsigned long line;
		/**
		 * 状态机 处理语法单元 
		 * 还是很难懂的 不过代码简单明了 效率高
		 */
		LNode* parseCell()
		{
			static int States[7][9]={
				{1 ,-1,-1,-1,-1, 0, 0,-2,-1}, 
				{-1, 2,-1,-1,-1,-1,-1,-2,-1}, // 1,处理节点创建 名字 加入父亲节点
				{ 3,-1,-1,-1, 4,-1,-1,-2,-1}, // 3,开始确定属性名字
				{-1,-1, 5, 2, 4,-1,-1,-2,-1}, // 4,结束节点
				{-1,-1,-1,-1,-1, 0,-1,-2, 0}, // 6,确定属性值
				{ 6,-1,-1,-1,-1,-1,-1,-2,-1}, // 0,确定输入{ 当前节点 为根节点 } pop当前根节点 
				{-1,-1,-1, 2, 4,-1,-1,-2,-1},
			};	
			int state = 0;
			line = 1;
			nowNode = NULL;
			LNode *root = new LNode;
			parents.push_back(root);
			for(std::vector<Cell>::iterator iter = cells.cells.begin(); iter != cells.cells.end();++iter)
			{
				int index = iter->getIndex(iter->type);
				if (index == -1) 
				{
					delete root;
					return NULL;
				}
				int oldstate =  States[state][index];
				if (oldstate == -1)
				{
					delete root;
					return NULL;
				}
				if (oldstate == -2)
				{
					line++;
				}
				else state = oldstate;
				parseState(state,*iter);
			}
			return root;
		}
		LNode * getParent()
		{
			if (parents.empty()) return NULL;
			return parents.back();
		}
		void parseState(int state,const Cell &cell)
		{
			switch(state)
			{
				case 0:
				{
					if (cell.type == Cell::OPEN_)
					{
						if (nowNode) parents.push_back(nowNode);
					}
					if (cell.type == Cell::CLOSE_)
					{
						if (parents.size())
						{
							parents.pop_back();
						}
						else
						{
							// error
						}
					}
				}break;
				case 1:
				{
					nowNode = new LNode;
					nowNode->line = line;
					nowNode->name = cell.name;
					if (getParent()) getParent()->childs.push_back(nowNode);
				}break;
				case 3:
				{
					if (nowNode && cell.name !="")
					{
						LValue lValue;
						lValue.name = cell.name;
						nowNode->lVales.push_back(lValue);
					}
				}break;
				case 6:
				{
					if (nowNode && nowNode->lVales.size() && cell.name !="")
					{
						nowNode->lVales.back().value = cell.name;
					}
				}break;
				case 4:
				{
				
				}break;
			}
		}
	};
}

