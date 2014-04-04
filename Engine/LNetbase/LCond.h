/**
 * \file
 * \version  $Id: LCond.h  $
 * \author  
 * \date 
 * \brief ����zCond�࣬�򵥶�ϵͳ���������������з�װ
 *
 * 
 */


#ifndef _zCond_h_
#define _zCond_h_

#include <pthread.h>

#include "LCantCopy.h"
#include "LMutex.h"

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
			//std::cout << __PRETTY_FUNCTION__ << std::endl;
			::pthread_cond_init(&cond, NULL);
		}

		/**
		 * \brief ������������������һ����������
		 *
		 */
		~LCond()
		{
			//std::cout << __PRETTY_FUNCTION__ << std::endl;
			::pthread_cond_destroy(&cond);
		}

		/**
		 * \brief �����еȴ���������������̹߳㲥�����źţ�ʹ��Щ�߳��ܹ���������ִ��
		 *
		 */
		void broadcast()
		{
			//std::cout << __PRETTY_FUNCTION__ << std::endl;
			::pthread_cond_broadcast(&cond);
		}

		/**
		 * \brief �����еȴ���������������̷߳����źţ�ʹ��Щ�߳��ܹ���������ִ��
		 *
		 */
		void signal()
		{
			//std::cout << __PRETTY_FUNCTION__ << std::endl;
			::pthread_cond_signal(&cond);
		}

		/**
		 * \brief �ȴ��ض���������������
		 *
		 *
		 * \param mutex ��Ҫ�ȴ��Ļ�����
		 */
		void wait(LMutex &mutex)
		{
			//std::cout << __PRETTY_FUNCTION__ << std::endl;
			::pthread_cond_wait(&cond, &mutex.mutex);
		}

	private:

		pthread_cond_t cond;		/**< ϵͳ�������� */

};

#endif


