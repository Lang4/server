/**
 * \file
 * \version  $Id: LThread.cpp  $
 * \author  
 * \date 
 * \brief ʵ����zThread
 *
 * 
 */


#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <mysql.h>

#include "LThread.h"

/**
 * \brief �̺߳���
 *
 * �ں��������������߳������ʵ�ֵĻص�����
 *
 * \param arg �����̵߳Ĳ���
 * \return �����߳̽�����Ϣ
 */
void *LThread::threadFunc(void *arg)
{
	LThread *thread = (LThread *)arg;

	//Zebra::logger->debug("%s", __PRETTY_FUNCTION__);
	//��ʼ�������
	//Zebra::logger->debug("%s: %u", __FUNCTION__, seedp);
	thread->mlock.lock();
	thread->alive = true;
	thread->cond.broadcast();
	thread->mlock.unlock();

	//�����߳��źŴ�����
	sigset_t sig_mask;
	sigfillset(&sig_mask);
	pthread_sigmask(SIG_SETMASK, &sig_mask, NULL);

	mysql_thread_init();

	//�����̵߳����ص�����
	thread->run();

	mysql_thread_end();

	thread->mlock.lock();
	thread->alive = false;
	thread->cond.broadcast();
	thread->mlock.unlock();

	//�������joinable����Ҫ�����߳���Դ
	if (!thread->isJoinable())
	{
		thread->mlock.lock();
		while(thread->alive)
			thread->cond.wait(thread->mlock);
		thread->mlock.unlock();
		delete (thread);
		thread = NULL;
	}

	pthread_exit(NULL);
}

/**
 * \brief �����̣߳������߳�
 *
 * \return �����߳��Ƿ�ɹ�
 */
bool LThread::start()
{
	//�߳��Ѿ��������У�ֱ�ӷ���
	if (alive)
	{
		return true;
	}

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if (!joinable) pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (0 != ::pthread_create(&thread, NULL, LThread::threadFunc, this)) 
	{
		return false;
	}

	pthread_attr_destroy(&attr);

	mlock.lock();
	while(!alive)
		cond.wait(mlock);
	mlock.unlock();

	//Zebra::logger->debug("�����߳� %s �ɹ�", getThreadName().c_str());

	return true;
}

/**
 * \brief �ȴ�һ���߳̽���
 *
 */
void LThread::join()
{
	//Zebra::logger->debug("%s", __PRETTY_FUNCTION__);

	if (0 != thread && joinable)
	{
		::pthread_join(thread, NULL);
		thread = 0;
		mlock.lock();
		while(alive)
			cond.wait(mlock);
		mlock.unlock();
	}
}

/**
 * \brief ���캯��
 *
 */
zThreadGroup::zThreadGroup() : vts(), rwlock()
{
}

/**
 * \brief ��������
 *
 */
zThreadGroup::~zThreadGroup()
{
	joinAll();
}

/**
 * \brief ���һ���̵߳�������
 * \param thread ����ӵ��߳�
 */
void zThreadGroup::add(LThread *thread)
{
	zRWLock_scope_wrlock scope_wrlock(rwlock);
	Container::iterator it = std::find(vts.begin(), vts.end(), thread);
	if (it == vts.end())
		vts.push_back(thread);
}

/**
 * \brief ����index�±��ȡ�߳�
 * \param index �±���
 * \return �߳�
 */
LThread *zThreadGroup::getByIndex(const Container::size_type index)
{
	zRWLock_scope_rdlock scope_rdlock(rwlock);
	if (index >= vts.size())
		return NULL;
	else
		return vts[index];
}

/**
 * \brief ����[]�����������index�±��ȡ�߳�
 * \param index �±���
 * \return �߳�
 */
LThread *zThreadGroup::operator[] (const Container::size_type index)
{
	zRWLock_scope_rdlock scope_rdlock(rwlock);
	if (index >= vts.size())
		return NULL;
	else
		return vts[index];
}

/**
 * \brief �ȴ������е������߳̽���
 */
void zThreadGroup::joinAll()
{
	zRWLock_scope_wrlock scope_wrlock(rwlock);
	while(!vts.empty())
	{
		LThread *pThread = vts.back();
		vts.pop_back();
		if (pThread)
		{
			pThread->final();
			pThread->join();
			delete (pThread);
			pThread = NULL;
		}
	}
}

/**
 * \brief �������е�����Ԫ�ص��ûص�����
 * \param cb �ص�����ʵ��
 */
void zThreadGroup::execAll(Callback &cb)
{
	zRWLock_scope_rdlock scope_rdlock(rwlock);
	for(Container::iterator it = vts.begin(); it != vts.end(); ++it)
	{
		cb.exec(*it);
	}
}

