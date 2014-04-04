server
======

 重构征途服务器

 编码规范 试行:
 ---------
 
 ### 命名<br/>
		类名 全大写 可以看出意义 class LSocket(){};
		变量名 首个字母小写 public: int mFeild; private int _mField; 
		函数名字 首个字母小写其他词连接时首个字母大写 sayHello()
		结构体 前面加st struct stData(){};
		命名空间 首字母大写
### 注释<br/>
		函数注释
		/**
		 * \brief 说明
		 * \param in 参数 输入参数
		 * \parma out 参数 输出参数
		 * \return int 返回说明
		 */
		类注释
		/**
		 * 说明
		 **/
### 以上试行