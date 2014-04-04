/**
 * \file
 * \version  $Id: LTCPClientTaskPool.h  $
 * \author  
 * \date 
 * \brief ��װʵ���̳߳أ����ڴ�������ӷ�����
 *
 * 
 */


#ifndef _zTCPClientTaskPool_h_
#define _zTCPClientTaskPool_h_

#include <string>
#include <vector>
#include <queue>
#include <list>
#include <unistd.h>
#include <sys/timeb.h>

#include "LSocket.h"
#include "LThread.h"
#include "LTCPClientTask.h"

class LCheckconnectThread;
class LCheckwaitThread;
class LTCPClientTaskThread;

/**
 * \brief �����̳߳��࣬��װ��һ���̴߳��������ӵ��̳߳ؿ��
 *
 */
class LTCPClientTaskPool : private LCantCopy
{

	public:

		explicit LTCPClientTaskPool(const unsigned int connPerThread=512, const int us=50000) : connPerThread(connPerThread)
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

		const unsigned int connPerThread;
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

		LMutex mlock;					/**< ������� */
		LTCPClientTaskContainer tasks;	/**< �����б� */

	public:
		int usleep_time;                                        /**< ѭ���ȴ�ʱ�� */
};

#endif

