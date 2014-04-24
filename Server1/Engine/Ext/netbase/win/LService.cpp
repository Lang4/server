/**
 * \brief ʵ�ַ����������
 *
 * 
 */
#include "LService.h"

#include <assert.h>
#include <signal.h>

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

/**
 * \brief CTRL + C���źŵĴ�����,��������
 *
 * \param signum �źű��
 */
static void ctrlcHandler(int signum)
{
  fprintf(stderr,"ctrlcHandler\n");
  //���û�г�ʼ��zServiceʵ��,��ʾ����
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
  //���û�г�ʼ��zServiceʵ��,��ʾ����
  LService *instance = LService::serviceInstance();
  instance->reloadConfig();
}

LService *LService::serviceInst = NULL;

/**
 * \brief ��ʼ������������,������Ҫʵ���������
 *
 * \return �Ƿ�ɹ�
 */
bool LService::init()
{
  //��ʼ�������
  srand(time(NULL));
  
  return true;
}

/**
 * \brief ��������ܵ�������
 */
void LService::main()
{
  //��ʼ������,��ȷ�Ϸ����������ɹ�
  if(signal(SIGTERM  , ctrlcHandler)==SIG_ERR)
  {
	fprintf(stderr,"�ź�����ʧ��\n");
  }
  

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

  //��������,�ͷ���Ӧ����Դ
  final();
}

