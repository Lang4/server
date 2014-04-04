/**
 * \file
 * \version  $Id: LCPClientTaskPool.cpp 6285 2006-04-11 06:39:28Z whj $
 * \brief ʵ���̳߳��࣬���ڴ�������ӷ�����
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
 * \brief ���TCP����״��,���δ����,��������
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
 * \brief ������������
 *
 */
#ifdef _POOL_ALLOC_
typedef std::list<LTCPClientTask *, __gnu_cxx::__pool_alloc<LTCPClientTask *> > LTCPClientTaskContainer;
#else
typedef std::list<LTCPClientTask * > LTCPClientTaskContainer;
#endif

/**
 * \brief �����������������
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
 * \brief ����TCP���ӵ���֤�������֤��ͨ������Ҫ�����������
 *
 */
class LCheckwaitThread : public LThread, public LTCPClientTaskQueue
{

	private:

		LCPClientTaskPool *pool;
		LTCPClientTaskContainer tasks;	/**< �����б� */
		LTCPClientTaskContainer::size_type task_count;          /**< tasks����(��֤�̰߳�ȫ*/
#ifdef _USE_EPOLL_
		int kdpfd;
		epollfdContainer epfds;
#else
		pollfdContainer pfds;
#endif

		/**
		 * \brief ���һ����������
		 * \param task ��������
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
		 * \brief ���캯��
		 * \param pool ���������ӳ�
		 * \param name �߳�����
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
		 * \brief ��������
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
 * \brief �ȴ�������ָ֤���������֤
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
						//�׽ӿڳ��ִ���
						remove(task);
						task->resetState();
					}
					else if (epfds[i].events & EPOLLIN)
					{
						switch(task->checkRebound())
						{
							case 1:
								//��֤�ɹ�����ȡ��һ��״̬
								remove(task);
								if (!pool->addMain(task))
									task->resetState();
								break;
							case 0:
								//��ʱ������ᴦ��
								break;
							case -1:
								//��֤ʧ�ܣ���������
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

	//�����еȴ���֤�����е����Ӽ��뵽���ն����У�������Щ����
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
						//�׽ӿڳ��ִ���
						remove(it, i--);
						task->resetState();
					}
					else if (pfds[i].revents & POLLIN)
					{
						switch(task->checkRebound())
						{
							case 1:
								//��֤�ɹ�����ȡ��һ��״̬
								remove(it, i--);
								if (!pool->addMain(task))
									task->resetState();
								break;
							case 0:
								//��ʱ������ᴦ��
								break;
							case -1:
								//��֤ʧ�ܣ���������
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

	//�����еȴ���֤�����е����Ӽ��뵽���ն����У�������Щ����
	for(i = 0, it = tasks.begin(), next = it, next++; it != tasks.end(); it = next, next++, i++)
	{
		LTCPClientTask *task = *it;
		remove(it, i--);
		task->resetState();
	}
#endif
}

/**
 * \brief TCP���ӵ��������̣߳�һ��һ���̴߳�����TCP���ӣ����������������Ч��
 *
 */
class LTCPClientTaskThread : public LThread, public LTCPClientTaskQueue
{

	private:

		LCPClientTaskPool *pool;
		LTCPClientTaskContainer tasks;	/**< �����б� */
		LTCPClientTaskContainer::size_type task_count;          /**< tasks����(��֤�̰߳�ȫ*/
#ifdef _USE_EPOLL_
		int kdpfd;
		epollfdContainer epfds;
#else
		pollfdContainer pfds;
#endif

		/**
		 * \brief ���һ����������
		 * \param task ��������
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

		static const LTCPClientTaskContainer::size_type connPerThread = 256;	/**< ÿ���̴߳����������� */

		/**
		 * \brief ���캯��
		 * \param pool ���������ӳ�
		 * \param name �߳�����
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
		 * \brief ��������
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
		 * \brief ������������ĸ���
		 * \return ����̴߳��������������
		 */
		const LTCPClientTaskContainer::size_type size() const
		{
			return task_count + _size;
		}

};

/**
 * \brief �������̣߳��ص��������ӵ��������ָ��
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
							//����ǵ�һ�μ��봦����ҪԤ�ȴ������е�����
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
						//�׽ӿڳ��ִ���
						task->Terminate(LTCPClientTask::TM_sock_error);
						check=true;
					}
					else
					{
						if (epfds_r[i].events & EPOLLIN)
						{
							//�׽ӿ�׼�����˶�ȡ����
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
							//�׽ӿڳ��ִ���
							task->Terminate(LTCPClientTask::TM_sock_error);
						}
						else
						{
							if (epfds[i].events & EPOLLIN)
							{
								//�׽ӿ�׼�����˶�ȡ����
								if (!task->ListeningRecv(true))
								{
									task->Terminate(LTCPClientTask::TM_sock_error);
								}
							}
							if (epfds[i].events & EPOLLOUT)
							{
								//�׽ӿ�׼������д�����
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

	//��������������е����Ӽ��뵽���ն����У�������Щ����
	for(it = tasks.begin(), next = it, next++; it != tasks.end(); it = next, next++)
	{
		LTCPClientTask *task = *it;
		remove(it);
		// state_okay -> state_recycle
		task->getNextState();
	}
}



/**
 * \brief ��������
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
 * \brief ��ʼ���̳߳أ�Ԥ�ȴ��������߳�
 *
 * \return ��ʼ���Ƿ�ɹ�
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
 * \brief ��һ��ָ��������ӵ�����
 * \param task ����ӵ�����
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
 * \brief ��ʱִ�е�����
 * ��Ҫ������ͻ��˶��߳�������
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
				//�Ѿ�������״̬��������������ź�
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
 * \brief ��������ӵ��ȴ�������֤���صĶ�����
 * \param task ����ӵ�����
 */
void LCPClientTaskPool::addCheckwait(LTCPClientTask *task)
{
	checkwaitThread->add(task);
	task->getNextState();
}

/**
 * \brief ��������ӵ�������ѭ����
 * \param task ����ӵ�����
 * \return ����Ƿ�ɹ�
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

