/**
 * \file
 * \version  $Id: LCantCopy.h  $
 * \author  
 * \date 
 * \brief _LCantCopy_h_
 *
 * 
 */


#ifndef _LCantCopy_h_
#define _LCantCopy_h_

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

