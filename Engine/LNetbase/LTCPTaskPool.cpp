/**
 * \file
 * \version  $Id: LTCPTaskPool.cpp  $
 * \author  
 * \date 
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
#include "LTCPTaskPool.h"
#include "LTime.h"

int LTCPTaskPool::usleep_time=50000;										/**< ѭ���ȴ�ʱ�� */
uint16_t LTCPTaskPool::connPerThread=512;
/**
 * \brief ������������
 *
 */
#ifdef _POOL_ALLOC_
typedef std::list<LTCPTask *, __gnu_cxx::__pool_alloc<LTCPTask *> > zTCPTaskContainer;
#else
typedef std::list<LTCPTask * > zTCPTaskContainer;
#endif

/**
 * \brief �����������������
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
 * \brief ����TCP���ӵ���֤�������֤��ͨ������Ҫ�����������
 *
 */
class LVerifyThread : public LThread, public LTCPTaskQueue
{

	private:

		LTCPTaskPool *pool;
		zTCPTaskContainer tasks;	/**< �����б� */
		zTCPTaskContainer::size_type task_count;			/**< tasks����(��֤�̰߳�ȫ*/
		int kdpfd;
		epollfdContainer epfds;

		/**
		 * \brief ���һ����������
		 * \param task ��������
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
		 * \brief ���캯��
		 * \param pool ���������ӳ�
		 * \param name �߳�����
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
		 * \brief ��������
		 *
		 */
		~LVerifyThread()
		{
			TEMP_FAILURE_RETRY(::close(kdpfd));
		}

		void run();

};

/**
 * \brief �ȴ�������ָ֤���������֤
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
					//����ָ��ʱ����֤��û��ͨ������Ҫ��������
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
						//�׽ӿڳ��ִ���
						remove(task);
						task->resetState();
						pool->addRecycle(task);
					}
					else if (epfds[i].events & EPOLLIN)
					{
						switch(task->verifyConn())
						{
							case 1:
								//��֤�ɹ�
								remove(task);
								//����Ψһ����֤
								if (task->uniqueAdd())
								{
									//Ψһ����֤�ɹ�����ȡ��һ��״̬
									task->setUnique();
									pool->addSync(task);
								}
								else
								{
									//Ψһ����֤ʧ�ܣ�������������
									task->resetState();
									pool->addRecycle(task);
								}
								break;
							case 0:
								//��ʱ������ᴦ��
								break;
							case -1:
								//��֤ʧ�ܣ���������
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

	//�����еȴ���֤�����е����Ӽ��뵽���ն����У�������Щ����
	for(it = tasks.begin(), next = it, next++; it != tasks.end(); it = next, next++)
	{
		LTCPTask *task = *it;
		remove(it);
		task->resetState();
		pool->addRecycle(task);
	}
}

/**
 * \brief �ȴ������߳�ͬ����֤������ӣ����ʧ�ܻ��߳�ʱ������Ҫ��������
 *
 */
class LSyncThread : public LThread, public LTCPTaskQueue
{

	private:

		LTCPTaskPool *pool;
		zTCPTaskContainer tasks;	/**< �����б� */

		void _add(LTCPTask *task)
		{
			tasks.push_front(task);
		}

	public:

		/**
		 * \brief ���캯��
		 * \param pool ���������ӳ�
		 * \param name �߳�����
		 */
		LSyncThread(
				LTCPTaskPool *pool,
				const std::string &name = std::string("LSyncThread"))
			: LThread(name), pool(pool)
			{}

		/**
		 * \brief ��������
		 *
		 */
		~LSyncThread() {};

		void run();

};

/**
 * \brief �ȴ������߳�ͬ����֤�������
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
						//�ȴ������߳�ͬ����֤�ɹ�
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
						//�ȴ������߳�ͬ����֤ʧ�ܣ���Ҫ��������
						it = tasks.erase(it);
						task->resetState();
						pool->addRecycle(task);
						break;
				}
			}
		}

		LThread::msleep(200);
	}

	//�����еȴ�ͬ����֤�����е����Ӽ��뵽���ն����У�������Щ����
	for(it = tasks.begin(); it != tasks.end();)
	{
		LTCPTask *task = *it;
		it = tasks.erase(it);
		task->resetState();
		pool->addRecycle(task);
	}
}

/**
 * \brief TCP���ӵ��������̣߳�һ��һ���̴߳�����TCP���ӣ����������������Ч��
 *
 */
class LOkayThread : public LThread, public LTCPTaskQueue
{

	private:

		LTCPTaskPool *pool;
		zTCPTaskContainer tasks;	/**< �����б� */
		zTCPTaskContainer::size_type task_count;			/**< tasks����(��֤�̰߳�ȫ*/
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
		 * \brief ���캯��
		 * \param pool ���������ӳ�
		 * \param name �߳�����
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
		 * \brief ��������
		 *
		 */
		~LOkayThread()
		{
			TEMP_FAILURE_RETRY(::close(kdpfd));
		}

		void run();

		/**
		 * \brief ������������ĸ���
		 * \return ����̴߳��������������
		 */
		const zTCPTaskContainer::size_type size() const
		{
			return task_count + _size;
		}

};

/**
 * \brief �������̣߳��ص��������ӵ��������ָ��
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

					//�������ź�ָ��
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
						 * ������״̬���������,
						 * ����ᵼ��һ��taskͬʱ�������߳��е�Σ�����
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
						//�׽ӿڳ��ִ���
						task->Terminate(LTCPTask::terminate_active);
						check=true;
					}
					else
					{
						if (epfds_r[i].events & EPOLLIN)
						{
							//�׽ӿ�׼�����˶�ȡ����
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
							//�׽ӿڳ��ִ���
							task->Terminate(LTCPTask::terminate_active);
						}
						else
						{
							if (epfds[i].events & EPOLLIN)
							{
								//�׽ӿ�׼�����˶�ȡ����
								if (!task->ListeningRecv(true))
								{
									task->Terminate(LTCPTask::terminate_active);
								}
							}
							if (epfds[i].events & EPOLLOUT)
							{
								//�׽ӿ�׼������д�����
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

	//��������������е����Ӽ��뵽���ն����У�������Щ����
	for(it = tasks.begin(), next = it, next++; it != tasks.end(); it = next, next++)
	{
		LTCPTask *task = *it;
		remove(it);
		// state_sync -> state_okay
		/*
		 * whj
		 * ������״̬���������,
		 * ����ᵼ��һ��taskͬʱ�������߳��е�Σ�����
		 */
		task->getNextState();
		pool->addRecycle(task);
	}
	TEMP_FAILURE_RETRY(::close(kdpfd_r));
}

/**
 * \brief ���ӻ����̣߳������������õ�TCP���ӣ��ͷ���Ӧ����Դ
 *
 */
class zRecycleThread : public LThread, public LTCPTaskQueue
{

	private:

		LTCPTaskPool *pool;
		zTCPTaskContainer tasks;	/**< �����б� */

		void _add(LTCPTask *task)
		{
			tasks.push_front(task);
		}

	public:

		/**
		 * \brief ���캯��
		 * \param pool ���������ӳ�
		 * \param name �߳�����
		 */
		zRecycleThread(
				LTCPTaskPool *pool,
				const std::string &name = std::string("zRecycleThread"))
			: LThread(name), pool(pool)
			{}

		/**
		 * \brief ��������
		 *
		 */
		~zRecycleThread() {};

		void run();

};

/**
 * \brief ���ӻ��մ����̣߳���ɾ���ڴ�ռ�֮ǰ��Ҫ��֤recycleConn����1
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
						//���մ�����ɿ����ͷ���Ӧ����Դ
						it = tasks.erase(it);
						if (task->isUnique())
							//����Ѿ�ͨ����Ψһ����֤����ȫ��Ψһ������ɾ��
							task->uniqueRemove();
						task->getNextState();
						SAFE_DELETE(task);
						break;
					case 0:
						//���ճ�ʱ���´��ٴ���
						it++;
						break;
				}
			}
		}

		LThread::msleep(200);
	}

	//�������е�����
	for(it = tasks.begin(); it != tasks.end();)
	{
		//���մ�����ɿ����ͷ���Ӧ����Դ
		LTCPTask *task = *it;
		it = tasks.erase(it);
		if (task->isUnique())
			//����Ѿ�ͨ����Ψһ����֤����ȫ��Ψһ������ɾ��
			task->uniqueRemove();
		task->getNextState();
		SAFE_DELETE(task);
	}
}


/**
 * \brief �������ӳ��������Ӹ���
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
 * \brief ��һ��TCP������ӵ���֤�����У���Ϊ���ڶ����֤���У���Ҫ����һ�����㷨��ӵ���ͬ����֤���������
 *
 * \param task һ����������
 */
bool LTCPTaskPool::addVerify(LTCPTask *task)
{

	//��Ϊ���ڶ����֤���У���Ҫ����һ�����㷨��ӵ���ͬ����֤���������
	static unsigned int hashcode = 0;
	LVerifyThread *pVerifyThread = (LVerifyThread *)verifyThreads.getByIndex(hashcode++ % maxVerifyThreads);
	if (pVerifyThread)
	{
		// state_sync -> state_okay
		/*
		 * whj
		 * ������״̬���������,
		 * ����ᵼ��һ��taskͬʱ�������߳��е�Σ�����
		 */
		task->getNextState();
		pVerifyThread->add(task);
	}
	return true;
}

/**
 * \brief ��һ��ͨ����֤��TCP������ӵ��ȴ�ͬ����֤������
 *
 * \param task һ����������
 */
void LTCPTaskPool::addSync(LTCPTask *task)
{
	// state_sync -> state_okay
	/*
	 * whj
	 * ������״̬���������,
	 * ����ᵼ��һ��taskͬʱ�������߳��е�Σ�����
	 */
	task->getNextState();
	syncThread->add(task);
}

/**
 * \brief ��һ��ͨ����֤��TCP���������
 *
 * \param task һ����������
 * \return ����Ƿ�ɹ�
 */
bool LTCPTaskPool::addOkay(LTCPTask *task)
{
	//���ȱ������е��̣߳��ҳ����еĲ������������ٵ��̣߳����ҳ�û���������߳�
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
		 * ������״̬���������,
		 * ����ᵼ��һ��taskͬʱ�������߳��е�Σ�����
		 */
		task->getNextState();
		//����߳�ͬʱ�������������û�е�������
		min->add(task);
		return true;
	}
	if (nostart)
	{
		//�̻߳�û�����У���Ҫ�����̣߳��ٰ���ӵ�����̵߳Ĵ��������
		if (nostart->start())
		{
			// state_sync -> state_okay
			/*
			 * whj
			 * ������״̬���������,
			 * ����ᵼ��һ��taskͬʱ�������߳��е�Σ�����
			 */
			task->getNextState();
			//����߳�ͬʱ�������������û�е�������
			nostart->add(task);
			return true;
		}
		else{}
	}

	//û���ҵ��߳�������������ӣ���Ҫ���չر�����
	return false;
}

/**
 * \brief ��һ��TCP������ӵ����մ��������
 *
 * \param task һ����������
 */
void LTCPTaskPool::addRecycle(LTCPTask *task)
{
	recycleThread->add(task);
}


/**
 * \brief ��ʼ���̳߳أ�Ԥ�ȴ��������߳�
 *
 * \return ��ʼ���Ƿ�ɹ�
 */
bool LTCPTaskPool::init()
{
	//������ʼ����֤�߳�
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

	//������ʼ���ȴ�ͬ����֤�ֳ�
	syncThread = new LSyncThread(this);
	NEW_CHECK(syncThread);
	if (syncThread && !syncThread->start())
		return false;

	//������ʼ���������̳߳�
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

	//������ʼ�������̳߳�
	recycleThread = new zRecycleThread(this);
	NEW_CHECK(recycleThread);
	if (recycleThread && !recycleThread->start())
		return false;

	return true;
}

/**
 * \brief �ͷ��̳߳أ��ͷŸ�����Դ���ȴ������߳��˳�
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

