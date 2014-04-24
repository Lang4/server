#pragma once
#include "LType.h"
#include "LService.h"
#include "LTCPServer.h"
#include "LThread.h"
class LAcceptThread;
/**
* \brief �����������
*
* ʵ���������������ܴ��룬�����Ƚ�ͨ��һ��
*
*/
class LNetService : public LService
{

public:
	/**
	* \brief ����������
	*
	*/
	virtual ~LNetService() { instance = NULL; };

	/**
	* \brief ���ݵõ���TCP/IP���ӻ�ȡһ����������
	*
	* \param sock TCP/IP�׽ӿ�
	* \param addr ��ַ
	*/
	virtual void newTCPTask(const SOCKET sock,const struct sockaddr_in *addr) = 0;

	/**
	* \brief ��ȡ���ӳ��е�������
	*
	* \return ������
	*/
	virtual const int getPoolSize() const
	{
		return 0;
	}

	/**
	* \brief ��ȡ���ӳ�״̬
	*
	* \return ���ӳ�״̬
	*/
	virtual const int getPoolState() const
	{
		return 0;
	}

protected:

	/**
	* \brief ���캯��
	* 
	* �ܱ����Ĺ��캯����ʵ����Singleton���ģʽ����֤��һ��������ֻ��һ����ʵ��
	*
	* \param name ����
	*/
	LNetService(const std::string &name) : LService(name)
	{
		instance = this;

		serviceName = name;
		tcpServer = NULL;
	}

	bool init(WORD port);
	bool serviceCallback();
	void final();

private:

	static LNetService *instance;    /**< ���Ψһʵ��ָ�룬���������࣬��ʼ��Ϊ��ָ�� */
	std::string serviceName;      /**< ������������� */

	LAcceptThread* pAcceptThread; // [ranqd] ���������߳�
public:
	LTCPServer *tcpServer;        /**< TCP������ʵ��ָ�� */
};
// [ranqd] ���������߳���
class LAcceptThread : public LThread
{
public:
	LAcceptThread( LNetService* p, const std::string &name ): LThread(name)
	{
		pService = p;
	}
	~LAcceptThread()
	{
		final();
		join();
	}
	LNetService* pService;

	void run()         // [ranqd] ���������̺߳���
	{
		while(!isFinal())
		{
			//Zebra::logger->debug("���������߳̽�����");
			struct sockaddr_in addr;
			if( pService->tcpServer != NULL )
			{
				int retcode = pService->tcpServer->accept(&addr);
				if (retcode >= 0) 
				{
					//�������ӳɹ�����������
					pService->newTCPTask(retcode,&addr);
				}
			}
		}
	}
};