/**
 * \file
 * \version  $Id: LCPClientTaskPool.cpp 6285 2006-04-11 06:39:28Z whj $
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
#include "LCPClientTaskPool.h"
#include "LTime.h"

/**
 * \brief 检测TCP连接状况,如果未连接,尝试连接
 *
 */
class LCheckconnectThread : public LThread
{
	private:
		LCPClientTaskPool *pool;
	public:
		LCheckconnectThread(
				LCPClientTaskPool *pool,
				const std::string &name = std::string("LCheckconnectThread"))
			: LThread(name), pool(pool)
			{
			}
		virtual void run()
		{
			while(!isFinal())
			{
				LThread::sleep(4);
				LTime ct;
				pool->timeAction(ct);
			}
		}
};

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

#ifdef _USE_EPOLL_
#ifdef _POOL_ALLOC_
typedef std::vector<struct epoll_event, __gnu_cxx::__pool_alloc<epoll_event> > epollfdContainer;
#else
typedef std::vector<struct epoll_event> epollfdContainer;
#endif
#else
typedef std::vector<struct pollfd> pollfdContainer;
#endif

class LTCPClientTaskQueue
{
	public:
		LTCPClientTaskQueue() :_size(0) {}
		virtual ~LTCPClientTaskQueue() {}
		inline void add(LTCPClientTask *task)
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
				LTCPClientTask *task = _queue.front();
				_queue.pop();
				_add(task);
			}
			_size = 0;
			mlock.unlock();
		}
	protected:
		virtual void _add(LTCPClientTask *task) = 0;
		unsigned int _size;
	private:
		LMutex mlock;
#ifdef _POOL_ALLOC_
		std::queue<LTCPClientTask *, std::deque<LTCPClientTask *, __gnu_cxx::__pool_alloc<LTCPClientTask *> > > _queue;
#else
		std::queue<LTCPClientTask *, std::deque<LTCPClientTask *> > _queue;
#endif
};

/**
 * \brief 处理TCP连接的验证，如果验证不通过，需要回收这个连接
 *
 */
class LCheckwaitThread : public LThread, public LTCPClientTaskQueue
{

	private:

		LCPClientTaskPool *pool;
		LTCPClientTaskContainer tasks;	/**< 任务列表 */
		LTCPClientTaskContainer::size_type task_count;          /**< tasks计数(保证线程安全*/
#ifdef _USE_EPOLL_
		int kdpfd;
		epollfdContainer epfds;
#else
		pollfdContainer pfds;
#endif

		/**
		 * \brief 添加一个连接任务
		 * \param task 连接任务
		 */
		void _add(LTCPClientTask *task)
		{
#ifdef _USE_EPOLL_
			task->addEpoll(kdpfd, EPOLLIN | EPOLLERR | EPOLLPRI, (void *)task);
			tasks.push_back(task);
			task_count = tasks.size();
			if (task_count > epfds.size())
			{
				epfds.resize(task_count + 16);
			}
#else
			struct pollfd pfd;
			task->fillPollFD(pfd, POLLIN | POLLERR | POLLPRI);
			tasks.push_back(task);
			task_count = tasks.size();
			pfds.push_back(pfd);
#endif
		}

#ifdef _USE_EPOLL_
		void remove(LTCPClientTask *task)
		{
			task->delEpoll(kdpfd, EPOLLIN | EPOLLERR | EPOLLPRI);
			tasks.remove(task);
			task_count = tasks.size();
		}
		void remove(LTCPClientTask_IT &it)
		{
			(*it)->delEpoll(kdpfd, EPOLLIN | EPOLLERR | EPOLLPRI);
			tasks.erase(it);
			task_count = tasks.size();
		}
#else
		void remove(LTCPClientTask_IT &it, int p)
		{
			int i;
			pollfdContainer::iterator iter;
			for(iter = pfds.begin(), i = 0; iter != pfds.end(); iter++, i++)
			{
				if (i == p)
				{
					pfds.erase(iter);
					tasks.erase(it);
					task_count = tasks.size();
					break;
				}
			}
		}
#endif

	public:

		/**
		 * \brief 构造函数
		 * \param pool 所属的连接池
		 * \param name 线程名称
		 */
		LCheckwaitThread(
				LCPClientTaskPool *pool,
				const std::string &name = std::string("LCheckwaitThread"))
			: LThread(name), pool(pool)
			{
				task_count = 0;
#ifdef _USE_EPOLL_
				kdpfd = epoll_create(256);
				assert(-1 != kdpfd);
				epfds.resize(256);
#endif
			}

		/**
		 * \brief 析构函数
		 *
		 */
		~LCheckwaitThread()
		{
#ifdef _USE_EPOLL_
			TEMP_FAILURE_RETRY(::close(kdpfd));
#endif
		}

		virtual void run();

};

/**
 * \brief 等待接受验证指令，并进行验证
 *
 */
void LCheckwaitThread::run()
{
#ifdef _USE_EPOLL_
	LTCPClientTask_IT it, next;

	while(!isFinal())
	{
		check_queue();
		if (!tasks.empty())
		{
			int retcode = epoll_wait(kdpfd, &epfds[0], task_count, 0);
			if (retcode > 0)
			{
				for(int i = 0; i < retcode; i++)
				{
					LTCPClientTask *task = (LTCPClientTask *)epfds[i].data.ptr;
					if (epfds[i].events & (EPOLLERR | EPOLLPRI))
					{
						//套接口出现错误
						remove(task);
						task->resetState();
					}
					else if (epfds[i].events & EPOLLIN)
					{
						switch(task->checkRebound())
						{
							case 1:
								//验证成功，获取下一个状态
								remove(task);
								if (!pool->addMain(task))
									task->resetState();
								break;
							case 0:
								//超时，下面会处理
								break;
							case -1:
								//验证失败，回收任务
								remove(task);
								task->resetState();
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
		LTCPClientTask *task = *it;
		remove(it);
		task->resetState();
	}
#else
	LTCPClientTask_IT it, next;
	pollfdContainer::size_type i;

	while(!isFinal())
	{
		check_queue();
		if (!pfds.empty())
		{
			for(i = 0; i < pfds.size(); i++);
			{
				pfds[i].revents = 0;
			}

			if (TEMP_FAILURE_RETRY(::poll(&pfds[0], pfds.size(), 0)) > 0)
			{
				for(i = 0, it = tasks.begin(), next = it, next++; it != tasks.end(); it = next, next++, i++)
				{
					LTCPClientTask *task = *it;
					if (pfds[i].revents & (POLLERR | POLLPRI))
					{
						//套接口出现错误
						remove(it, i--);
						task->resetState();
					}
					else if (pfds[i].revents & POLLIN)
					{
						switch(task->checkRebound())
						{
							case 1:
								//验证成功，获取下一个状态
								remove(it, i--);
								if (!pool->addMain(task))
									task->resetState();
								break;
							case 0:
								//超时，下面会处理
								break;
							case -1:
								//验证失败，回收任务
								remove(it, i--);
								task->resetState();
								break;
						}
					}
				}
			}
		}

		LThread::msleep(50);
	}

	//把所有等待验证队列中的连接加入到回收队列中，回收这些连接
	for(i = 0, it = tasks.begin(), next = it, next++; it != tasks.end(); it = next, next++, i++)
	{
		LTCPClientTask *task = *it;
		remove(it, i--);
		task->resetState();
	}
#endif
}

/**
 * \brief TCP连接的主处理线程，一般一个线程带几个TCP连接，这样可以显著提高效率
 *
 */
class LTCPClientTaskThread : public LThread, public LTCPClientTaskQueue
{

	private:

		LCPClientTaskPool *pool;
		LTCPClientTaskContainer tasks;	/**< 任务列表 */
		LTCPClientTaskContainer::size_type task_count;          /**< tasks计数(保证线程安全*/
#ifdef _USE_EPOLL_
		int kdpfd;
		epollfdContainer epfds;
#else
		pollfdContainer pfds;
#endif

		/**
		 * \brief 添加一个连接任务
		 * \param task 连接任务
		 */
		void _add(LTCPClientTask *task)
		{
#ifdef _USE_EPOLL_
			task->addEpoll(kdpfd, EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLPRI, (void *)task);
			tasks.push_back(task);
			task_count = tasks.size();
			if (task_count > epfds.size())
			{
				epfds.resize(task_count + 16);
			}
#else
			struct pollfd pfd;
			task->fillPollFD(pfd, POLLIN | POLLOUT | POLLERR | POLLPRI);
			tasks.push_back(task);
			task_count = tasks.size();
			pfds.push_back(pfd);
#endif
		}

#ifdef _USE_EPOLL_
		void remove(LTCPClientTask_IT &it)
		{
			(*it)->delEpoll(kdpfd, EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLPRI);
			tasks.erase(it);
			task_count = tasks.size();
		}
#else
		void remove(LTCPClientTask_IT &it, int p)
		{
			int i;
			pollfdContainer::iterator iter;
			for(iter = pfds.begin(), i = 0; iter != pfds.end(); iter++, i++)
			{
				if (i == p)
				{
					pfds.erase(iter);
					tasks.erase(it);
					task_count = tasks.size();
					break;
				}
			}
		}
#endif

	public:

		static const LTCPClientTaskContainer::size_type connPerThread = 256;	/**< 每个线程带的连接数量 */

		/**
		 * \brief 构造函数
		 * \param pool 所属的连接池
		 * \param name 线程名称
		 */
		LTCPClientTaskThread(
				LCPClientTaskPool *pool,
				const std::string &name = std::string("LTCPClientTaskThread"))
			: LThread(name), pool(pool)
			{
				task_count = 0;
#ifdef _USE_EPOLL_
				kdpfd = epoll_create(connPerThread);
				assert(-1 != kdpfd);
				epfds.resize(connPerThread);
#endif
			}

		/**
		 * \brief 析构函数
		 *
		 */
		~LTCPClientTaskThread()
		{
#ifdef _USE_EPOLL_
			TEMP_FAILURE_RETRY(::close(kdpfd));
#endif
		}

		virtual void run();

		/**
		 * \brief 返回连接任务的个数
		 * \return 这个线程处理的连接任务数
		 */
		const LTCPClientTaskContainer::size_type size() const
		{
			return task_count + _size;
		}

};

/**
 * \brief 主处理线程，回调处理连接的输入输出指令
 *
 */
void LTCPClientTaskThread::run()
{
	LTCPClientTask_IT it, next;

	LRTime currentTime;

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
					LTCPClientTask *task = *it;

					if (task->isTerminate())
					{
						if (task->isFdsrAdd())
						{
							task->delEpoll(kdpfd_r, EPOLLIN | EPOLLERR | EPOLLPRI);
							fds_count_r --;
							task->fdsrAdd(false); 
						}
						remove(it);
						// state_okay -> state_recycle
						task->getNextState();
					}
					else
					{
						if (task->checkFirstMainLoop())
						{
							//如果是第一次加入处理，需要预先处理缓冲中的数据
							task->ListeningRecv(false);
						}
						if(!task->isFdsrAdd())
						{
							task->addEpoll(kdpfd_r, EPOLLIN | EPOLLERR | EPOLLPRI, (void *)task);
							task->fdsrAdd(true); 
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
					LTCPClientTask *task = (LTCPClientTask *)epfds_r[i].data.ptr;
					if (epfds_r[i].events & (EPOLLERR | EPOLLPRI))
					{
						//套接口出现错误
						task->Terminate(LTCPClientTask::TM_sock_error);
						check=true;
					}
					else
					{
						if (epfds_r[i].events & EPOLLIN)
						{
							//套接口准备好了读取操作
							if (!task->ListeningRecv(true))
							{
								task->Terminate(LTCPClientTask::TM_sock_error);
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
						LTCPClientTask *task = (LTCPClientTask *)epfds[i].data.ptr;
						if (epfds[i].events & (EPOLLERR | EPOLLPRI))
						{
							//套接口出现错误
							task->Terminate(LTCPClientTask::TM_sock_error);
						}
						else
						{
							if (epfds[i].events & EPOLLIN)
							{
								//套接口准备好了读取操作
								if (!task->ListeningRecv(true))
								{
									task->Terminate(LTCPClientTask::TM_sock_error);
								}
							}
							if (epfds[i].events & EPOLLOUT)
							{
								//套接口准备好了写入操作
								if (!task->ListeningSend())
								{
									task->Terminate(LTCPClientTask::TM_sock_error);
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
		LTCPClientTask *task = *it;
		remove(it);
		// state_okay -> state_recycle
		task->getNextState();
	}
}



/**
 * \brief 析构函数
 *
 */
LCPClientTaskPool::~LCPClientTaskPool()
{
	if (checkconnectThread)
	{
		checkconnectThread->final();
		checkconnectThread->join();
		SAFE_DELETE(checkconnectThread);
	}
	if (checkwaitThread)
	{
		checkwaitThread->final();
		checkwaitThread->join();
		SAFE_DELETE(checkwaitThread);
	}

	taskThreads.joinAll();

	LTCPClientTask_IT it, next;
	for(it = tasks.begin(), next = it, next++; it != tasks.end(); it = next, next++)
	{
		LTCPClientTask *task = *it;
		tasks.erase(it);
		SAFE_DELETE(task);
	}
}

LTCPClientTaskThread *LCPClientTaskPool::newThread()
{
	std::ostringstream name;
	name << "LTCPClientTaskThread[" << taskThreads.size() << "]";
	LTCPClientTaskThread *taskThread = new LTCPClientTaskThread(this, name.str());
	NEW_CHECK(taskThread);
	if (NULL == taskThread)
		return NULL;
	if (!taskThread->start())
		return NULL;
	taskThreads.add(taskThread);
	return taskThread;
}

/**
 * \brief 初始化线程池，预先创建各种线程
 *
 * \return 初始化是否成功
 */
bool LCPClientTaskPool::init()
{
	checkconnectThread = new LCheckconnectThread(this); 
	NEW_CHECK(checkconnectThread);
	if (NULL == checkconnectThread)
		return false;
	if (!checkconnectThread->start())
		return false;
	checkwaitThread = new LCheckwaitThread(this);
	NEW_CHECK(checkwaitThread);
	if (NULL == checkwaitThread)
		return false;
	if (!checkwaitThread->start())
		return false;

	if (NULL == newThread())
		return false;

	return true;
}

/**
 * \brief 把一个指定任务添加到池中
 * \param task 待添加的任务
 */
bool LCPClientTaskPool::put(LTCPClientTask *task)
{
	if (task)
	{
		mlock.lock();
		tasks.push_front(task);
		mlock.unlock();
		return true;
	}
	else
		return false;
}

/**
 * \brief 定时执行的任务
 * 主要是如果客户端断线尝试重连
 */
void LCPClientTaskPool::timeAction(const LTime &ct)
{
	mlock.lock();
	for(LTCPClientTask_IT it = tasks.begin(); it != tasks.end(); ++it)
	{
		LTCPClientTask *task = *it;
		switch(task->getState())
		{
			case LTCPClientTask::close:
				if (task->checkStateTimeout(LTCPClientTask::close, ct, 4)
						&& task->connect())
				{
					addCheckwait(task);
				}
				break;
			case LTCPClientTask::sync:
				break;
			case LTCPClientTask::okay:
				//已经在连接状态，发送网络测试信号
				task->checkConn();
				break;
			case LTCPClientTask::recycle:
				if (task->checkStateTimeout(LTCPClientTask::recycle, ct, 4))
					task->getNextState();
				break;
		}
	}
	mlock.unlock();
}

/**
 * \brief 把任务添加到等待连接认证返回的队列中
 * \param task 待添加的任务
 */
void LCPClientTaskPool::addCheckwait(LTCPClientTask *task)
{
	checkwaitThread->add(task);
	task->getNextState();
}

/**
 * \brief 把任务添加到主处理循环中
 * \param task 待添加的任务
 * \return 添加是否成功
 */
bool LCPClientTaskPool::addMain(LTCPClientTask *task)
{
	LTCPClientTaskThread *taskThread = NULL;
	for(unsigned int i = 0; i < taskThreads.size(); i++)
	{
		LTCPClientTaskThread *tmp = (LTCPClientTaskThread *)taskThreads.getByIndex(i);
		//Zebra::logger->debug("%u", tmp->size());
		if (tmp && tmp->size() < connPerThread)
		{
			taskThread = tmp;
			break;
		}
	}
	if (NULL == taskThread)
		taskThread = newThread();
	if (taskThread)
	{
		taskThread->add(task);
		task->getNextState();
		return true;
	}
	else
	{
		return false;
	}
}

