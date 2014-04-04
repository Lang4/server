/**
 * \file
 * \version  $Id: LCantCopy.h  $
 * \author  
 * \date 
 * \brief 定义zNoncopyable类，使所有的子类禁用拷贝构造函数和赋值符号
 *
 * 
 */


#ifndef _zNoncopyable_h_
#define _zNoncopyable_h_

/**
 * \brief 使所有的子类禁用拷贝构造函数和赋值符号
 *
 */
class LCantCopy
{

	protected:

		/**
		 * \brief 缺省构造函数
		 *
		 */
		LCantCopy() {};

		/**
		 * \brief 缺省析构函数
		 *
		 */
		~LCantCopy() {};

	private:

		/**
		 * \brief 拷贝构造函数，没有实现，禁用掉了
		 *
		 */
		LCantCopy(const LCantCopy&);

		/**
		 * \brief 赋值操作符号，没有实现，禁用掉了
		 *
		 */
		const LCantCopy & operator= (const LCantCopy &);

};

#endif

