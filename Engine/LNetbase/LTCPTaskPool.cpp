/**
 * \file
 * \version  $Id: LTCPTaskPool.cpp  $
 * \author  
 * \date 
 * \brief 实现线程池类，用于处理多连接服务器
 *
 * 
 */


#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <assert.h>
#include <ext/pool_allocator.h>

#include "LSocket.h"
#include "LThread.h"
#include "LTCPTaskPool.h"
#include "LTime.h"

int LTCPTaskPool::usleep_time=50000;										/**< 循环等待时间 */
uint16_t LTCPTaskPool::connPerThread=512;
/**
 * \brief 连接任务链表
 *
 */
#ifdef _POOL_ALLOC_
typedef std::list<LTCPTask *, __gnu_cxx::__pool_alloc<LTCPTask *> > zTCPTaskContainer;
#else
typedef std::list<LTCPTask * > zTCPTaskContainer;
#endif

/**
 * \brief 连接任务链表叠代器
 *
 */
typedef zTCPTaskContainer::iterator zTCPTask_IT;

#ifdef _USE_EPOLL_
#ifdef _POOL_ALLOC_
typedef std::vector<struct epoll_event, __gnu_cxx::__pool_alloc<epoll_event> > epollfdContainer;
#else
typedef std::vector<struct epoll_event> epollfdContainer;
#endif
#else
typedef std::vector<struct pollfd> pollfdContainer;
#endif

class LTCPTaskQueue
{
	public:
		LTCPTaskQueue() :_size(0) {}
		virtual ~LTCPTaskQueue() {}
		inline void add(LTCPTask *task)
		{
			mlock.lock();
			_queue.push(task);
			_size++;
			mlock.unlock();
		}
		inline void check_queue()
		{
			mlock.lock();
			while(!_queue.empty())
			{
				LTCPTask *task = _queue.front();
				_queue.pop();
				_add(task);
			}
			_size = 0;
			mlock.unlock();
		}
	protected:
		virtual void _add(LTCPTask *task) = 0;
		unsigned int _size;
	private:
		LMutex mlock;
#ifdef _POOL_ALLOC_
		std::queue<LTCPTask *, std::deque<LTCPTask *, __gnu_cxx::__pool_alloc<LTCPTask *> > > _queue;
#else
		std::queue<LTCPTask *, std::deque<LTCPTask * > > _queue;
#endif
};

/**
 * \brief 处理TCP连接的验证，如果验证不通过，需要回收这个连接
 *
 */
class LVerifyThread : public LThread, public LTCPTaskQueue
{

	private:

		LTCPTaskPool *pool;
		zTCPTaskContainer tasks;	/**< 任务列表 */
		zTCPTaskContainer::size_type task_count;			/**< tasks计数(保证线程安全*/
		int kdpfd;
		epollfdContainer epfds;

		/**
		 * \brief 添加一个连接任务
		 * \param task 连接任务
		 */
		void _add(LTCPTask *task)
		{
			task->addEpoll(kdpfd, EPOLLIN | EPOLLERR | EPOLLPRI, (void *)task);
			tasks.push_back(task);
			task_count = tasks.size();;
			if (task_count > epfds.size())
			{
				epfds.resize(task_count + 16);
			}
		}

		void remove(LTCPTask *task)
		{
			task->delEpoll(kdpfd, EPOLLIN | EPOLLERR | EPOLLPRI);
			tasks.remove(task);
			task_count = tasks.size();
		}
		void remove(zTCPTask_IT &it)
		{
			(*it)->delEpoll(kdpfd, EPOLLIN | EPOLLERR | EPOLLPRI);
			tasks.erase(it);
			task_count = tasks.size();
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
			: LThread(name), pool(pool)
		{
			task_count = 0;
			kdpfd = epoll_create(256);
			assert(-1 != kdpfd);
			epfds.resize(256);
		}

		/**
		 * \brief 析构函数
		 *
		 */
		~LVerifyThread()
		{
			TEMP_FAILURE_RETRY(::close(kdpfd));
		}

		void run();

};

/**
 * \brief 等待接受验证指令，并进行验证
 *
 */
void LVerifyThread::run()
{
	LRTime currentTime;
	zTCPTask_IT it, next;

	while(!isFinal())
	{
		currentTime.now();

		check_queue();
		if (!tasks.empty())
		{
			for(it = tasks.begin(), next = it, next++; it != tasks.end(); it = next, next++)
			{
				LTCPTask *task = *it;
				if (task->checkVerifyTimeout(currentTime))
				{
					//超过指定时间验证还没有通过，需要回收连接
					remove(it);
					task->resetState();
					pool->addRecycle(task);
				}
			}

			int retcode = epoll_wait(kdpfd, &epfds[0], task_count, 0);
			if (retcode > 0)
			{
				for(int i = 0; i < retcode; i++)
				{
					LTCPTask *task = (LTCPTask *)epfds[i].data.ptr;
					if (epfds[i].events & (EPOLLERR | EPOLLPRI))
					{
						//套接口出现错误
						remove(task);
						task->resetState();
						pool->addRecycle(task);
					}
					else if (epfds[i].events & EPOLLIN)
					{
						switch(task->verifyConn())
						{
							case 1:
								//验证成功
								remove(task);
								//再做唯一性验证
								if (task->uniqueAdd())
								{
									//唯一性验证成功，获取下一个状态
									task->setUnique();
									pool->addSync(task);
								}
								else
								{
									//唯一性验证失败，回收连接任务
									task->resetState();
									pool->addRecycle(task);
								}
								break;
							case 0:
								//超时，下面会处理
								break;
							case -1:
								//验证失败，回收任务
								remove(task);
								task->resetState();
								pool->addRecycle(task);
								break;
						}
					}
				}
			}
		}

		LThread::msleep(50);
	}

	//把所有等待验证队列中的连接加入到回收队列中，回收这些连接
	for(it = tasks.begin(), next = it, next++; it != tasks.end(); it = next, next++)
	{
		LTCPTask *task = *it;
		remove(it);
		task->resetState();
		pool->addRecycle(task);
	}
}

/**
 * \brief 等待其它线程同步验证这个连接，如果失败或者超时，都需要回收连接
 *
 */
class LSyncThread : public LThread, public LTCPTaskQueue
{

	private:

		LTCPTaskPool *pool;
		zTCPTaskContainer tasks;	/**< 任务列表 */

		void _add(LTCPTask *task)
		{
			tasks.push_front(task);
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
			: LThread(name), pool(pool)
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
	zTCPTask_IT it;

	while(!isFinal())
	{
		check_queue();

		if (!tasks.empty())
		{
			for(it = tasks.begin(); it != tasks.end();)
			{
				LTCPTask *task = *it;
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
						//等待其它线程同步验证失败，需要回收连接
						it = tasks.erase(it);
						task->resetState();
						pool->addRecycle(task);
						break;
				}
			}
		}

		LThread::msleep(200);
	}

	//把所有等待同步验证队列中的连接加入到回收队列中，回收这些连接
	for(it = tasks.begin(); it != tasks.end();)
	{
		LTCPTask *task = *it;
		it = tasks.erase(it);
		task->resetState();
		pool->addRecycle(task);
	}
}

/**
 * \brief TCP连接的主处理线程，一般一个线程带几个TCP连接，这样可以显著提高效率
 *
 */
class LOkayThread : public LThread, public LTCPTaskQueue
{

	private:

		LTCPTaskPool *pool;
		zTCPTaskContainer tasks;	/**< 任务列表 */
		zTCPTaskContainer::size_type task_count;			/**< tasks计数(保证线程安全*/
		int kdpfd;
		epollfdContainer epfds;

		void _add(LTCPTask *task)
		{
			task->addEpoll(kdpfd, EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLPRI, (void *)task);
			tasks.push_back(task);
			task_count = tasks.size();
			if (task_count > epfds.size())
			{
				epfds.resize(task_count + 16);
			}
			task->ListeningRecv(false);
		}

		void remove(zTCPTask_IT &it)
		{
			(*it)->delEpoll(kdpfd, EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLPRI);
			tasks.erase(it);
			task_count = tasks.size();
		}

	public:


		/**
		 * \brief 构造函数
		 * \param pool 所属的连接池
		 * \param name 线程名称
		 */
		LOkayThread(
				LTCPTaskPool *pool,
				const std::string &name = std::string("LOkayThread"))
			: LThread(name), pool(pool)
		{
			task_count = 0;
			kdpfd = epoll_create(LTCPTaskPool::connPerThread);
			assert(-1 != kdpfd);
			epfds.resize(LTCPTaskPool::connPerThread);
		}

		/**
		 * \brief 析构函数
		 *
		 */
		~LOkayThread()
		{
			TEMP_FAILURE_RETRY(::close(kdpfd));
		}

		void run();

		/**
		 * \brief 返回连接任务的个数
		 * \return 这个线程处理的连接任务数
		 */
		const zTCPTaskContainer::size_type size() const
		{
			return task_count + _size;
		}

};

/**
 * \brief 主处理线程，回调处理连接的输入输出指令
 *
 */
void LOkayThread::run()
{
	LRTime currentTime;
	zTCPTask_IT it, next;

	int kdpfd_r;
	epollfdContainer epfds_r;
	kdpfd_r = epoll_create(256);
	assert(-1 != kdpfd_r);
	epfds.resize(256);
	DWORD fds_count_r = 0;
	int time = pool->usleep_time;
	bool check=false;
	while(!isFinal())
	{
		currentTime.now();

		if (check) 
		{
			check_queue();
			if (!tasks.empty())
			{
				for(it = tasks.begin(), next = it, next++; it != tasks.end(); it = next, next++)
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
						if (task->isFdsrAdd())
						{
							task->delEpoll(kdpfd_r, EPOLLIN | EPOLLERR | EPOLLPRI);
							fds_count_r --;
						}
						remove(it);
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
						if(!task->isFdsrAdd())
						{
							task->addEpoll(kdpfd_r, EPOLLIN | EPOLLERR | EPOLLPRI, (void *)task);
							task->fdsrAdd(); 
							fds_count_r++;
							if (fds_count_r > epfds_r.size())
							{
								epfds_r.resize(fds_count_r + 16);
							}
						}
					}
				}
			}
			check=false;
		}

		if(fds_count_r)
		{
			struct timeval _tv_1;
			struct timeval _tv_2;
			gettimeofday(&_tv_1,NULL);
			int retcode = epoll_wait(kdpfd_r, &epfds_r[0], fds_count_r, time/1000);
			if (retcode > 0)
			{
				for(int i = 0; i < retcode; i++)
				{
					LTCPTask *task = (LTCPTask *)epfds_r[i].data.ptr;
					if (epfds_r[i].events & (EPOLLERR | EPOLLPRI))
					{
						//套接口出现错误
						task->Terminate(LTCPTask::terminate_active);
						check=true;
					}
					else
					{
						if (epfds_r[i].events & EPOLLIN)
						{
							//套接口准备好了读取操作
							if (!task->ListeningRecv(true))
							{
								task->Terminate(LTCPTask::terminate_active);
								check=true;
							}
						}
					}
					epfds_r[i].events=0; 
				}
			}
			gettimeofday(&_tv_2,NULL);
			int end=_tv_2.tv_sec*1000000 + _tv_2.tv_usec;
			int begin= _tv_1.tv_sec*1000000 + _tv_1.tv_usec;
			time = time - (end - begin);
		}
		else
		{
			LThread::usleep(time);
			time = 0;
		}
		if(check)
		{
			if(time <=0)
			{
				time = 0;
			}
			continue;
		}
		if (time <=0)
		{
			if (!tasks.empty())
			{
				int retcode = epoll_wait(kdpfd, &epfds[0], task_count, 0);
				if (retcode > 0)
				{
					for(int i = 0; i < retcode; i++)
					{
						LTCPTask *task = (LTCPTask *)epfds[i].data.ptr;
						if (epfds[i].events & (EPOLLERR | EPOLLPRI))
						{
							//套接口出现错误
							task->Terminate(LTCPTask::terminate_active);
						}
						else
						{
							if (epfds[i].events & EPOLLIN)
							{
								//套接口准备好了读取操作
								if (!task->ListeningRecv(true))
								{
									task->Terminate(LTCPTask::terminate_active);
								}
							}
							if (epfds[i].events & EPOLLOUT)
							{
								//套接口准备好了写入操作
								if (!task->ListeningSend())
								{
									task->Terminate(LTCPTask::terminate_active);
								}
							}
						}
						epfds[i].events=0; 
					}
				}
				time = pool->usleep_time;
			}
			check=true;
		}

		//LThread::usleep(pool->usleep_time);
	}

	//把所有任务队列中的连接加入到回收队列中，回收这些连接
	for(it = tasks.begin(), next = it, next++; it != tasks.end(); it = next, next++)
	{
		LTCPTask *task = *it;
		remove(it);
		// state_sync -> state_okay
		/*
		 * whj
		 * 先设置状态再添加容器,
		 * 否则会导致一个task同时在两个线程中的危险情况
		 */
		task->getNextState();
		pool->addRecycle(task);
	}
	TEMP_FAILURE_RETRY(::close(kdpfd_r));
}

/**
 * \brief 连接回收线程，回收所有无用的TCP连接，释放相应的资源
 *
 */
class zRecycleThread : public LThread, public LTCPTaskQueue
{

	private:

		LTCPTaskPool *pool;
		zTCPTaskContainer tasks;	/**< 任务列表 */

		void _add(LTCPTask *task)
		{
			tasks.push_front(task);
		}

	public:

		/**
		 * \brief 构造函数
		 * \param pool 所属的连接池
		 * \param name 线程名称
		 */
		zRecycleThread(
				LTCPTaskPool *pool,
				const std::string &name = std::string("zRecycleThread"))
			: LThread(name), pool(pool)
			{}

		/**
		 * \brief 析构函数
		 *
		 */
		~zRecycleThread() {};

		void run();

};

/**
 * \brief 连接回收处理线程，在删除内存空间之前需要保证recycleConn返回1
 *
 */
void zRecycleThread::run()
{
	zTCPTask_IT it;

	while(!isFinal())
	{
		check_queue();

		if (!tasks.empty())
		{
			for(it = tasks.begin(); it != tasks.end();)
			{
				LTCPTask *task = *it;
				switch(task->recycleConn())
				{
					case 1:
						//回收处理完成可以释放相应的资源
						it = tasks.erase(it);
						if (task->isUnique())
							//如果已经通过了唯一性验证，从全局唯一容器中删除
							task->uniqueRemove();
						task->getNextState();
						SAFE_DELETE(task);
						break;
					case 0:
						//回收超时，下次再处理
						it++;
						break;
				}
			}
		}

		LThread::msleep(200);
	}

	//回收所有的连接
	for(it = tasks.begin(); it != tasks.end();)
	{
		//回收处理完成可以释放相应的资源
		LTCPTask *task = *it;
		it = tasks.erase(it);
		if (task->isUnique())
			//如果已经通过了唯一性验证，从全局唯一容器中删除
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
	struct MyCallback : zThreadGroup::Callback
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
 * \brief 把一个TCP连接添加到验证队列中，因为存在多个验证队列，需要按照一定的算法添加到不同的验证处理队列中
 *
 * \param task 一个连接任务
 */
bool LTCPTaskPool::addVerify(LTCPTask *task)
{

	//因为存在多个验证队列，需要按照一定的算法添加到不同的验证处理队列中
	static unsigned int hashcode = 0;
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
	//首先便利所有的线程，找出运行的并且连接数最少的线程，再找出没有启动的线程
	LOkayThread *min = NULL, *nostart = NULL;
	for(int i = 0; i < maxThreadCount; i++)
	{
		LOkayThread *pOkayThread = (LOkayThread *)okayThreads.getByIndex(i);
		if (pOkayThread)
		{
			if (pOkayThread->isAlive())
			{
				if (NULL == min || min->size() > pOkayThread->size())
					min = pOkayThread;
			}
			else
			{
				nostart = pOkayThread;
				break;
			}
		}
	}
	if (min && min->size() < LTCPTaskPool::connPerThread)
	{
		// state_sync -> state_okay
		/*
		 * whj
		 * 先设置状态再添加容器,
		 * 否则会导致一个task同时在两个线程中的危险情况
		 */
		task->getNextState();
		//这个线程同时处理的连接数还没有到达上限
		min->add(task);
		return true;
	}
	if (nostart)
	{
		//线程还没有运行，需要创建线程，再把添加到这个线程的处理队列中
		if (nostart->start())
		{
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
		else{}
	}

	//没有找到线程来处理这个连接，需要回收关闭连接
	return false;
}

/**
 * \brief 把一个TCP连接添加到回收处理队列中
 *
 * \param task 一个连接任务
 */
void LTCPTaskPool::addRecycle(LTCPTask *task)
{
	recycleThread->add(task);
}


/**
 * \brief 初始化线程池，预先创建各种线程
 *
 * \return 初始化是否成功
 */
bool LTCPTaskPool::init()
{
	//创建初始化验证线程
	for(int i = 0; i < maxVerifyThreads; i++)
	{
		std::ostringstream name;
		name << "LVerifyThread[" << i << "]";
		LVerifyThread *pVerifyThread = new LVerifyThread(this, name.str());
		NEW_CHECK(pVerifyThread);
		if (NULL == pVerifyThread)
			return false;
		if (!pVerifyThread->start())
			return false;
		verifyThreads.add(pVerifyThread);
	}

	//创建初始化等待同步验证现程
	syncThread = new LSyncThread(this);
	NEW_CHECK(syncThread);
	if (syncThread && !syncThread->start())
		return false;

	//创建初始化主运行线程池
	maxThreadCount = (maxConns + LTCPTaskPool::connPerThread - 1) / LTCPTaskPool::connPerThread;
	for(int i = 0; i < maxThreadCount; i++)
	{
		std::ostringstream name;
		name << "LOkayThread[" << i << "]";
		LOkayThread *pOkayThread = new LOkayThread(this, name.str());
		NEW_CHECK(pOkayThread);
		if (NULL == pOkayThread)
			return false;
		if (i < minThreadCount && !pOkayThread->start())
			return false;
		okayThreads.add(pOkayThread);
	}

	//创建初始化回收线程池
	recycleThread = new zRecycleThread(this);
	NEW_CHECK(recycleThread);
	if (recycleThread && !recycleThread->start())
		return false;

	return true;
}

/**
 * \brief 释放线程池，释放各种资源，等待各种线程退出
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

