/**
 * \file
 * \version  $Id: LService.cpp  $
 * \author  
 * \date 
 * \brief ʵ�ַ����������
 *
 * 
 */


#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <assert.h>
#include <signal.h>
#include <unistd.h>

#include "LSocket.h"
#include "LTCPServer.h"
#include "LService.h"

/**
 * \brief CTRL + C���źŵĴ���������������
 *
 * \param signum �źű��
 */
static void ctrlcHandler(int signum)
{
	//���û�г�ʼ��zServiceʵ������ʾ����
	LService *instance = LService::serviceInstance();
	instance->Terminate();
}

/**
 * \brief HUP�źŴ�����
 *
 * \param signum �źű��
 */
static void hupHandler(int signum)
{
	//���û�г�ʼ��zServiceʵ������ʾ����
	LService *instance = LService::serviceInstance();
	instance->reloadConfig();
}

LService *LService::serviceInst = NULL;

/**
 * \brief ��ʼ������������������Ҫʵ���������
 *
 * \return �Ƿ�ɹ�
 */
bool LService::init()
{
	//�����źŴ���
	struct sigaction sig;

	sig.sa_handler = ctrlcHandler;
	sigemptyset(&sig.sa_mask);
	sig.sa_flags = 0;
	sigaction(SIGINT, &sig, NULL);
	sigaction(SIGQUIT, &sig, NULL);
	sigaction(SIGABRT, &sig, NULL);
	sigaction(SIGTERM, &sig, NULL);
	sig.sa_handler = hupHandler;
	sigaction(SIGHUP, &sig, NULL);
	sig.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sig, NULL);

	//��ʼ�������
	srand(time(NULL));
	
	return true;
}

/**
 * \brief ��������ܵ�������
 */
void LService::main()
{
	//��ʼ�����򣬲�ȷ�Ϸ����������ɹ�
	if (init()
	&& validate())
	{
		//�������ص��߳�
		while(!isTerminate())
		{
			if (!serviceCallback())
			{
				break;
			}
		}
	}

	//���������ͷ���Ӧ����Դ
	final();
}

