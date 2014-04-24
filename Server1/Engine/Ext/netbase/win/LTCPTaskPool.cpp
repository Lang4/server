/**
* \brief 实现线程池类,用于处理多连接服务器
*/
#include "LTCPTaskPool.h"

#include <assert.h>
//#include <ext/pool_allocator.h>
#include "LTCPTask.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

LTCPTask* g_DeleteLog = NULL;

LMutex g_DeleteLock;

int LTCPTaskPool::usleep_time=50000;                    /**< 循环等待时间 */
/**
* \brief 连接任务链表
*
*/
//typedef std::list<LTCPTask *,__gnu_cxx::__pool_alloc<LTCPTask *> > LTCPTaskContainer;
typedef std::vector<LTCPTask *> LTCPTaskContainer;

/**
* \brief 连接任务链表叠代器
*
*/
typedef LTCPTaskContainer::iterator LTCPTask_IT;

typedef std::vector<struct pollfd> pollfdContainer;

class LTCPTaskQueue
{
public:
	LTCPTaskQueue() :_size(0) {}
	virtual ~LTCPTaskQueue() {}
	inline void add(LTCPTask *task)
	{
		mlock.lock();
		_queue.push_back(task);
		_size++;
		mlock.unlock();
	}
	inline void check_queue()
	{
		mlock.lock();
		while(!_queue.empty())
		{
			LTCPTask *task = _queue.back();
			_queue.pop_back();
			_add(task);
		}
		_size = 0;
		mlock.unlock();
	}
protected:
	virtual void _add(LTCPTask *task) = 0;
	DWORD _size;
private:
	LMutex mlock;
	//std::queue<LTCPTask *,std::deque<LTCPTask *,__gnu_cxx::__pool_alloc<LTCPTask *> > > _queue;
	std::vector<LTCPTask *> _queue;
};

/**
* \brief 处理TCP连接的验证,如果验证不通过,需要回收这个连接
*
*/
class LVerifyThread : public LThread,public LTCPTaskQueue
{

private:

	LTCPTaskPool *pool;
	LTCPTaskContainer tasks;  /**< 任务列表 */
	LTCPTaskContainer::size_type task_count;      /**< tasks计数(保证线程安全*/
//	pollfdContainer pfds;

	LMutex m_Lock;
	/**
	* \brief 添加一个连接任务
	* \param task 连接任务
	*/
	void _add(LTCPTask *task)
	{
		printf("LVerifyThread::_add\n");
		m_Lock.lock();
		//struct pollfd pfd;
		//task->fillPollFD(pfd,POLLIN | POLLPRI);
		tasks.push_back(task);
		task_count = tasks.size();
	//	pfds.push_back(pfd);
		m_Lock.unlock();
		printf("LVerifyThread::_add_end\n");
	}


	void remove(LTCPTask_IT &it,int p)
	{
		//int i;
		//pollfdContainer::iterator iter;
		//bool bDeleted = false;
		//m_Lock.lock();
		//for(iter = pfds.begin(),i = 0; iter != pfds.end(); iter++,i++)
		//{
		//	if (i == p)
		//	{
		//		pfds.erase(iter);
		//		it = tasks.erase(it);
		//		task_count = tasks.size();
		//		g_CsizeV = task_count;
		//		bDeleted = true;
		//		break;
		//	}
		//}
		//m_Lock.unlock();
		//if( !bDeleted )
		//{
		//	int iii = 0;
		//}
	}

public:

	/**
	* \brief 构造函数
	* \param pool 所属的连接池
	* \param name 线程名称
	*/
	LVerifyThread(
		LTCPTaskPool *pool,
		const std::string &name = std::string("LVerifyThread"))
		: LThread(name),pool(pool)
	{
		task_count = 0;
	}

	/**
	* \brief 析构函数
	*
	*/
	~LVerifyThread()
	{
	}

	void run();

};

/**
* \brief 等待接受验证指令,并进行验证
*
*/
void LVerifyThread::run()
{
	printf("LVerifyThread::run\n");

	LRTime currentTime;
	LTCPTask_IT it,next;
	pollfdContainer::size_type i;
	DWORD dwBeginTime = 0;
	while(!isFinal())
	{

		//fprintf(stderr,"LVerifyThread::run\n");

		//if( dwBeginTime != 0 )
		//{
		//	printf("zVerifyThread循环时间：%d ms", GetTickCount() - dwBeginTime);
		//}

		//dwBeginTime = GetTickCount();


		currentTime.now();

		check_queue();
		//if (!pfds.empty())
		{
			m_Lock.lock();
			if(!tasks.empty())
			{
				for(i = 0,it = tasks.begin();  it != tasks.end();)
				{
					LTCPTask *task = *it;
					if (task->checkVerifyTimeout(currentTime))
					{
						//超过指定时间验证还没有通过,需要回收连接
						it = tasks.erase(it);
						task_count = tasks.size();
						task->resetState();
						pool->addRecycle(task);
					}
					else
					{
						i ++;
						it++;
					}
				}
				if(!tasks.empty())
				{
					int i;
					bool status = false;

					for(i = 0,it = tasks.begin(); it != tasks.end();)
					{						
						printf("verify2 %d",tasks.size());
						LTCPTask *task = *it;
						int ret = task->WaitRecv( false );
						if ( ret == -1 )
						{
							//套接口出现错误
							it = tasks.erase(it);
							task_count = tasks.size();
							task->resetState();
							printf("套接口错误回收\n");
							pool->addRecycle(task);
						}
						else if( ret > 0 )
						{
							switch(task->verifyConn())
							{
							case 1:
								//验证成功
								it = tasks.erase(it);
								task_count = tasks.size();
								//再做唯一性验证
								if (task->uniqueAdd())
								{
									//唯一性验证成功,获取下一个状态
									printf("客户端唯一性验证成功\n");
									task->setUnique();
									pool->addSync(task);
								}
								else
								{
									//唯一性验证失败,回收连接任务
									printf("客户端唯一性验证失败\n");
									task->resetState();
									printf("唯一性验证失败回收\n");
									pool->addRecycle(task);
								}
								break;

							case -1:
								//验证失败,回收任务
								it = tasks.erase(it);
								task_count = tasks.size();
								task->resetState();
								printf("验证失败回收\n");
								pool->addRecycle(task);
								break;	
							default:
								//超时,下面会处理
								i++;
								it++;
								break;
							}
						}
						else
						{
							i++;
							it++;
						}
					}
				}
			}
			m_Lock.unlock();
		}

		LThread::msleep(50);
	}

	//把所有等待验证队列中的连接加入到回收队列中,回收这些连接

	fprintf(stderr,"LVerifyThread::final\n");

	if(tasks.size() == 0)
		return;
	for(i = 0,it = tasks.begin(); it != tasks.end();)
	{
		LTCPTask *task = *it;
		remove(it,i);
		task->resetState();
		pool->addRecycle(task);
	}
}

/**
* \brief 等待其它线程同步验证这个连接,如果失败或者超时,都需要回收连接
*
*/
class LSyncThread : public LThread,public LTCPTaskQueue
{

private:

	LTCPTaskPool *pool;
	LTCPTaskContainer tasks;  /**< 任务列表 */

	LMutex m_Lock;
	void _add(LTCPTask *task)
	{
		m_Lock.lock();
		tasks.push_back(task);
		m_Lock.unlock();
	}

public:

	/**
	* \brief 构造函数
	* \param pool 所属的连接池
	* \param name 线程名称
	*/
	LSyncThread(
		LTCPTaskPool *pool,
		const std::string &name = std::string("LSyncThread"))
		: LThread(name),pool(pool)
	{}

	/**
	* \brief 析构函数
	*
	*/
	~LSyncThread() {};

	void run();

};

/**
* \brief 等待其它线程同步验证这个连接
*
*/
void LSyncThread::run()
{
	printf("LSyncThread::run\n");
	LTCPTask_IT it;
	DWORD dwBeginTime = 0;
	while(!isFinal())
	{

		//fprintf(stderr,"LVerifyThread::run\n");

		//if( dwBeginTime != 0 )
		//{
		//	printf("zSyncThread循环时间：%d ms", GetTickCount() - dwBeginTime);
		//}

		//dwBeginTime = GetTickCount();

		//fprintf(stderr,"LSyncThread::run\n");
		check_queue();

		m_Lock.lock();
		if (!tasks.empty())
		{
			for(it = tasks.begin(); it != tasks.end();)
			{
				LTCPTask *task = (*it);
				switch(task->waitSync())
				{
				case 1:
					//等待其它线程同步验证成功
					it = tasks.erase(it);
					if (!pool->addOkay(task))
					{
						task->resetState();
						pool->addRecycle(task);
					}
					break;
				case 0:
					it++;
					break;
				case -1:
					//等待其它线程同步验证失败,需要回收连接
					it = tasks.erase(it);
					task->resetState();
					pool->addRecycle(task);
					break;
				}
			}
		}
		m_Lock.unlock();
		LThread::msleep(200);
	}

	fprintf(stderr,"LSyncThread::final\n");
	//把所有等待同步验证队列中的连接加入到回收队列中,回收这些连接
	for(it = tasks.begin(); it != tasks.end();)
	{
		LTCPTask *task = *it;
		it = tasks.erase(it);
		task->resetState();
		pool->addRecycle(task);
	}
}

/**
* \brief TCP连接的主处理线程,一般一个线程带几个TCP连接,这样可以显著提高效率
*
*/
class LOkayThread : public LThread,public LTCPTaskQueue
{

private:

	Timer  _one_sec_; // 秒定时器
	LTCPTaskPool *pool;
	LTCPTaskContainer tasks;  /**< 任务列表 */
	LTCPTaskContainer::size_type task_count;      /**< tasks计数(保证线程安全*/

	//pollfdContainer pfds;

	LMutex m_Lock;

	void _add(LTCPTask *task)
	{
		m_Lock.lock();
	//	struct pollfd pfd;
	//	task->fillPollFD(pfd,POLLIN | POLLOUT | POLLPRI);
		tasks.push_back(task);
		task_count = tasks.size();
		//pfds.push_back(pfd);
		task->ListeningRecv(false);
		m_Lock.unlock();
	}


	void remove(LTCPTask_IT &it,int p)
	{
		//int i;
		//pollfdContainer::iterator iter;
		//bool bDeleted = false;
		//m_Lock.lock();
		//for(iter = pfds.begin(),i = 0; iter != pfds.end(); iter++,i++)
		//{
		//	if (i == p)
		//	{
		//		pfds.erase(iter);
		//		it = tasks.erase(it);
		//		task_count = tasks.size();
		//		g_CsizeO = task_count;
		//		bDeleted = true;
		//		break;
		//	}
		//}
		//m_Lock.unlock();
		//if( !bDeleted )
		//{
		//	int iii = 0;
		//}
	}

public:

	static const LTCPTaskContainer::size_type connPerThread = 512;  /**< 每个线程带的连接数量 */

	/**
	* \brief 构造函数
	* \param pool 所属的连接池
	* \param name 线程名称
	*/
	LOkayThread(
		LTCPTaskPool *pool,
		const std::string &name = std::string("LOkayThread"))
		: LThread(name),pool(pool),_one_sec_(1)
	{
		task_count = 0;
	}

	/**
	* \brief 析构函数
	*
	*/
	~LOkayThread()
	{
	}

	void run();

	/**
	* \brief 返回连接任务的个数
	* \return 这个线程处理的连接任务数
	*/
	const LTCPTaskContainer::size_type size() const
	{
		return task_count + _size;
	}

};

/**
* \brief 主处理线程,回调处理连接的输入输出指令
*
*/
void LOkayThread::run()
{
	printf("LOkayThread::run\n");

	LRTime currentTime;
	LTCPTask_IT it,next;
	pollfdContainer::size_type i;

	int time = pool->usleep_time;
	pollfdContainer::iterator iter_r;
	pollfdContainer pfds_r;
	LTCPTaskContainer tasks_r;    
	bool check=false;
	DWORD dwBeginTime = 0;
	while(!isFinal())
	{

		//fprintf(stderr,"LVerifyThread::run\n");

		//if( dwBeginTime != 0 )
		//{
		//	printf("zOkayThread循环时间：%d ms", GetTickCount() - dwBeginTime);
		//}

		//dwBeginTime = GetTickCount();
		currentTime.now();
		check_queue();
		if (check)
		{
			m_Lock.lock();
			if (!tasks.empty())
			{
				for(i = 0,it = tasks.begin(); it != tasks.end(); )
				{
					LTCPTask *task = *it;
					//检查测试信号指令
					task->checkSignal(currentTime);

					if (task->isTerminateWait())
					{
						task->Terminate();
					}
					if (task->isTerminate())
					{
						it = tasks.erase(it);
						task_count = tasks.size();
						// state_sync -> state_okay
						/*
						* whj
						* 先设置状态再添加容器,
						* 否则会导致一个task同时在两个线程中的危险情况
						*/
						task->getNextState();
						pool->addRecycle(task);
					}
					else
					{
						i ++;
						it ++;
					}
					//else
					//{
					//	pfds[i].revents = 0;
					//}
				}
			}
			m_Lock.unlock();
			check=false;
		}
		LThread::usleep(time);
		time = 0;
		if (check)
		{
			if (time <=0)
			{
				time = 0;
			}
			continue;
		}
		if (time <=0)
		{
			m_Lock.lock();
			if (!tasks.empty())
			{
				for(i = 0,it = tasks.begin(); it != tasks.end(); it++,i++)
				{
					LTCPTask *task = (*it);

					bool UseIocp = task->UseIocp();

					if( UseIocp )
					{ 
						int retcode = task->WaitRecv( false );
						if ( retcode == -1 )
						{
							//套接口出现错误
							printf("LOkayThread::run: 套接口异常错误");
							task->Terminate(LTCPTask::terminate_active);
						}
						else if( retcode > 0 )
						{
							//套接口准备好了读取操作
							if (!task->ListeningRecv(true))
							{
								printf("LOkayThread::run: 套接口读操作错误");
								task->Terminate(LTCPTask::terminate_active);
							}
						}
						retcode = task->WaitSend( false );
						if( retcode == -1 )
						{
							//套接口出现错误
							printf("LOkayThread::run: 套接口异常错误");
							task->Terminate(LTCPTask::terminate_active);
						}
						else if( retcode ==  1 )
						{
							//套接口准备好了写入操作
							if (!task->ListeningSend())
							{
								printf("LOkayThread::run: 套接口写操作错误 port = %u",task->getPort());

								task->Terminate(LTCPTask::terminate_active);
							}
						}
					}						
				}
			}
			m_Lock.unlock();
			time = pool->usleep_time;
		}
		check=true;
	}

	//把所有任务队列中的连接加入到回收队列中,回收这些连接

	fprintf(stderr,"LOkayThread::final\n");


	if(tasks.size() == 0)
		return;

	for(i = 0,it = tasks.begin(); it != tasks.end();)
	{
		LTCPTask *task = *it;
		it = tasks.erase(it);
		//state_sync -> state_okay
		/*
		* whj
		* 先设置状态再添加容器,
		* 否则会导致一个task同时在两个线程中的危险情况
		*/
		task->getNextState();
		pool->addRecycle(task);
	}
}

/**
* \brief 连接回收线程,回收所有无用的TCP连接,释放相应的资源
*
*/
DWORD dwStep[100];
class LRecycleThread : public LThread,public LTCPTaskQueue
{

private:

	LTCPTaskPool *pool;
	LTCPTaskContainer tasks;  /**< 任务列表 */

	LMutex m_Lock;

	void _add(LTCPTask *task)
	{
		m_Lock.lock();
		tasks.push_back(task);
		m_Lock.unlock();
	}

public:

	/**
	* \brief 构造函数
	* \param pool 所属的连接池
	* \param name 线程名称
	*/
	LRecycleThread(
		LTCPTaskPool *pool,
		const std::string &name = std::string("LRecycleThread"))
		: LThread(name),pool(pool)
	{}

	/**
	* \brief 析构函数
	*
	*/
	~LRecycleThread() {};

	void run();

};

/**
* \brief 连接回收处理线程,在删除内存空间之前需要保证recycleConn返回1
*
*/
//std::map<LTCPTask*,int> g_RecycleLog;
void LRecycleThread::run()
{
	printf("LRecycleThread::run\n");
	LTCPTask_IT it;
	DWORD dwBeginTime = 0;
	while(!isFinal())
	{		
		//fprintf(stderr,"LVerifyThread::run\n");

		//if( dwBeginTime != 0 )
		//{
		//	printf("zRecycleThread循环时间：%d ms", GetTickCount() - dwBeginTime);
		//}

		//dwBeginTime = GetTickCount();
		//fprintf(stderr,"LRecycleThread::run\n");
		check_queue();

		DWORD dwLog = 0;
		int i;
		m_Lock.lock();
		if (!tasks.empty())
		{
			for(i = 0,it = tasks.begin(); it != tasks.end();i++)
			{
				LTCPTask *task = *it;
				switch(task->recycleConn())
				{
				case 1:
					//回收处理完成可以释放相应的资源
					it = tasks.erase(it);
					if (task->isUnique())
						//如果已经通过了唯一性验证,从全局唯一容器中删除
						task->uniqueRemove();
					task->getNextState();
					//				if( !task->UseIocp() ) // [ranqd] 使用Iocp的连接不在这里回收
//					g_RecycleLog[task] = 0;
					SAFE_DELETE(task);
					break;
				default:
					//回收超时,下次再处理
					it++;
					break;
				}
			}
		}
		m_Lock.unlock();

		LThread::msleep(200);
	}

	//回收所有的连接

	fprintf(stderr,"LRecycleThread::final\n");
	for(it = tasks.begin(); it != tasks.end();)
	{
		//回收处理完成可以释放相应的资源
		LTCPTask *task = *it;
		it = tasks.erase(it);
		if (task->isUnique())
			//如果已经通过了唯一性验证,从全局唯一容器中删除
			task->uniqueRemove();
		task->getNextState();
		SAFE_DELETE(task);
	}
}


/**
* \brief 返回连接池中子连接个数
*
*/
const int LTCPTaskPool::getSize()
{
	printf("LTCPTaskPool::getSize\n");
	struct MyCallback : LThreadGroup::Callback
	{
		int size;
		MyCallback() : size(0) {}
		void exec(LThread *e)
		{
			LOkayThread *pOkayThread = (LOkayThread *)e;
			size += pOkayThread->size();
		}
	};
	MyCallback mcb;
	okayThreads.execAll(mcb);
	return mcb.size;
}

/**
* \brief 把一个TCP连接添加到验证队列中,因为存在多个验证队列,需要按照一定的算法添加到不同的验证处理队列中
*
* \param task 一个连接任务
*/
bool LTCPTaskPool::addVerify(LTCPTask *task)
{

	printf("LTCPTaskPool::addVerify\n");
	//因为存在多个验证队列,需要按照一定的算法添加到不同的验证处理队列中
	static DWORD hashcode = 0;
	LVerifyThread *pVerifyThread = (LVerifyThread *)verifyThreads.getByIndex(hashcode++ % maxVerifyThreads);
	if (pVerifyThread)
	{
		// state_sync -> state_okay
		/*
		* whj
		* 先设置状态再添加容器,
		* 否则会导致一个task同时在两个线程中的危险情况
		*/
		task->getNextState();
		pVerifyThread->add(task);
	}
	return true;
}

/**
* \brief 把一个通过验证的TCP连接添加到等待同步验证队列中
*
* \param task 一个连接任务
*/
void LTCPTaskPool::addSync(LTCPTask *task)
{
	printf("LTCPTaskPool::addSync\n");
	// state_sync -> state_okay
	/*
	* whj
	* 先设置状态再添加容器,
	* 否则会导致一个task同时在两个线程中的危险情况
	*/
	task->getNextState();
	syncThread->add(task);
}

/**
* \brief 把一个通过验证的TCP处理队列中
*
* \param task 一个连接任务
* \return 添加是否成功
*/
bool LTCPTaskPool::addOkay(LTCPTask *task)
{
	printf("LTCPTaskPool::addOkay\n");
	//首先遍历所有的线程,找出运行的并且连接数最少的线程,再找出没有启动的线程
	LOkayThread *pmin = NULL,*nostart = NULL;
	for(int i = 0; i < maxThreadCount; i++)
	{
		LOkayThread *pOkayThread = (LOkayThread *)okayThreads.getByIndex(i);
		if (pOkayThread)
		{
			if (pOkayThread->isAlive())
			{
				if (NULL == pmin || pmin->size() > pOkayThread->size())
					pmin = pOkayThread;
			}
			else
			{
				nostart = pOkayThread;
				break;
			}
		}
	}
	if (pmin && pmin->size() < LOkayThread::connPerThread)
	{
		// state_sync -> state_okay
		/*
		* whj
		* 先设置状态再添加容器,
		* 否则会导致一个task同时在两个线程中的危险情况
		*/
		task->getNextState();
		//这个线程同时处理的连接数还没有到达上限
		pmin->add(task);
		return true;
	}
	if (nostart)
	{
		//线程还没有运行,需要创建线程,再把添加到这个线程的处理队列中
		if (nostart->start())
		{
			printf("zTCPTaskPool创建工作线程\n");
			// state_sync -> state_okay
			/*
			* whj
			* 先设置状态再添加容器,
			* 否则会导致一个task同时在两个线程中的危险情况
			*/
			task->getNextState();
			//这个线程同时处理的连接数还没有到达上限
			nostart->add(task);
			return true;
		}
		else
			printf("zTCPTaskPool不能创建工作线程");
	}

	printf("zTCPTaskPool没有找到合适的线程来处理连接");
	//没有找到线程来处理这个连接,需要回收关闭连接
	return false;
}

/**
* \brief 把一个TCP连接添加到回收处理队列中
*
* \param task 一个连接任务
*/

void LTCPTaskPool::addRecycle(LTCPTask *task)
{
	printf("LTCPTaskPool::addRecycle\n");
	//if( g_RecycleLog[task] == 0 )
	//{
	//	g_RecycleLog[task] = 1;
	//}
	//else
	//{
	//	int iii = 0;
	//}
	recycleThread->add(task);
}


/**
* \brief 初始化线程池,预先创建各种线程
*
* \return 初始化是否成功
*/
bool LTCPTaskPool::init()
{
	printf("LTCPTaskPool::init\n");
	//创建初始化验证线程
	for(int i = 0; i < maxVerifyThreads; i++)
	{
		std::ostringstream name;
		name << "LVerifyThread[" << i << "]";
		LVerifyThread *pVerifyThread = new LVerifyThread(this,name.str());
		if (NULL == pVerifyThread)
			return false;
		if (!pVerifyThread->start())
			return false;
		verifyThreads.add(pVerifyThread);
	}

	//创建初始化等待同步验证线程
	syncThread = new LSyncThread(this);
	if (syncThread && !syncThread->start())
		return false;

	//创建初始化主运行线程池
	maxThreadCount = (maxConns + LOkayThread::connPerThread - 1) / LOkayThread::connPerThread;
	printf("最大TCP连接数%d,每线程TCP连接数%d,线程个数%d\n",maxConns,LOkayThread::connPerThread,maxThreadCount);
	for(int i = 0; i < maxThreadCount; i++)
	{
		std::ostringstream name;
		name << "LOkayThread[" << i << "]";
		LOkayThread *pOkayThread = new LOkayThread(this,name.str());
		if (NULL == pOkayThread)
			return false;
		if (i < minThreadCount && !pOkayThread->start())
			return false;
		okayThreads.add(pOkayThread);
	}

	//创建初始化回收线程池
	recycleThread = new LRecycleThread(this);
	if (recycleThread && !recycleThread->start())
		return false;

	return true;
}

/**
* \brief 释放线程池,释放各种资源,等待各种线程退出
*
*/
void LTCPTaskPool::final()
{
	verifyThreads.joinAll();
	if (syncThread)
	{
		syncThread->final();
		syncThread->join();
		SAFE_DELETE(syncThread);
	}

	okayThreads.joinAll();
	if (recycleThread)
	{
		recycleThread->final();
		recycleThread->join();
		SAFE_DELETE(recycleThread);
	}
}

