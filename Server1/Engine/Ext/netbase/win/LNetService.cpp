/**
* \brief ʵ�����������
*
* 
*/
#include "LNetService.h"
#include "LIocp.h"
#include <iostream>


LNetService *LNetService::instance = NULL;

/**
* \brief ��ʼ������������
*
* ʵ��<code>LService::init</code>���麯��
*
* \param port �˿�
* \return �Ƿ�ɹ�
*/
bool LNetService::init(WORD port)
{
	if (!LService::init())
		return false;

	//��ʼ��������
	tcpServer = new LTCPServer(serviceName);
	if (NULL == tcpServer)
		return false;
	if (!tcpServer->bind(serviceName,port))
		return false;

	// [ranqd] ��ʼ�������߳�
	pAcceptThread = new LAcceptThread( this, serviceName );
	if( pAcceptThread == NULL )
		return false;
	if(!pAcceptThread->start())
		return false;

	return true;
}

/**
* \brief ��������������ص�����
*
* ʵ���麯��<code>LService::serviceCallback</code>����Ҫ���ڼ�������˿ڣ��������false���������򣬷���true����ִ�з���
*
* \return �ص��Ƿ�ɹ�
*/
bool LNetService::serviceCallback()
{
	// [ranqd] ÿ�����һ�������������
	LRTime currentTime;
	currentTime.now();
	if( _one_sec_( currentTime ) )
	{
		LIocp::getInstance().UpdateNetLog();
	}
	Sleep(10);
	return true;
}
/**
* \brief �����������������
*
* ʵ�ִ��麯��<code>LService::final</code>��������Դ
*
*/
void LNetService::final()
{
	SAFE_DELETE(tcpServer);
}

