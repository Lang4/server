/**
 * \file
 * \version  $Id: LRWLock.h  $
 * \author  
 * \date 
 * \brief 定义zRWLock类，简单对系统读写锁操作进行封装
 *
 * 
 */


#ifndef _zRWLock_h_
#define _zRWLock_h_

#include <pthread.h>

#include "LCantCopy.h"

/**
 * \brief 封装了系统读写锁，使用上要简单，省去了手工初始化和销毁系统读写锁的工作，这些工作都可以由构造函数和析构函数来自动完成
 *
 */
class LRWLock : private LCantCopy
{

	public:
		//读写错计数测试
		//unsigned int rd_count;
		//unsigned int wr_count;

		/**
		 * \brief 构造函数，用于创建一个读写锁
		 *
		 */
		LRWLock()//:rd_count(0),wr_count(0)
		{
			//std::cout << __PRETTY_FUNCTION__ << std::endl;
			::pthread_rwlock_init(&rwlock, NULL);
		}

		/**
		 * \brief 析构函数，用于销毁一个读写锁
		 *
		 */
		~LRWLock()
		{
			//std::cout << __PRETTY_FUNCTION__ << std::endl;
			::pthread_rwlock_destroy(&rwlock);
		}

		/**
		 * \brief 对读写锁进行读加锁操作
		 *
		 */
		inline void rdlock()
		{
			//std::cout << __PRETTY_FUNCTION__ << std::endl;
			::pthread_rwlock_rdlock(&rwlock);
			//++rd_count;
		};

		/**
		 * \brief 对读写锁进行写加锁操作
		 *
		 */
		inline void wrlock()
		{
			//std::cout << __PRETTY_FUNCTION__ << std::endl;
			::pthread_rwlock_wrlock(&rwlock);
			//++wr_count;
			//++rd_count;
		}

		/**
		 * \brief 对读写锁进行解锁操作
		 *
		 */
		inline void unlock()
		{
			//std::cout << __PRETTY_FUNCTION__ << std::endl;
			::pthread_rwlock_unlock(&rwlock);
			//--rd_count;
		}

	private:

		pthread_rwlock_t rwlock;		/**< 系统读写锁 */

};

/**
 * \brief rdlock Wrapper
 * 方便在复杂函数中读写锁的使用
 */
class LRWLockScopeRdlock : private LCantCopy
{

	public:

		/**
		 * \brief 构造函数
		 * 对锁进行rdlock操作
		 * \param m 锁的引用
		 */
		LRWLockScopeRdlock(LRWLock &m) : rwlock(m)
		{
			rwlock.rdlock();
		}

		/**
		 * \brief 析购函数
		 * 对锁进行unlock操作
		 */
		~LRWLockScopeRdlock()
		{
			rwlock.unlock();
		}

	private:

		/**
		 * \brief 锁的引用
		 */
		LRWLock &rwlock;

};

/**
 * \brief wrlock Wrapper
 * 方便在复杂函数中读写锁的使用
 */
class LRWLockScopeWrlock : private LCantCopy
{

	public:

		/**
		 * \brief 构造函数
		 * 对锁进行wrlock操作
		 * \param m 锁的引用
		 */
		LRWLockScopeWrlock(LRWLock &m) : rwlock(m)
		{
			rwlock.wrlock();
		}

		/**
		 * \brief 析购函数
		 * 对锁进行unlock操作
		 */
		~LRWLockScopeWrlock()
		{
			rwlock.unlock();
		}

	private:

		/**
		 * \brief 锁的引用
		 */
		LRWLock &rwlock;

};

#endif

