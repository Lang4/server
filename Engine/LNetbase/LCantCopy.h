/**
 * \file
 * \version  $Id: LCantCopy.h  $
 * \author  
 * \date 
 * \brief ����zNoncopyable�࣬ʹ���е�������ÿ������캯���͸�ֵ����
 *
 * 
 */


#ifndef _zNoncopyable_h_
#define _zNoncopyable_h_

/**
 * \brief ʹ���е�������ÿ������캯���͸�ֵ����
 *
 */
class LCantCopy
{

	protected:

		/**
		 * \brief ȱʡ���캯��
		 *
		 */
		LCantCopy() {};

		/**
		 * \brief ȱʡ��������
		 *
		 */
		~LCantCopy() {};

	private:

		/**
		 * \brief �������캯����û��ʵ�֣����õ���
		 *
		 */
		LCantCopy(const LCantCopy&);

		/**
		 * \brief ��ֵ�������ţ�û��ʵ�֣����õ���
		 *
		 */
		const LCantCopy & operator= (const LCantCopy &);

};

#endif

