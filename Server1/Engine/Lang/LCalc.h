/**********************************************************************************
 * Author jijinlong Date 2014/3/21
 * email:jjl_2009_hi@163.com
 * 语句计算模块 设定数值类变量的运算 左值计算类型
 */

#pragma once
class LCalc{
public:
	enum{
		NULL_ = -1,
		OP_ = 0,
		EQUAL_ = 1,
		ASCII_ = 2,
		SPACE_ = 3,
	};
	LCalc(){
		
	};
	static int getType(const char&point)
	{
		switch(point)
		{
			case '>':case '<':case '!':case '+':case '-':
			case '/':case '*':case '%':return OP_;
			case '=':return EQUAL_;
			case '0':case '1':case '2':case '3':
			case '4':case '5':case '6':case '7':
			case '8':case '9':case '#':case '$':
			case '@':return ASCII_;
			case '\n':case '\t':case '\r':
			case ' ':return SPACE_;
			default:
				{
					if (point >= 'a' && point <= 'z') return ASCII_;
					if (point >= 'A'&& point <= 'Z') return ASCII_;
				}
				break;
		}
		return -1;
	}

	double parseString(const char *pointer)
	{
		std::string temp;
		std::vector<std::string> strings;
		static int States[7][4]=
			{
				{-1,-1, 1, 0},
				{ 2, 3, 1, 4},
				{-1, 3, 6, 5},
				{-1,-1, 1, 3},
				{ 2, 3,-1, 4},
				{-1,-1, 1, 5},
				{ 2, 3, 1, 4},
			};
		int state = 0;
		while (*pointer != '\0')
		{
			int index = getType(*pointer);
			if (index == -1 || index < 0 || index > 3)
			{
				printf("不可识别的字符串");
				return 0;
			}
			state = States[state][index];
			if (state < 0 || state > 6)
			{
				printf("语法有误");
				return 0;
			}
			parseState(state,*pointer,temp,strings);
			pointer++;
		}
		if (temp!="")
		{
			strings.push_back(temp);
			temp.clear();
		}
		return calc(strings);
	}
	static void parseState(int state,const char &pointer,std::string &temp,std::vector<std::string> &strings)
	{
		switch(state)
		{
		case 0: 
		case 4:
		case 5:
			{
				if (temp!="")
				{
					strings.push_back(temp);
					temp.clear();
				}
			}break;
		case 6:
			{
				if (temp!="")
				{
					strings.push_back(temp);
					temp.clear();
				}
				temp.push_back(pointer);
			}break;
		case 1: 
			{
				temp.push_back(pointer);
			}break;
		case 2:
			{
				if (temp!="")
				{
					strings.push_back(temp);
					temp.clear();
				}
				temp.push_back(pointer);
			}break;
		case 3:
			{
				if (temp!="")
				{
					temp.push_back(pointer);
					strings.push_back(temp);	
				}
				else if (pointer == '=')
				{
					temp.push_back(pointer);
					strings.push_back(temp);	
				}
				temp.clear();
			}break;
		}
		return;
	}
	static int checkOperator(const std::string &op)
	{
		static char * ops[]={
			">", // 0
			"<", // 1
			">=", // 2
			"<=", // 3
			"==", // 4
			"!=", // 5
			"=", // 6
			"+", // 7
			"-", // 8
			"*" //9
			,"/" // 10
			,"%" // 11
			,"!" // 12
		};
		for (unsigned int index = 0; index < 13;++index)
		{
			if (op == ops[index]) return index;
		}
		return -1;
	}
	/**
	 * 计算表达式
	 * 两两计算 
	 */
	double calc(std::vector<std::string> &strings)
	{
		double result = 0;
		int op = -1;
		for (std::vector<std::string>::iterator iter = strings.begin(); iter != strings.end();++iter)
		{
			int o = (checkOperator(*iter));
			if (o != -1)
			{
				op = o;
				continue;
			}
			float now = getValue(*iter);
			switch(op)
			{
				case -1: result = now;break;
				case 0: result = result > now ? 1:0; break;
				case 1: result = result < now ? 1:0; break;
				case 2: result = result >= now ? 1:0; break;
				case 3: result = result <= now ? 1:0; break;
				case 4: result = result == now ? 1:0; break;
				case 5:result = result != now ? 1:0; break;
				case 6: result = result == now ? 1:0; break;
				case 7:result += now;break;
				case 8:result -= now;break;
				case 9: result *= now; break;
				case 10: result /= now;break;
				case 11: result /= now;break;
				case 12: result = now != 0 ? 1:0; break;
			}
			
		}
		return result;
	}
	virtual float getValue(const std::string & name)
	{
		return atof(name.c_str());
	}
	virtual ~LCalc(){}
};

