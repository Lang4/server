/**
 * \file
 * \version  $Id: LThread.h  $
 * \author  
 * \date 
 * \brief 定义类zThread
 *
 * 
 */

#ifndef _zThread_h_
#define _zThread_h_

#include <pthread.h>
#include <unistd.h>

#include "LCantCopy.h"
#include "LMutex.h"
#include "LCond.h"
#include "LRWLock.h"
#include <vector>
/**
 * \brief 封装了线程操作，所有使用线程的基类
 *
 */
class LThread : private LCantCopy
{

	public:

		/**
		 * \brief 构造函数，创建一个对象
		 *
		 * \param name 线程名称
		 * \param joinable 标明这个线程退出的时候是否保存状态，如果为true表示线程退出保存状态，否则将不保存退出状态
		 */
		LThread(const std::string &name = std::string("LThread"), const bool joinable = true) 
			: threadName(name), alive(false), complete(false), thread(0), joinable(joinable) {};

		/**
		 * \brief 析构函数，用于销毁一个对象，回收对象空间
		 *
		 */
		virtual ~LThread() {};

		/**
		 * \brief 获取当前线程编号
		 *
		 *
		 * \return 线程编号
		 */
		static pthread_t getCurrentThreadId()
		{
			return ::pthread_self();
		}

		/**
		 * \brief 使当前线程睡眠指定的时间，秒
		 *
		 *
		 * \param sec 指定的时间，秒
		 */
		static void sleep(const long sec)
		{
			::sleep(sec);
		}

		/**
		 * \brief 使当前线程睡眠指定的时间，毫秒
		 *
		 *
		 * \param msec 指定的时间，毫秒
		 */
		static void msleep(const long msec)
		{
			::usleep(1000 * msec);
		}

		/**
		 * \brief 使当前线程睡眠指定的时间，微秒
		 *
		 *
		 * \param usec 指定的时间，微秒
		 */
		static void usleep(const long usec)
		{
			::usleep(usec);
		}

		/**
		 * \brief 线程是否是joinable的
		 *
		 *
		 * \return joinable
		 */
		const bool isJoinable() const
		{
			return joinable;
		}

		/**
		 * \brief 检查线程是否在运行状态
		 *
		 * \return 线程是否在运行状态
		 */
		const bool isAlive() const
		{
			return alive;
		}

		static void *threadFunc(void *arg);
		bool start();
		void join();

		/**
		 * \brief 主动结束线程
		 *
		 * 其实只是设置标记，那么线程的run主回调循环回检查这个标记，如果这个标记已经设置，就退出循环
		 *
		 */
		void final()
		{
			//Zebra::logger->debug("%s", __PRETTY_FUNCTION__);
			complete = true;
		}

		/**
		 * \brief 判断线程是否继续运行下去
		 *
		 * 主要用在run()函数循环中，判断循环是否继续执行下去
		 *
		 * \return 线程主回调是否继续执行
		 */
		const bool isFinal() const 
		{
			//if (complete)
			//	Zebra::logger->debug("%s", __PRETTY_FUNCTION__);
			return complete;
		}

		/**
		 * \brief 纯虚构函数，线程主回调函数，每个需要实例华的派生类需要重载这个函数
		 *
		 * 如果是无限循环需要在每个循环检查线程退出标记isFinal()，这样能够保证线程安全退出
		 * <pre>
		 * 	while(!isFinal())
		 * 	{
		 * 		...
		 * 	}
		 * 	</pre>
		 *
		 */
		virtual void run() = 0;

		/**
		 * \brief 判断两个线程是否是同一个线程
		 * \param other 待比较的线程
		 * \return 是否是同一个线程
		 */
		bool operator==(const LThread& other) const
		{
			return pthread_equal(thread, other.thread) != 0;
		}

		/**
		 * \brief 判断两个线程是否不是同一个线程
		 * \param other 待比较的线程
		 * \return 是否不是同一个线程
		 */
		bool operator!=(const LThread& other) const
		{
			return !operator==(other);
		}

		/**
		 * \brief 返回线程名称
		 *
		 * \return 线程名称
		 */
		const std::string &getThreadName() const
		{
			return threadName;
		}

	private:

		std::string threadName;			/**< 线程名称 */
		LMutex mlock;					/**< 互斥锁 */
		LCond cond;						/**< 条件变量 */
		volatile bool alive;			/**< 线程是否在运行 */
		volatile bool complete;			/**< 线程是否将结束 */
		pthread_t thread;				/**< 线程编号 */
		bool joinable;					/**< 线程属性，是否设置joinable标记 */

}; 

/**
 * \brief 对线程进行分组管理的类
 *
 */
class zThreadGroup : private LCantCopy
{

	public:

		struct Callback
		{
			virtual void exec(LThread *e)=0;
			virtual ~Callback(){};
		};

		typedef std::vector<LThread *> Container;	/**< 容器类型 */

		zThreadGroup();
		~zThreadGroup();
		void add(LThread *thread);
		LThread *getByIndex(const Container::size_type index);
		LThread *operator[] (const Container::size_type index);
		void joinAll();
		void execAll(Callback &cb);
		
		const Container::size_type size()
		{
			zRWLock_scope_rdlock scope_rdlock(rwlock);
			return vts.size();
		}

	private:

		Container vts;								/**< 线程向量 */
		LRWLock rwlock;								/**< 读写锁 */

};

#endif

