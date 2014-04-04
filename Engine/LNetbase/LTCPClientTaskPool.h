/**
 * \file
 * \version  $Id: LTCPClientTaskPool.h  $
 * \author  
 * \date 
 * \brief 封装实现线程池，用于处理多连接服务器
 *
 * 
 */


#ifndef _zTCPClientTaskPool_h_
#define _zTCPClientTaskPool_h_

#include <string>
#include <vector>
#include <queue>
#include <list>
#include <unistd.h>
#include <sys/timeb.h>

#include "LSocket.h"
#include "LThread.h"
#include "LTCPClientTask.h"

class LCheckconnectThread;
class LCheckwaitThread;
class LTCPClientTaskThread;

/**
 * \brief 连接线程池类，封装了一个线程处理多个连接的线程池框架
 *
 */
class LTCPClientTaskPool : private LCantCopy
{

	public:

		explicit LTCPClientTaskPool(const unsigned int connPerThread=512, const int us=50000) : connPerThread(connPerThread)
		{       
			usleep_time=us;
			checkwaitThread = NULL; 
		} 
		~LTCPClientTaskPool();

		bool init();
		bool put(LTCPClientTask *task);
		void timeAction(const LTime &ct);

		void addCheckwait(LTCPClientTask *task);
		bool addMain(LTCPClientTask *task);
		void setUsleepTime(int time)
		{
			usleep_time = time;
		}

	private:

		const unsigned int connPerThread;
		LTCPClientTaskThread *newThread();

		/**
		 * \brief 连接检测线程
		 *
		 */
		LCheckconnectThread *checkconnectThread;;
		/**
		 * \brief 连接等待返回信息的线程
		 *
		 */
		LCheckwaitThread *checkwaitThread;;
		/**
		 * \brief 所有成功连接处理的主线程
		 *
		 */
		LThreadGroup taskThreads;

		/**
		 * \brief 连接任务链表
		 *
		 */
#ifdef _POOL_ALLOC_
		typedef std::list<LTCPClientTask *, __gnu_cxx::__pool_alloc<LTCPClientTask *> > LTCPClientTaskContainer;
#else
		typedef std::list<LTCPClientTask * > LTCPClientTaskContainer;
#endif

		/**
		 * \brief 连接任务链表叠代器
		 *
		 */
		typedef LTCPClientTaskContainer::iterator LTCPClientTask_IT;

		LMutex mlock;					/**< 互斥变量 */
		LTCPClientTaskContainer tasks;	/**< 任务列表 */

	public:
		int usleep_time;                                        /**< 循环等待时间 */
};

#endif

