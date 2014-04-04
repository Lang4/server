/**
 * \file
 * \version  $Id: LThread.h  $
 * \author  
 * \date 
 * \brief ������zThread
 *
 * 
 */

#ifndef _zThread_h_
#define _zThread_h_

#include <pthread.h>
#include <unistd.h>

#include "LCantCopy.h"
#include "LMutex.h"
#include "LCond.h"
#include "LRWLock.h"
#include <vector>
/**
 * \brief ��װ���̲߳���������ʹ���̵߳Ļ���
 *
 */
class LThread : private LCantCopy
{

	public:

		/**
		 * \brief ���캯��������һ������
		 *
		 * \param name �߳�����
		 * \param joinable ��������߳��˳���ʱ���Ƿ񱣴�״̬�����Ϊtrue��ʾ�߳��˳�����״̬�����򽫲������˳�״̬
		 */
		LThread(const std::string &name = std::string("LThread"), const bool joinable = true) 
			: threadName(name), alive(false), complete(false), thread(0), joinable(joinable) {};

		/**
		 * \brief ������������������һ�����󣬻��ն���ռ�
		 *
		 */
		virtual ~LThread() {};

		/**
		 * \brief ��ȡ��ǰ�̱߳��
		 *
		 *
		 * \return �̱߳��
		 */
		static pthread_t getCurrentThreadId()
		{
			return ::pthread_self();
		}

		/**
		 * \brief ʹ��ǰ�߳�˯��ָ����ʱ�䣬��
		 *
		 *
		 * \param sec ָ����ʱ�䣬��
		 */
		static void sleep(const long sec)
		{
			::sleep(sec);
		}

		/**
		 * \brief ʹ��ǰ�߳�˯��ָ����ʱ�䣬����
		 *
		 *
		 * \param msec ָ����ʱ�䣬����
		 */
		static void msleep(const long msec)
		{
			::usleep(1000 * msec);
		}

		/**
		 * \brief ʹ��ǰ�߳�˯��ָ����ʱ�䣬΢��
		 *
		 *
		 * \param usec ָ����ʱ�䣬΢��
		 */
		static void usleep(const long usec)
		{
			::usleep(usec);
		}

		/**
		 * \brief �߳��Ƿ���joinable��
		 *
		 *
		 * \return joinable
		 */
		const bool isJoinable() const
		{
			return joinable;
		}

		/**
		 * \brief ����߳��Ƿ�������״̬
		 *
		 * \return �߳��Ƿ�������״̬
		 */
		const bool isAlive() const
		{
			return alive;
		}

		static void *threadFunc(void *arg);
		bool start();
		void join();

		/**
		 * \brief ���������߳�
		 *
		 * ��ʵֻ�����ñ�ǣ���ô�̵߳�run���ص�ѭ���ؼ�������ǣ�����������Ѿ����ã����˳�ѭ��
		 *
		 */
		void final()
		{
			//Zebra::logger->debug("%s", __PRETTY_FUNCTION__);
			complete = true;
		}

		/**
		 * \brief �ж��߳��Ƿ����������ȥ
		 *
		 * ��Ҫ����run()����ѭ���У��ж�ѭ���Ƿ����ִ����ȥ
		 *
		 * \return �߳����ص��Ƿ����ִ��
		 */
		const bool isFinal() const 
		{
			//if (complete)
			//	Zebra::logger->debug("%s", __PRETTY_FUNCTION__);
			return complete;
		}

		/**
		 * \brief ���鹹�������߳����ص�������ÿ����Ҫʵ��������������Ҫ�����������
		 *
		 * ���������ѭ����Ҫ��ÿ��ѭ������߳��˳����isFinal()�������ܹ���֤�̰߳�ȫ�˳�
		 * <pre>
		 * 	while(!isFinal())
		 * 	{
		 * 		...
		 * 	}
		 * 	</pre>
		 *
		 */
		virtual void run() = 0;

		/**
		 * \brief �ж������߳��Ƿ���ͬһ���߳�
		 * \param other ���Ƚϵ��߳�
		 * \return �Ƿ���ͬһ���߳�
		 */
		bool operator==(const LThread& other) const
		{
			return pthread_equal(thread, other.thread) != 0;
		}

		/**
		 * \brief �ж������߳��Ƿ���ͬһ���߳�
		 * \param other ���Ƚϵ��߳�
		 * \return �Ƿ���ͬһ���߳�
		 */
		bool operator!=(const LThread& other) const
		{
			return !operator==(other);
		}

		/**
		 * \brief �����߳�����
		 *
		 * \return �߳�����
		 */
		const std::string &getThreadName() const
		{
			return threadName;
		}

	private:

		std::string threadName;			/**< �߳����� */
		LMutex mlock;					/**< ������ */
		LCond cond;						/**< �������� */
		volatile bool alive;			/**< �߳��Ƿ������� */
		volatile bool complete;			/**< �߳��Ƿ񽫽��� */
		pthread_t thread;				/**< �̱߳�� */
		bool joinable;					/**< �߳����ԣ��Ƿ�����joinable��� */

}; 

/**
 * \brief ���߳̽��з���������
 *
 */
class zThreadGroup : private LCantCopy
{

	public:

		struct Callback
		{
			virtual void exec(LThread *e)=0;
			virtual ~Callback(){};
		};

		typedef std::vector<LThread *> Container;	/**< �������� */

		zThreadGroup();
		~zThreadGroup();
		void add(LThread *thread);
		LThread *getByIndex(const Container::size_type index);
		LThread *operator[] (const Container::size_type index);
		void joinAll();
		void execAll(Callback &cb);
		
		const Container::size_type size()
		{
			zRWLock_scope_rdlock scope_rdlock(rwlock);
			return vts.size();
		}

	private:

		Container vts;								/**< �߳����� */
		LRWLock rwlock;								/**< ��д�� */

};

#endif

