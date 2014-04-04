/**
 * \file
 * \version  $Id: LNetService.cpp  $
 * \author  
 * \date 
 * \brief ʵ�����������
 *
 * 
 */

#include <iostream>
#include <string>
#include <ext/numeric>

#include "LService.h"
#include "LThread.h"
#include "LSocket.h"
#include "LTCPServer.h"
#include "LTCPTaskPool.h"
#include "LNetService.h"

LNetService *LNetService::instance = NULL;

/**
 * \brief ��ʼ������������
 *
 * ʵ��<code>LService::init</code>���麯��
 *
 * \param port �˿�
 * \return �Ƿ�ɹ�
 */
bool LNetService::init(unsigned short port)
{
	if (!LService::init())
		return false;
	
	//��ʼ��������
	tcpServer = new LTCPServer(serviceName);
	NEW_CHECK(tcpServer);
	if (NULL == tcpServer)
		return false;
	if (!tcpServer->bind(serviceName, port))
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
	struct sockaddr_in addr;
	int retcode = tcpServer->accept(&addr);
	if (retcode >= 0) 
	{
		//�������ӳɹ�����������
		newTCPTask(retcode, &addr);
	}

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

