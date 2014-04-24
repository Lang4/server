#pragma once
#include "LCantCopy.h"
#include "LType.h"
#include <string>
#include <list>
#include <map>
#include <vector>
#include <time.h>
#include <sstream>
#include <errno.h>
#include "windows.h"
/**
* \brief ������zThread
*
* 
*/
/**
* \brief �ٽ�������װ��ϵͳ�ٽ�����������ʹ��ϵͳ�ٽ���ʱ����Ҫ�ֹ���ʼ���������ٽ�������Ĳ���
*
*/
class LMutex : private LCantCopy
{

	friend class LCond;

public:
	/**
	* \brief ���캯��������һ�����������
	*
	*/
	LMutex() 
	{
		m_hMutex = CreateMutex(NULL,FALSE,NULL);
	}

	/**
	* \brief ��������������һ�����������
	*
	*/
	~LMutex()
	{
		CloseHandle(m_hMutex);
	}

	/**
	* \brief ����һ��������
	*
	*/
	inline void lock()
	{
		if( WaitForSingleObject(m_hMutex,10000) == WAIT_TIMEOUT )
		{
			char szName[MAX_PATH];
			GetModuleFileName(NULL,szName,sizeof(szName));
			::MessageBox(NULL,"����������", szName, MB_ICONERROR);
		}
	}

	/**
	* \brief ����һ��������
	*
	*/
	inline void unlock()
	{
		ReleaseMutex(m_hMutex);
	}

private:

	HANDLE m_hMutex;    /**< ϵͳ������ */

};

/**
* \brief Wrapper
* �����ڸ��Ӻ���������ʹ��
*/
class LMutexScopeLock : private LCantCopy
{

public:

	/**
	* \brief ���캯��
	* ��������lock����
	* \param m ��������
	*/
	LMutexScopeLock(LMutex &m) : mlock(m)
	{
		mlock.lock();
	}

	/**
	* \brief ��������
	* ��������unlock����
	*/
	~LMutexScopeLock()
	{
		mlock.unlock();
	}

private:

	/**
	* \brief ��������
	*/
	LMutex &mlock;

};

/**
* \brief ��װ��ϵͳ����������ʹ����Ҫ�򵥣�ʡȥ���ֹ���ʼ��������ϵͳ���������Ĺ�������Щ�����������ɹ��캯���������������Զ����
*
*/
class LCond : private LCantCopy
{

public:

	/**
	* \brief ���캯�������ڴ���һ����������
	*
	*/
	LCond()
	{
		m_hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	}

	/**
	* \brief ������������������һ����������
	*
	*/
	~LCond()
	{
		CloseHandle(m_hEvent);
	}

	/**
	* \brief �����еȴ���������������̹߳㲥�����źţ�ʹ��Щ�߳��ܹ���������ִ��
	*
	*/
	void broadcast()
	{
		SetEvent(m_hEvent);
	}

	/**
	* \brief �����еȴ���������������̷߳����źţ�ʹ��Щ�߳��ܹ���������ִ��
	*
	*/
	void signal()
	{
		SetEvent(m_hEvent);
	}

	/**
	* \brief �ȴ��ض���������������
	*
	*
	* \param m_hMutex ��Ҫ�ȴ��Ļ�����
	*/
	void wait(LMutex &mutex)
	{
		WaitForSingleObject(m_hEvent,INFINITE);
	}

private:

	HANDLE m_hEvent;    /**< ϵͳ�������� */

};

/**
* \brief ��װ��ϵͳ��д����ʹ����Ҫ�򵥣�ʡȥ���ֹ���ʼ��������ϵͳ��д���Ĺ�������Щ�����������ɹ��캯���������������Զ����
*
*/
class LRWLock : private LCantCopy
{

public:
	/**
	* \brief ���캯�������ڴ���һ����д��
	*
	*/
	LRWLock()
	{
		m_hMutex = CreateMutex(NULL,FALSE,NULL);
	}

	/**
	* \brief ������������������һ����д��
	*
	*/
	~LRWLock()
	{
		CloseHandle(m_hMutex);
	}

	/**
	* \brief �Զ�д�����ж���������
	*
	*/
	inline void rdlock()
	{
		WaitForSingleObject(m_hMutex,INFINITE);
	};

	/**
	* \brief �Զ�д������д��������
	*
	*/
	inline void wrlock()
	{
		WaitForSingleObject(m_hMutex,INFINITE);
	}

	/**
	* \brief �Զ�д�����н�������
	*
	*/
	inline void unlock()
	{
		ReleaseMutex(m_hMutex);
	}

private:

	HANDLE m_hMutex;    /**< ϵͳ��д�� */

};

/**
* \brief rdlock Wrapper
* �����ڸ��Ӻ����ж�д����ʹ��
*/
class LRWLockScopeRdlock : private LCantCopy
{

public:

	/**
	* \brief ���캯��
	* ��������rdlock����
	* \param m ��������
	*/
	LRWLockScopeRdlock(LRWLock &m) : rwlock(m)
	{
		rwlock.rdlock();
	}

	/**
	* \brief ��������
	* ��������unlock����
	*/
	~LRWLockScopeRdlock()
	{
		rwlock.unlock();
	}

private:

	/**
	* \brief ��������
	*/
	LRWLock &rwlock;

};

/**
* \brief wrlock Wrapper
* �����ڸ��Ӻ����ж�д����ʹ��
*/
class LRWLockScopeWrlock : private LCantCopy
{

public:

	/**
	* \brief ���캯��
	* ��������wrlock����
	* \param m ��������
	*/
	LRWLockScopeWrlock(LRWLock &m) : rwlock(m)
	{
		rwlock.wrlock();
	}

	/**
	* \brief ��������
	* ��������unlock����
	*/
	~LRWLockScopeWrlock()
	{
		rwlock.unlock();
	}

private:

	/**
	* \brief ��������
	*/
	LRWLock &rwlock;

};

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
	LThread(const std::string &name = std::string("LThread"),const bool joinable = true) 
		: threadName(name),alive(false),complete(false),joinable(joinable) { m_hThread = NULL; };

	/**
	* \brief ������������������һ�����󣬻��ն���ռ�
	*
	*/
	virtual ~LThread()
	{
		if (NULL != m_hThread)
		{
			CloseHandle(m_hThread);
		}
	};

	/**
	* \brief ʹ��ǰ�߳�˯��ָ����ʱ�䣬��
	*
	*
	* \param sec ָ����ʱ�䣬��
	*/
	static void sleep(const long sec)
	{
		::Sleep(1000 * sec);
	}

	/**
	* \brief ʹ��ǰ�߳�˯��ָ����ʱ�䣬����
	*
	*
	* \param msec ָ����ʱ�䣬����
	*/
	static void msleep(const long msec)
	{
		::Sleep(msec);
	}

	/**
	* \brief ʹ��ǰ�߳�˯��ָ����ʱ�䣬΢��
	*
	*
	* \param usec ָ����ʱ�䣬΢��
	*/
	static void usleep(const long usec)
	{
		::Sleep(usec / 1000);
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

	static DWORD WINAPI threadFunc(void *arg);
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
		return complete;
	}

	/**
	* \brief ���鹹�������߳����ص�������ÿ����Ҫʵ��������������Ҫ�����������
	*
	* ���������ѭ����Ҫ��ÿ��ѭ������߳��˳����isFinal()�������ܹ���֤�̰߳�ȫ�˳�
	* <pre>
	*   while(!isFinal())
	*   {
	*     ...
	*   }
	*   </pre>
	*
	*/
	virtual void run() = 0;


	/**
	* \brief �����߳�����
	*
	* \return �߳�����
	*/
	const std::string &getThreadName() const
	{
		return threadName;
	}

public:

	std::string threadName;      /**< �߳����� */
	LMutex mlock;          /**< ������ */
	volatile bool alive;      /**< �߳��Ƿ������� */
	volatile bool complete;
	HANDLE m_hThread;        /**< �̱߳�� */
	bool joinable;          /**< �߳����ԣ��Ƿ�����joinable��� */

}; 

/**
* \brief ���߳̽��з���������
*
*/
class LThreadGroup : private LCantCopy
{

public:

	struct Callback
	{
		virtual void exec(LThread *e)=0;
		virtual ~Callback(){};
	};

	typedef std::vector<LThread *> Container;  /**< �������� */

	LThreadGroup();
	~LThreadGroup();
	void add(LThread *thread);
	LThread *getByIndex(const Container::size_type index);
	LThread *operator[] (const Container::size_type index);
	void joinAll();
	void execAll(Callback &cb);

	const Container::size_type size()
	{
		LRWLockScopeRdlock scope_rdlock(rwlock);
		return vts.size();
	}

private:

	Container vts;                /**< �߳����� */
	LRWLock rwlock;                /**< ��д�� */

};
