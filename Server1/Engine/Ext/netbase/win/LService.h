#pragma once
#include "LCantCopy.h"
#include "LType.h"
#include "LTime.h"
/**
* \brief �����˷������Ŀ�ܻ���
*
* ���з���������ʵ����Ҫ�̳�����࣬���Ҳ��������ж��ٸ����࣬�������л���ֻ��һ�����ʵ��<br>
* ֻҪ������ʹ��Singleton���ģʽʵ�־Ϳ�����
*
*/
class LService : private LCantCopy
{

public:
	Timer  _one_sec_; // �붨ʱ��
	/**
	* \brief ����������
	*
	*/
	virtual ~LService() { serviceInst = NULL; };

	/**
	* \brief ���¶�ȡ�����ļ���ΪHUP�źŵĴ�����
	*
	* ȱʡʲô���鶼���ɣ�ֻ�Ǽ����һ��������Ϣ�����������������ɵ�����
	*
	*/
	virtual void reloadConfig()
	{
	}

	/**
	* \brief �ж���ѭ���Ƿ����
	*
	* �������true�����������ص�
	*
	* \return ��ѭ���Ƿ����
	*/
	bool isTerminate() const
	{
		return terminate;
	}

	/**
	* \brief ������ѭ����Ҳ���ǽ������ص�����
	*
	*/
	void Terminate()
	{
		terminate = true;
	}

	void main();

	/**
	* \brief ���ط����ʵ��ָ��
	*
	* \return �����ʵ��ָ��
	*/
	static LService *serviceInstance()
	{
		return serviceInst;
	}


protected:

	/**
	* \brief ���캯��
	*
	*/
	LService(const std::string &name) : name(name),_one_sec_(1)
	{
		serviceInst = this;

		terminate = false;
	}

	virtual bool init();

	/**
	* \brief ȷ�Ϸ�������ʼ���ɹ��������������ص�����
	*
	* \return ȷ���Ƿ�ɹ�
	*/
	virtual bool validate()
	{
		return true;
	}

	/**
	* \brief �����������ص���������Ҫ���ڼ�������˿ڣ��������false���������򣬷���true����ִ�з���
	*
	* \return �ص��Ƿ�ɹ�
	*/
	virtual bool serviceCallback() = 0;

	/**
	* \brief �������������򣬻�����Դ�����麯����������Ҫʵ���������
	*
	*/
	virtual void final() = 0;

private:

	static LService *serviceInst;    /**< ���Ψһʵ��ָ�룬���������࣬��ʼ��Ϊ��ָ�� */

	std::string name;          /**< �������� */
	bool terminate;            /**< ���������� */

};
