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
* \brief �����̳߳��࣬��װ��һ���̴߳��������ӵ��̳߳ؿ��
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
	* \brief ���Ӽ���߳�
	*
	*/
	LCheckconnectThread *checkconnectThread;;
	/**
	* \brief ���ӵȴ�������Ϣ���߳�
	*
	*/
	LCheckwaitThread *checkwaitThread;;
	/**
	* \brief ���гɹ����Ӵ�������߳�
	*
	*/
	LThreadGroup taskThreads;

	/**
	* \brief ������������
	*
	*/
	//typedef std::list<LTCPClientTask *,__pool_alloc<LTCPClientTask *> > LTCPClientTaskContainer;
	typedef std::list<LTCPClientTask *> LTCPClientTaskContainer;


	/**
	* \brief �����������������
	*
	*/
	typedef LTCPClientTaskContainer::iterator zTCPClientTask_IT;

	LMutex mlock;          /**< ������� */
	LTCPClientTaskContainer tasks;  /**< �����б� */

public:
	int usleep_time;                                        /**< ѭ���ȴ�ʱ�� */
};
