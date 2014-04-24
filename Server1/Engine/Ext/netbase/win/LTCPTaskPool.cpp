/**
* \brief ʵ���̳߳���,���ڴ�������ӷ�����
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

int LTCPTaskPool::usleep_time=50000;                    /**< ѭ���ȴ�ʱ�� */
/**
* \brief ������������
*
*/
//typedef std::list<LTCPTask *,__gnu_cxx::__pool_alloc<LTCPTask *> > LTCPTaskContainer;
typedef std::vector<LTCPTask *> LTCPTaskContainer;

/**
* \brief �����������������
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
* \brief ����TCP���ӵ���֤,�����֤��ͨ��,��Ҫ�����������
*
*/
class LVerifyThread : public LThread,public LTCPTaskQueue
{

private:

	LTCPTaskPool *pool;
	LTCPTaskContainer tasks;  /**< �����б� */
	LTCPTaskContainer::size_type task_count;      /**< tasks����(��֤�̰߳�ȫ*/
//	pollfdContainer pfds;

	LMutex m_Lock;
	/**
	* \brief ���һ����������
	* \param task ��������
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
	* \brief ���캯��
	* \param pool ���������ӳ�
	* \param name �߳�����
	*/
	LVerifyThread(
		LTCPTaskPool *pool,
		const std::string &name = std::string("LVerifyThread"))
		: LThread(name),pool(pool)
	{
		task_count = 0;
	}

	/**
	* \brief ��������
	*
	*/
	~LVerifyThread()
	{
	}

	void run();

};

/**
* \brief �ȴ�������ָ֤��,��������֤
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
		//	printf("zVerifyThreadѭ��ʱ�䣺%d ms", GetTickCount() - dwBeginTime);
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
						//����ָ��ʱ����֤��û��ͨ��,��Ҫ��������
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
							//�׽ӿڳ��ִ���
							it = tasks.erase(it);
							task_count = tasks.size();
							task->resetState();
							printf("�׽ӿڴ������\n");
							pool->addRecycle(task);
						}
						else if( ret > 0 )
						{
							switch(task->verifyConn())
							{
							case 1:
								//��֤�ɹ�
								it = tasks.erase(it);
								task_count = tasks.size();
								//����Ψһ����֤
								if (task->uniqueAdd())
								{
									//Ψһ����֤�ɹ�,��ȡ��һ��״̬
									printf("�ͻ���Ψһ����֤�ɹ�\n");
									task->setUnique();
									pool->addSync(task);
								}
								else
								{
									//Ψһ����֤ʧ��,������������
									printf("�ͻ���Ψһ����֤ʧ��\n");
									task->resetState();
									printf("Ψһ����֤ʧ�ܻ���\n");
									pool->addRecycle(task);
								}
								break;

							case -1:
								//��֤ʧ��,��������
								it = tasks.erase(it);
								task_count = tasks.size();
								task->resetState();
								printf("��֤ʧ�ܻ���\n");
								pool->addRecycle(task);
								break;	
							default:
								//��ʱ,����ᴦ��
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

	//�����еȴ���֤�����е����Ӽ��뵽���ն�����,������Щ����

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
* \brief �ȴ������߳�ͬ����֤�������,���ʧ�ܻ��߳�ʱ,����Ҫ��������
*
*/
class LSyncThread : public LThread,public LTCPTaskQueue
{

private:

	LTCPTaskPool *pool;
	LTCPTaskContainer tasks;  /**< �����б� */

	LMutex m_Lock;
	void _add(LTCPTask *task)
	{
		m_Lock.lock();
		tasks.push_back(task);
		m_Lock.unlock();
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
		: LThread(name),pool(pool)
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
	printf("LSyncThread::run\n");
	LTCPTask_IT it;
	DWORD dwBeginTime = 0;
	while(!isFinal())
	{

		//fprintf(stderr,"LVerifyThread::run\n");

		//if( dwBeginTime != 0 )
		//{
		//	printf("zSyncThreadѭ��ʱ�䣺%d ms", GetTickCount() - dwBeginTime);
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
					//�ȴ������߳�ͬ����֤ʧ��,��Ҫ��������
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
	//�����еȴ�ͬ����֤�����е����Ӽ��뵽���ն�����,������Щ����
	for(it = tasks.begin(); it != tasks.end();)
	{
		LTCPTask *task = *it;
		it = tasks.erase(it);
		task->resetState();
		pool->addRecycle(task);
	}
}

/**
* \brief TCP���ӵ��������߳�,һ��һ���̴߳�����TCP����,���������������Ч��
*
*/
class LOkayThread : public LThread,public LTCPTaskQueue
{

private:

	Timer  _one_sec_; // �붨ʱ��
	LTCPTaskPool *pool;
	LTCPTaskContainer tasks;  /**< �����б� */
	LTCPTaskContainer::size_type task_count;      /**< tasks����(��֤�̰߳�ȫ*/

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

	static const LTCPTaskContainer::size_type connPerThread = 512;  /**< ÿ���̴߳����������� */

	/**
	* \brief ���캯��
	* \param pool ���������ӳ�
	* \param name �߳�����
	*/
	LOkayThread(
		LTCPTaskPool *pool,
		const std::string &name = std::string("LOkayThread"))
		: LThread(name),pool(pool),_one_sec_(1)
	{
		task_count = 0;
	}

	/**
	* \brief ��������
	*
	*/
	~LOkayThread()
	{
	}

	void run();

	/**
	* \brief ������������ĸ���
	* \return ����̴߳��������������
	*/
	const LTCPTaskContainer::size_type size() const
	{
		return task_count + _size;
	}

};

/**
* \brief �������߳�,�ص��������ӵ��������ָ��
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
		//	printf("zOkayThreadѭ��ʱ�䣺%d ms", GetTickCount() - dwBeginTime);
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
					//�������ź�ָ��
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
						* ������״̬���������,
						* ����ᵼ��һ��taskͬʱ�������߳��е�Σ�����
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
							//�׽ӿڳ��ִ���
							printf("LOkayThread::run: �׽ӿ��쳣����");
							task->Terminate(LTCPTask::terminate_active);
						}
						else if( retcode > 0 )
						{
							//�׽ӿ�׼�����˶�ȡ����
							if (!task->ListeningRecv(true))
							{
								printf("LOkayThread::run: �׽ӿڶ���������");
								task->Terminate(LTCPTask::terminate_active);
							}
						}
						retcode = task->WaitSend( false );
						if( retcode == -1 )
						{
							//�׽ӿڳ��ִ���
							printf("LOkayThread::run: �׽ӿ��쳣����");
							task->Terminate(LTCPTask::terminate_active);
						}
						else if( retcode ==  1 )
						{
							//�׽ӿ�׼������д�����
							if (!task->ListeningSend())
							{
								printf("LOkayThread::run: �׽ӿ�д�������� port = %u",task->getPort());

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

	//��������������е����Ӽ��뵽���ն�����,������Щ����

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
		* ������״̬���������,
		* ����ᵼ��һ��taskͬʱ�������߳��е�Σ�����
		*/
		task->getNextState();
		pool->addRecycle(task);
	}
}

/**
* \brief ���ӻ����߳�,�����������õ�TCP����,�ͷ���Ӧ����Դ
*
*/
DWORD dwStep[100];
class LRecycleThread : public LThread,public LTCPTaskQueue
{

private:

	LTCPTaskPool *pool;
	LTCPTaskContainer tasks;  /**< �����б� */

	LMutex m_Lock;

	void _add(LTCPTask *task)
	{
		m_Lock.lock();
		tasks.push_back(task);
		m_Lock.unlock();
	}

public:

	/**
	* \brief ���캯��
	* \param pool ���������ӳ�
	* \param name �߳�����
	*/
	LRecycleThread(
		LTCPTaskPool *pool,
		const std::string &name = std::string("LRecycleThread"))
		: LThread(name),pool(pool)
	{}

	/**
	* \brief ��������
	*
	*/
	~LRecycleThread() {};

	void run();

};

/**
* \brief ���ӻ��մ����߳�,��ɾ���ڴ�ռ�֮ǰ��Ҫ��֤recycleConn����1
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
		//	printf("zRecycleThreadѭ��ʱ�䣺%d ms", GetTickCount() - dwBeginTime);
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
					//���մ�����ɿ����ͷ���Ӧ����Դ
					it = tasks.erase(it);
					if (task->isUnique())
						//����Ѿ�ͨ����Ψһ����֤,��ȫ��Ψһ������ɾ��
						task->uniqueRemove();
					task->getNextState();
					//				if( !task->UseIocp() ) // [ranqd] ʹ��Iocp�����Ӳ����������
//					g_RecycleLog[task] = 0;
					SAFE_DELETE(task);
					break;
				default:
					//���ճ�ʱ,�´��ٴ���
					it++;
					break;
				}
			}
		}
		m_Lock.unlock();

		LThread::msleep(200);
	}

	//�������е�����

	fprintf(stderr,"LRecycleThread::final\n");
	for(it = tasks.begin(); it != tasks.end();)
	{
		//���մ�����ɿ����ͷ���Ӧ����Դ
		LTCPTask *task = *it;
		it = tasks.erase(it);
		if (task->isUnique())
			//����Ѿ�ͨ����Ψһ����֤,��ȫ��Ψһ������ɾ��
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
* \brief ��һ��TCP������ӵ���֤������,��Ϊ���ڶ����֤����,��Ҫ����һ�����㷨��ӵ���ͬ����֤���������
*
* \param task һ����������
*/
bool LTCPTaskPool::addVerify(LTCPTask *task)
{

	printf("LTCPTaskPool::addVerify\n");
	//��Ϊ���ڶ����֤����,��Ҫ����һ�����㷨��ӵ���ͬ����֤���������
	static DWORD hashcode = 0;
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
	printf("LTCPTaskPool::addSync\n");
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
	printf("LTCPTaskPool::addOkay\n");
	//���ȱ������е��߳�,�ҳ����еĲ������������ٵ��߳�,���ҳ�û���������߳�
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
		* ������״̬���������,
		* ����ᵼ��һ��taskͬʱ�������߳��е�Σ�����
		*/
		task->getNextState();
		//����߳�ͬʱ�������������û�е�������
		pmin->add(task);
		return true;
	}
	if (nostart)
	{
		//�̻߳�û������,��Ҫ�����߳�,�ٰ���ӵ�����̵߳Ĵ��������
		if (nostart->start())
		{
			printf("zTCPTaskPool���������߳�\n");
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
		else
			printf("zTCPTaskPool���ܴ��������߳�");
	}

	printf("zTCPTaskPoolû���ҵ����ʵ��߳�����������");
	//û���ҵ��߳��������������,��Ҫ���չر�����
	return false;
}

/**
* \brief ��һ��TCP������ӵ����մ��������
*
* \param task һ����������
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
* \brief ��ʼ���̳߳�,Ԥ�ȴ��������߳�
*
* \return ��ʼ���Ƿ�ɹ�
*/
bool LTCPTaskPool::init()
{
	printf("LTCPTaskPool::init\n");
	//������ʼ����֤�߳�
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

	//������ʼ���ȴ�ͬ����֤�߳�
	syncThread = new LSyncThread(this);
	if (syncThread && !syncThread->start())
		return false;

	//������ʼ���������̳߳�
	maxThreadCount = (maxConns + LOkayThread::connPerThread - 1) / LOkayThread::connPerThread;
	printf("���TCP������%d,ÿ�߳�TCP������%d,�̸߳���%d\n",maxConns,LOkayThread::connPerThread,maxThreadCount);
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

	//������ʼ�������̳߳�
	recycleThread = new LRecycleThread(this);
	if (recycleThread && !recycleThread->start())
		return false;

	return true;
}

/**
* \brief �ͷ��̳߳�,�ͷŸ�����Դ,�ȴ������߳��˳�
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

