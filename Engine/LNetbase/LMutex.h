/**
 * \file
 * \version  $Id: LMutex.h  $
 * \author  
 * \date 
 * \brief ������ķ�װ����Ҫ��Ϊ��ʹ�÷���
 *
 * 
 */

#ifndef _zMutex_h_
#define _zMutex_h_

#include <pthread.h>
#include <iostream>

#include "LCantCopy.h"

/**
 * \brief �����壬��װ��ϵͳ�����壬������ʹ��ϵͳ������ʱ����Ҫ�ֹ���ʼ�������ٻ��������Ĳ���
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
		LMutex(int kind = PTHREAD_MUTEX_FAST_NP) 
		{
			//std::cout << __PRETTY_FUNCTION__ << std::endl;
			pthread_mutexattr_t attr;
			::pthread_mutexattr_init(&attr);
			::pthread_mutexattr_settype(&attr, kind);
			::pthread_mutex_init(&mutex, &attr);
		}

		/**
		 * \brief ��������������һ�����������
		 *
		 */
		~LMutex()
		{
			//std::cout << __PRETTY_FUNCTION__ << std::endl;
			::pthread_mutex_destroy(&mutex);
		}

		/**
		 * \brief ����һ��������
		 *
		 */
		inline void lock()
		{
			::pthread_mutex_lock(&mutex);
		}

		/**
		 * \brief ����һ��������
		 *
		 */
		inline void unlock()
		{
			::pthread_mutex_unlock(&mutex);
		}

	private:

		pthread_mutex_t mutex;		/**< ϵͳ������ */

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

#endif

