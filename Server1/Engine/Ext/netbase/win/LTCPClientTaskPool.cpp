/**
* \brief 实现线程池类,用于处理多连接服务器
*
* 
*/
#include "LTCPClientTaskPool.h"
#include "LTCPClientTask.h"
#include <assert.h>
//#include <ext/pool_allocator.h>

#include <iostream>
#include <queue>
/**
* \brief 检测TCP连接状况,如果未连接,尝试连接
*
*/
class LCheckconnectThread : public LThread
{
private:
	LTCPClientTaskPool *pool;
public:
	LCheckconnectThread(
		LTCPClientTaskPool *pool,
		const std::string &name = std::string("LCheckconnectThread"))
		: LThread(name),pool(pool)
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
//typedef std::list<LTCPClientTask *,__gnu_cxx::__pool_alloc<LTCPClientTask *> > LTCPClientTaskContainer;
typedef std::list<LTCPClientTask *> LTCPClientTaskContainer;

/**
* \brief 连接任务链表叠代器
*
*/
typedef LTCPClientTaskContainer::iterator LTCPClientTask_IT;

typedef std::vector<struct pollfd> pollfdContainer;

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
	DWORD _size;
private:
	LMutex mlock;
	//std::queue<LTCPClientTask *,std::deque<LTCPClientTask *,__gnu_cxx::__pool_alloc<LTCPClientTask *> > > _queue;
	std::queue<LTCPClientTask *> _queue;
};

/**
* \brief 处理TCP连接的验证,如果验证不通过,需要回收这个连接
*
*/
class LCheckwaitThread : public LThread,public LTCPClientTaskQueue
{

private:

	LTCPClientTaskPool *pool;
	LTCPClientTaskContainer tasks;  /**< 任务列表 */
	LTCPClientTaskContainer::size_type task_count;          /**< tasks计数(保证线程安全*/
	pollfdContainer pfds;

	/**
	* \brief 添加一个连接任务
	* \param task 连接任务
	*/
	void _add(LTCPClientTask *task)
	{
		printf("LCheckwaitThread::_add\n");

		struct pollfd pfd;
		task->fillPollFD(pfd,POLLIN | POLLPRI);
		tasks.push_back(task);
		task_count = tasks.size();
		pfds.push_back(pfd);
	}

	void remove(LTCPClientTask_IT &it,int p)
	{
		printf("LCheckwaitThread::remove\n");
		int i=0;
		pollfdContainer::iterator iter;
		for(iter = pfds.begin(),i = 0; iter != pfds.end(); iter++,i++)
		{
			if (i == p)
			{
				pfds.erase(iter);
				it = tasks.erase(it);
				task_count = tasks.size();
				break;
			}
		}
	}

public:

	/**
	* \brief 构造函数
	* \param pool 所属的连接池
	* \param name 线程名称
	*/
	LCheckwaitThread(
		LTCPClientTaskPool *pool,
		const std::string &name = std::string("LCheckwaitThread"))
		: LThread(name),pool(pool)
	{
		task_count = 0;
	}

	/**
	* \brief 析构函数
	*
	*/
	~LCheckwaitThread()
	{
	}

	virtual void run();

};

/**
* \brief 等待接受验证指令,并进行验证
*
*/
void LCheckwaitThread::run()
{
	printf("LCheckwaitThread::run\n");

	LTCPClientTask_IT it,next;
	pollfdContainer::size_type i;

	while(!isFinal())
	{
		check_queue();

		if (tasks.size() > 0)
		{
			if( WaitRecvAll( &pfds[0],pfds.size(), 0 ) <= 0 ) continue;

			for(i = 0,it = tasks.begin(); it != tasks.end();)
			{
				LTCPClientTask *task = *it;

				if ( pfds[i].revents & POLLPRI )
				{
					//套接口出现错误
					printf("套接口出现错误remove\n");
					remove(it,i--);
					task->resetState();
				}
				else if( pfds[i].revents & POLLIN )
				{
					switch(task->checkRebound())
					{
					case 1:
						//验证成功,获取下一个状态
						remove(it,i);
						if (!pool->addMain(task))
							task->resetState();
						break;
					case -1:
						//验证失败,回收任务
						printf("验证失败remove\n");
						remove(it,i);
						task->resetState();
						break;
					default:
						it ++;
						i  ++;
						//超时,下面会处理
						break;
					}
				}
				else
				{
					i ++;
					it ++;
				}
			}
		}

		LThread::msleep(50);
	}

	if(tasks.size() == 0)
		return;
	//把所有等待验证队列中的连接加入到回收队列中,回收这些连接
	for(i = 0,it = tasks.begin(); it != tasks.end();)
	{
		LTCPClientTask *task = *it;
		remove(it,i);
		task->resetState();
	}
}

/**
* \brief TCP连接的主处理线程,一般一个线程带几个TCP连接,这样可以显著提高效率
*
*/
class LTCPClientTaskThread : public LThread,public LTCPClientTaskQueue
{

private:

	LTCPClientTaskPool *pool;
	LTCPClientTaskContainer tasks;  /**< 任务列表 */
	LTCPClientTaskContainer::size_type task_count;          /**< tasks计数(保证线程安全*/

	pollfdContainer pfds;

	LMutex m_Lock;
	/**
	* \brief 添加一个连接任务
	* \param task 连接任务
	*/
	void _add(LTCPClientTask *task)
	{

		struct pollfd pfd;
		m_Lock.lock();
		task->fillPollFD(pfd,POLLIN | POLLOUT | POLLPRI);
		tasks.push_back(task);
		task_count = tasks.size();
		pfds.push_back(pfd);
		m_Lock.unlock();
	}


	void remove(LTCPClientTask_IT &it,int p)
	{
		int i;
		pollfdContainer::iterator iter;
		m_Lock.lock();
		for(iter = pfds.begin(),i = 0; iter != pfds.end(); iter++,i++)
		{
			if (i == p)
			{
				pfds.erase(iter);
				it = tasks.erase(it);
				task_count = tasks.size();
				break;
			}
		}
		m_Lock.unlock();
	}

public:

	static const LTCPClientTaskContainer::size_type connPerThread = 256;  /**< 每个线程带的连接数量 */

	/**
	* \brief 构造函数
	* \param pool 所属的连接池
	* \param name 线程名称
	*/
	LTCPClientTaskThread(
		LTCPClientTaskPool *pool,
		const std::string &name = std::string("LTCPClientTaskThread"))
		: LThread(name),pool(pool)
	{
		task_count = 0;

	}

	/**
	* \brief 析构函数
	*
	*/
	~LTCPClientTaskThread()
	{
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
* \brief 主处理线程,回调处理连接的输入输出指令
*
*/
void LTCPClientTaskThread::run()
{
	printf("LTCPClientTaskThread::run\n");

	LTCPClientTask_IT it,next;
	pollfdContainer::size_type i;

	while(!isFinal())
	{
		check_queue();
		m_Lock.lock();
		if (!tasks.empty())
		{
			for(i = 0,it = tasks.begin(); it != tasks.end();)
			{
				LTCPClientTask *task = *it;

				if (task->isTerminate())
				{
					m_Lock.unlock();
					remove(it,i);
					m_Lock.lock();
					// state_okay -> state_recycle
					task->getNextState();
				}
				else
				{
					if (task->checkFirstMainLoop())
					{
						//如果是第一次加入处理,需要预先处理缓冲中的数据
						task->ListeningRecv(false);
					}
					i++;
					it++;
				}
			}

			if (!tasks.empty())
			{
				for(i = 0,it = tasks.begin(); it != tasks.end(); it++,i++)
				{
					LTCPClientTask *task = *it;

					bool UseIocp = task->UseIocp();
					if( UseIocp )
					{
						int retcode = task->WaitRecv( false );
						if ( retcode == -1 )
						{
							//套接口出现错误
							printf("%LTCPClientTaskThread::run: 套接口异常错误");
							task->Terminate(LTCPClientTask::TM_sock_error);
						}
						else if( retcode > 0 )
						{
							//套接口准备好了读取操作
							if (!task->ListeningRecv(true))
							{
								printf("LTCPClientTaskThread::run: 套接口读操作错误");
								task->Terminate(LTCPClientTask::TM_sock_error);
							}
						}
						retcode = task->WaitSend( false );
						if( retcode == - 1 )
						{
							//套接口出现错误
							printf("%LTCPClientTaskThread::run: 套接口异常错误");
							task->Terminate(LTCPClientTask::TM_sock_error);
						}
						else if( retcode == 1 )
						{
							//套接口准备好了写入操作
							if (!task->ListeningSend())
							{
								printf("LTCPClientTaskThread::run: 套接口写操作错误");
								task->Terminate(LTCPClientTask::TM_sock_error);
							}
						}
					}
					else
					{
						if( ::poll(&pfds[i],1,0) <= 0 ) continue;
						if ( pfds[i].revents & POLLPRI )
						{
							//套接口出现错误
							printf("%LTCPClientTaskThread::run: 套接口异常错误");
							task->Terminate(LTCPClientTask::TM_sock_error);
						}
						else
						{
							if( pfds[i].revents & POLLIN)
							{
								//套接口准备好了读取操作
								if (!task->ListeningRecv(true))
								{
									printf("LTCPClientTaskThread::run: 套接口读操作错误");
									task->Terminate(LTCPClientTask::TM_sock_error);
								}
							}
							if ( pfds[i].revents & POLLOUT)
							{
								//套接口准备好了写入操作
								if (!task->ListeningSend())
								{
									printf("LTCPClientTaskThread::run: 套接口写操作错误");
									task->Terminate(LTCPClientTask::TM_sock_error);
								}
							}
						}
					}
				}
			}			
		}
		else
		{
			int iii = 0;
		}
		m_Lock.unlock();
		LThread::usleep(pool->usleep_time);
	}

	//把所有任务队列中的连接加入到回收队列中,回收这些连接


	if(tasks.size() == 0)
		return ;

	for(i = 0,it = tasks.begin(); it != tasks.end();)
	{
		LTCPClientTask *task = *it;
		remove(it,i);
		// state_okay -> state_recycle
		task->getNextState();
	}
}



/**
* \brief 析构函数
*
*/
LTCPClientTaskPool::~LTCPClientTaskPool()
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

	LTCPClientTask_IT it,next;


	if(tasks.size() > 0)
		for(it = tasks.begin(),next = it,next++; it != tasks.end(); it = next,next == tasks.end()? next : next++)
		{
			LTCPClientTask *task = *it;
			tasks.erase(it);
			SAFE_DELETE(task);
		}
}

LTCPClientTaskThread *LTCPClientTaskPool::newThread()
{
	std::ostringstream name;
	name << "LTCPClientTaskThread[" << taskThreads.size() << "]";
	LTCPClientTaskThread *taskThread = new LTCPClientTaskThread(this,name.str());
	if (NULL == taskThread)
		return NULL;
	if (!taskThread->start())
		return NULL;
	taskThreads.add(taskThread);
	return taskThread;
}

/**
* \brief 初始化线程池,预先创建各种线程
*
* \return 初始化是否成功
*/
bool LTCPClientTaskPool::init()
{
	checkconnectThread = new LCheckconnectThread(this); 
	if (NULL == checkconnectThread)
		return false;
	if (!checkconnectThread->start())
		return false;
	checkwaitThread = new LCheckwaitThread(this);
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
bool LTCPClientTaskPool::put(LTCPClientTask *task)
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
void LTCPClientTaskPool::timeAction(const LTime &ct)
{
	mlock.lock();
	for(LTCPClientTask_IT it = tasks.begin(); it != tasks.end(); ++it)
	{
		LTCPClientTask *task = *it;
		switch(task->getState())
		{
		case LTCPClientTask::close:
			if (task->checkStateTimeout(LTCPClientTask::close,ct,4)
				&& task->connect())
			{
				task->addToContainer();
				addCheckwait(task);
			}
			break;
		case LTCPClientTask::sync:
			break;
		case LTCPClientTask::okay:
			//已经在连接状态,发送网络测试信号
			task->checkConn();
			break;
		case LTCPClientTask::recycle:
			if (task->checkStateTimeout(LTCPClientTask::recycle,ct,4))
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
void LTCPClientTaskPool::addCheckwait(LTCPClientTask *task)
{
	checkwaitThread->add(task);
	task->getNextState();
}

/**
* \brief 把任务添加到主处理循环中
* \param task 待添加的任务
* \return 添加是否成功
*/
bool LTCPClientTaskPool::addMain(LTCPClientTask *task)
{
	LTCPClientTaskThread *taskThread = NULL;
	for(DWORD i = 0; i < taskThreads.size(); i++)
	{
		LTCPClientTaskThread *tmp = (LTCPClientTaskThread *)taskThreads.getByIndex(i);
		//Zebra::logger->debug("%u",tmp->size());
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
		printf("LTCPClientTaskPool::addMain: 不能得到一个空闲线程");
		return false;
	}
}

