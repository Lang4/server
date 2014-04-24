#pragma once
#include "LType.h"
#include "LCantCopy.h"
#include "LSocket.h"
#include "LThread.h"
#include "LTime.h"
class LCheckwaitThread;
class LCheckconnectThread;
class LTCPClientTaskThread;
class LTCPClientTask;
/**
* \brief 连接线程池类，封装了一个线程处理多个连接的线程池框架
*
*/
class LTCPClientTaskPool : private LCantCopy
{

public:

	explicit LTCPClientTaskPool(const DWORD connPerThread,const int us=50000) : connPerThread(connPerThread)
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

	const DWORD connPerThread;
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
	//typedef std::list<LTCPClientTask *,__pool_alloc<LTCPClientTask *> > LTCPClientTaskContainer;
	typedef std::list<LTCPClientTask *> LTCPClientTaskContainer;


	/**
	* \brief 连接任务链表叠代器
	*
	*/
	typedef LTCPClientTaskContainer::iterator zTCPClientTask_IT;

	LMutex mlock;          /**< 互斥变量 */
	LTCPClientTaskContainer tasks;  /**< 任务列表 */

public:
	int usleep_time;                                        /**< 循环等待时间 */
};
