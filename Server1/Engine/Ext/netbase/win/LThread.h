#pragma once
#include "LCantCopy.h"
#include "LType.h"
#include <string>
#include <list>
#include <map>
#include <vector>
#include <time.h>
#include <sstream>
#include <errno.h>
#include "windows.h"
/**
* \brief 定义类zThread
*
* 
*/
/**
* \brief 临界区，封装了系统临界区，避免了使用系统临界区时候需要手工初始化和销毁临界区对象的操作
*
*/
class LMutex : private LCantCopy
{

	friend class LCond;

public:
	/**
	* \brief 构造函数，构造一个互斥体对象
	*
	*/
	LMutex() 
	{
		m_hMutex = CreateMutex(NULL,FALSE,NULL);
	}

	/**
	* \brief 析构函数，销毁一个互斥体对象
	*
	*/
	~LMutex()
	{
		CloseHandle(m_hMutex);
	}

	/**
	* \brief 加锁一个互斥体
	*
	*/
	inline void lock()
	{
		if( WaitForSingleObject(m_hMutex,10000) == WAIT_TIMEOUT )
		{
			char szName[MAX_PATH];
			GetModuleFileName(NULL,szName,sizeof(szName));
			::MessageBox(NULL,"发生死锁！", szName, MB_ICONERROR);
		}
	}

	/**
	* \brief 解锁一个互斥体
	*
	*/
	inline void unlock()
	{
		ReleaseMutex(m_hMutex);
	}

private:

	HANDLE m_hMutex;    /**< 系统互斥体 */

};

/**
* \brief Wrapper
* 方便在复杂函数中锁的使用
*/
class LMutexScopeLock : private LCantCopy
{

public:

	/**
	* \brief 构造函数
	* 对锁进行lock操作
	* \param m 锁的引用
	*/
	LMutexScopeLock(LMutex &m) : mlock(m)
	{
		mlock.lock();
	}

	/**
	* \brief 析购函数
	* 对锁进行unlock操作
	*/
	~LMutexScopeLock()
	{
		mlock.unlock();
	}

private:

	/**
	* \brief 锁的引用
	*/
	LMutex &mlock;

};

/**
* \brief 封装了系统条件变量，使用上要简单，省去了手工初始化和销毁系统条件变量的工作，这些工作都可以由构造函数和析构函数来自动完成
*
*/
class LCond : private LCantCopy
{

public:

	/**
	* \brief 构造函数，用于创建一个条件变量
	*
	*/
	LCond()
	{
		m_hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	}

	/**
	* \brief 析构函数，用于销毁一个条件变量
	*
	*/
	~LCond()
	{
		CloseHandle(m_hEvent);
	}

	/**
	* \brief 对所有等待这个条件变量的线程广播发送信号，使这些线程能够继续往下执行
	*
	*/
	void broadcast()
	{
		SetEvent(m_hEvent);
	}

	/**
	* \brief 对所有等待这个条件变量的线程发送信号，使这些线程能够继续往下执行
	*
	*/
	void signal()
	{
		SetEvent(m_hEvent);
	}

	/**
	* \brief 等待特定的条件变量满足
	*
	*
	* \param m_hMutex 需要等待的互斥体
	*/
	void wait(LMutex &mutex)
	{
		WaitForSingleObject(m_hEvent,INFINITE);
	}

private:

	HANDLE m_hEvent;    /**< 系统条件变量 */

};

/**
* \brief 封装了系统读写锁，使用上要简单，省去了手工初始化和销毁系统读写锁的工作，这些工作都可以由构造函数和析构函数来自动完成
*
*/
class LRWLock : private LCantCopy
{

public:
	/**
	* \brief 构造函数，用于创建一个读写锁
	*
	*/
	LRWLock()
	{
		m_hMutex = CreateMutex(NULL,FALSE,NULL);
	}

	/**
	* \brief 析构函数，用于销毁一个读写锁
	*
	*/
	~LRWLock()
	{
		CloseHandle(m_hMutex);
	}

	/**
	* \brief 对读写锁进行读加锁操作
	*
	*/
	inline void rdlock()
	{
		WaitForSingleObject(m_hMutex,INFINITE);
	};

	/**
	* \brief 对读写锁进行写加锁操作
	*
	*/
	inline void wrlock()
	{
		WaitForSingleObject(m_hMutex,INFINITE);
	}

	/**
	* \brief 对读写锁进行解锁操作
	*
	*/
	inline void unlock()
	{
		ReleaseMutex(m_hMutex);
	}

private:

	HANDLE m_hMutex;    /**< 系统读写锁 */

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
	LThread(const std::string &name = std::string("LThread"),const bool joinable = true) 
		: threadName(name),alive(false),complete(false),joinable(joinable) { m_hThread = NULL; };

	/**
	* \brief 析构函数，用于销毁一个对象，回收对象空间
	*
	*/
	virtual ~LThread()
	{
		if (NULL != m_hThread)
		{
			CloseHandle(m_hThread);
		}
	};

	/**
	* \brief 使当前线程睡眠指定的时间，秒
	*
	*
	* \param sec 指定的时间，秒
	*/
	static void sleep(const long sec)
	{
		::Sleep(1000 * sec);
	}

	/**
	* \brief 使当前线程睡眠指定的时间，毫秒
	*
	*
	* \param msec 指定的时间，毫秒
	*/
	static void msleep(const long msec)
	{
		::Sleep(msec);
	}

	/**
	* \brief 使当前线程睡眠指定的时间，微秒
	*
	*
	* \param usec 指定的时间，微秒
	*/
	static void usleep(const long usec)
	{
		::Sleep(usec / 1000);
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

	static DWORD WINAPI threadFunc(void *arg);
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
		return complete;
	}

	/**
	* \brief 纯虚构函数，线程主回调函数，每个需要实例华的派生类需要重载这个函数
	*
	* 如果是无限循环需要在每个循环检查线程退出标记isFinal()，这样能够保证线程安全退出
	* <pre>
	*   while(!isFinal())
	*   {
	*     ...
	*   }
	*   </pre>
	*
	*/
	virtual void run() = 0;


	/**
	* \brief 返回线程名称
	*
	* \return 线程名称
	*/
	const std::string &getThreadName() const
	{
		return threadName;
	}

public:

	std::string threadName;      /**< 线程名称 */
	LMutex mlock;          /**< 互斥锁 */
	volatile bool alive;      /**< 线程是否在运行 */
	volatile bool complete;
	HANDLE m_hThread;        /**< 线程编号 */
	bool joinable;          /**< 线程属性，是否设置joinable标记 */

}; 

/**
* \brief 对线程进行分组管理的类
*
*/
class LThreadGroup : private LCantCopy
{

public:

	struct Callback
	{
		virtual void exec(LThread *e)=0;
		virtual ~Callback(){};
	};

	typedef std::vector<LThread *> Container;  /**< 容器类型 */

	LThreadGroup();
	~LThreadGroup();
	void add(LThread *thread);
	LThread *getByIndex(const Container::size_type index);
	LThread *operator[] (const Container::size_type index);
	void joinAll();
	void execAll(Callback &cb);

	const Container::size_type size()
	{
		LRWLockScopeRdlock scope_rdlock(rwlock);
		return vts.size();
	}

private:

	Container vts;                /**< 线程向量 */
	LRWLock rwlock;                /**< 读写锁 */

};
