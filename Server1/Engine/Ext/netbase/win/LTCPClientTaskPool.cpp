/**
* \brief ʵ���̳߳���,���ڴ�������ӷ�����
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
* \brief ���TCP����״��,���δ����,��������
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
* \brief ������������
*
*/
//typedef std::list<LTCPClientTask *,__gnu_cxx::__pool_alloc<LTCPClientTask *> > LTCPClientTaskContainer;
typedef std::list<LTCPClientTask *> LTCPClientTaskContainer;

/**
* \brief �����������������
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
* \brief ����TCP���ӵ���֤,�����֤��ͨ��,��Ҫ�����������
*
*/
class LCheckwaitThread : public LThread,public LTCPClientTaskQueue
{

private:

	LTCPClientTaskPool *pool;
	LTCPClientTaskContainer tasks;  /**< �����б� */
	LTCPClientTaskContainer::size_type task_count;          /**< tasks����(��֤�̰߳�ȫ*/
	pollfdContainer pfds;

	/**
	* \brief ���һ����������
	* \param task ��������
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
	* \brief ���캯��
	* \param pool ���������ӳ�
	* \param name �߳�����
	*/
	LCheckwaitThread(
		LTCPClientTaskPool *pool,
		const std::string &name = std::string("LCheckwaitThread"))
		: LThread(name),pool(pool)
	{
		task_count = 0;
	}

	/**
	* \brief ��������
	*
	*/
	~LCheckwaitThread()
	{
	}

	virtual void run();

};

/**
* \brief �ȴ�������ָ֤��,��������֤
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
					//�׽ӿڳ��ִ���
					printf("�׽ӿڳ��ִ���remove\n");
					remove(it,i--);
					task->resetState();
				}
				else if( pfds[i].revents & POLLIN )
				{
					switch(task->checkRebound())
					{
					case 1:
						//��֤�ɹ�,��ȡ��һ��״̬
						remove(it,i);
						if (!pool->addMain(task))
							task->resetState();
						break;
					case -1:
						//��֤ʧ��,��������
						printf("��֤ʧ��remove\n");
						remove(it,i);
						task->resetState();
						break;
					default:
						it ++;
						i  ++;
						//��ʱ,����ᴦ��
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
	//�����еȴ���֤�����е����Ӽ��뵽���ն�����,������Щ����
	for(i = 0,it = tasks.begin(); it != tasks.end();)
	{
		LTCPClientTask *task = *it;
		remove(it,i);
		task->resetState();
	}
}

/**
* \brief TCP���ӵ��������߳�,һ��һ���̴߳�����TCP����,���������������Ч��
*
*/
class LTCPClientTaskThread : public LThread,public LTCPClientTaskQueue
{

private:

	LTCPClientTaskPool *pool;
	LTCPClientTaskContainer tasks;  /**< �����б� */
	LTCPClientTaskContainer::size_type task_count;          /**< tasks����(��֤�̰߳�ȫ*/

	pollfdContainer pfds;

	LMutex m_Lock;
	/**
	* \brief ���һ����������
	* \param task ��������
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

	static const LTCPClientTaskContainer::size_type connPerThread = 256;  /**< ÿ���̴߳����������� */

	/**
	* \brief ���캯��
	* \param pool ���������ӳ�
	* \param name �߳�����
	*/
	LTCPClientTaskThread(
		LTCPClientTaskPool *pool,
		const std::string &name = std::string("LTCPClientTaskThread"))
		: LThread(name),pool(pool)
	{
		task_count = 0;

	}

	/**
	* \brief ��������
	*
	*/
	~LTCPClientTaskThread()
	{
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
* \brief �������߳�,�ص��������ӵ��������ָ��
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
						//����ǵ�һ�μ��봦��,��ҪԤ�ȴ������е�����
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
							//�׽ӿڳ��ִ���
							printf("%LTCPClientTaskThread::run: �׽ӿ��쳣����");
							task->Terminate(LTCPClientTask::TM_sock_error);
						}
						else if( retcode > 0 )
						{
							//�׽ӿ�׼�����˶�ȡ����
							if (!task->ListeningRecv(true))
							{
								printf("LTCPClientTaskThread::run: �׽ӿڶ���������");
								task->Terminate(LTCPClientTask::TM_sock_error);
							}
						}
						retcode = task->WaitSend( false );
						if( retcode == - 1 )
						{
							//�׽ӿڳ��ִ���
							printf("%LTCPClientTaskThread::run: �׽ӿ��쳣����");
							task->Terminate(LTCPClientTask::TM_sock_error);
						}
						else if( retcode == 1 )
						{
							//�׽ӿ�׼������д�����
							if (!task->ListeningSend())
							{
								printf("LTCPClientTaskThread::run: �׽ӿ�д��������");
								task->Terminate(LTCPClientTask::TM_sock_error);
							}
						}
					}
					else
					{
						if( ::poll(&pfds[i],1,0) <= 0 ) continue;
						if ( pfds[i].revents & POLLPRI )
						{
							//�׽ӿڳ��ִ���
							printf("%LTCPClientTaskThread::run: �׽ӿ��쳣����");
							task->Terminate(LTCPClientTask::TM_sock_error);
						}
						else
						{
							if( pfds[i].revents & POLLIN)
							{
								//�׽ӿ�׼�����˶�ȡ����
								if (!task->ListeningRecv(true))
								{
									printf("LTCPClientTaskThread::run: �׽ӿڶ���������");
									task->Terminate(LTCPClientTask::TM_sock_error);
								}
							}
							if ( pfds[i].revents & POLLOUT)
							{
								//�׽ӿ�׼������д�����
								if (!task->ListeningSend())
								{
									printf("LTCPClientTaskThread::run: �׽ӿ�д��������");
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

	//��������������е����Ӽ��뵽���ն�����,������Щ����


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
* \brief ��������
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
* \brief ��ʼ���̳߳�,Ԥ�ȴ��������߳�
*
* \return ��ʼ���Ƿ�ɹ�
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
* \brief ��һ��ָ��������ӵ�����
* \param task ����ӵ�����
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
* \brief ��ʱִ�е�����
* ��Ҫ������ͻ��˶��߳�������
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
			//�Ѿ�������״̬,������������ź�
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
* \brief ��������ӵ��ȴ�������֤���صĶ�����
* \param task ����ӵ�����
*/
void LTCPClientTaskPool::addCheckwait(LTCPClientTask *task)
{
	checkwaitThread->add(task);
	task->getNextState();
}

/**
* \brief ��������ӵ�������ѭ����
* \param task ����ӵ�����
* \return ����Ƿ�ɹ�
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
		printf("LTCPClientTaskPool::addMain: ���ܵõ�һ�������߳�");
		return false;
	}
}

