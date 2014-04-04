重构征途服务器
======

 编码规范 试行:
 ------
 
### 命名<br/>
		类名 全大写 可以看出意义 class LSocket(){};
		
		变量名 首个字母小写 public: int mFeild; private int _mField; 
		
		函数名字 首个字母小写其他词连接时首个字母大写 sayHello()
		
		结构体 前面加st struct stData(){};
		
		命名空间 首字母大写
		
		其他见如下
### 注释 + 实例<br/>
		#define MAX_NUM  //字母全都大写 以_分割
		namespace net { // 命名空间名字 全小写
			类注释
			/**
			 * 说明
			 * 对Socket 的封装
			 * 提供基本的接法
			 **/
			class LScoket { // 凡是类型相关的 括号紧跟其后 (class struct enum union) 且名字大写
			public:
				int mFeild; // 变量注释
				
				/**
				 *\berif 构造函数说明
				 */
				LScoket()
				{ // 函数的括号放下
				
				}
				
				函数注释
				/**
				 * \brief 说明
				 * \param in 参数 输入参数
				 * \parma out 参数 输出参数
				 * \return int 返回说明
				 */
				int sayHello(int a,int &b)
				{
					return 1;
				}
				
				enum Count{ //和类一致的命名方式
					MAX_NUM = 1, // 枚举全都大写 和宏一致的命名方式
				};
			private:
				int _mField; // 变量注释
			};  // class LScoket
		}; // namespace net 
### 以上试行