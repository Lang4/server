/**
 * \file
 * \version  $Id: LTCPTask.cpp  $
 * \author  
 * \date 
 * \brief ʵ���̳߳��࣬���ڴ�������ӷ�����
 *
 * 
 */


#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <assert.h>

#include "LSocket.h"
#include "LThread.h"
#include "LTCPTask.h"
#include "LTCPTaskPool.h"

CmdAnalysis LTCPTask::analysis("Taskָ�����ͳ��",600);
/**
 * \brief ���׽ӿڷ���ָ���������־���ã�������ֱ�ӿ����������������У�ʵ�ʵķ��Ͷ���������һ���߳���
 *
 *
 * \param pstrCmd �����͵�ָ��
 * \param nCmdLen ������ָ��Ĵ�С
 * \return �����Ƿ�ɹ�
 */
bool LTCPTask::sendCmd(const void *pstrCmd, int nCmdLen)
{
	//static CmdAnalysis analysis("Taskָ���ͳ��",600);
	/*
	Cmd::t_NullCmd *ptNullCmd = (Cmd::t_NullCmd *)pstrCmd;
	analysis.add(ptNullCmd->cmd,ptNullCmd->para,nCmdLen);
	// */
	return mSocket.sendCmd(pstrCmd, nCmdLen, buffered);
}

bool LTCPTask::sendCmdPack(const void *pstrCmd, int nCmdLen)
{
	return mSocket.sendCmdNoPack(pstrCmd, nCmdLen, buffered);
}

/**
 * \brief ���׽ӿ��н������ݣ����Ҳ�����д����ڵ����������֮ǰ��֤�Ѿ����׽ӿڽ�������ѯ
 *
 * \param needRecv �Ƿ���Ҫ�������׽ӿڽ������ݣ�false����Ҫ���գ�ֻ�Ǵ�������ʣ���ָ�true��Ҫʵ�ʽ������ݣ�Ȼ��Ŵ���
 * \return �����Ƿ�ɹ���true��ʾ���ճɹ���false��ʾ����ʧ�ܣ�������Ҫ�Ͽ����� 
 */
bool LTCPTask::ListeningRecv(bool needRecv)
{
	int retcode = 0;
	if (needRecv) {
		retcode = mSocket.recvToBuf_NoPoll();
	}
	//struct timeval tv_2;
	if (-1 == retcode)
	{		
		return false;
	}
	else
	{
		do
		{
			unsigned char pstrCmd[LSocket::MAX_DATASIZE];
			int nCmdLen = mSocket.recvToCmd_NoPoll(pstrCmd, sizeof(pstrCmd));
			if (nCmdLen <= 0)
				//����ֻ�Ǵӻ���ȡ���ݰ������Բ������û������ֱ�ӷ���
				break;
			else
			{
#ifdef _NULLCMD
				Cmd::t_NullCmd *ptNullCmd = (Cmd::t_NullCmd *)pstrCmd;
				if (Cmd::CMD_NULL == ptNullCmd->cmd
						&& Cmd::PARA_NULL == ptNullCmd->para)
				{
					//���صĲ���ָ���Ҫ�ݼ�����
					//Zebra::logger->debug("������յ����ز����ź�");
					clearTick();
				}
				else
#endif
				{
					msgParse(pstrCmd, nCmdLen);
					/*
					analysis.add(ptNullCmd->cmd,ptNullCmd->para,nCmdLen);
					// */
				}
			}
		}
		while(true);
	}

	return true;
}

/**
 * \brief ���ͻ����е����ݵ��׽ӿڣ��ٵ������֮ǰ��֤�Ѿ����׽ӿڽ�������ѯ
 *
 * \return �����Ƿ�ɹ���true��ʾ���ͳɹ���false��ʾ����ʧ�ܣ�������Ҫ�Ͽ�����
 */
bool LTCPTask::ListeningSend()
{
	return mSocket.sync();
}

/**
 * \brief ��TCP�������񽻸���һ��������У��л�״̬
 *
 */
void LTCPTask::getNextState()
{
	zTCPTask_State old_state = getState();

	switch(old_state)
	{
		case notuse:
			setState(verify);
			break;
		case verify:
			setState(sync);
			break;
		case sync:
			buffered = true;
			addToContainer();
			setState(okay);
			break;
		case okay:
			removeFromContainer();
			setState(recycle);
			break;
		case recycle:
			setState(notuse);
			break;
	}

}

/**
 * \brief ��ֵ��������״̬����������
 *
 */
void LTCPTask::resetState()
{
	zTCPTask_State old_state = getState();

	switch(old_state)
	{
		case notuse:
		/*
		 * whj 
		 * ���sync�������ӵ�okay������ʧ�ܻ����okay״̬resetState�Ŀ�����
		 */
		//case okay:
		case recycle:
			//�����ܵ�
			break;
		case verify:
		case sync:
		case okay:
			//TODO ��ͬ�Ĵ���ʽ
			break;
	}

	setState(recycle);
}

void LTCPTask::checkSignal(const LRTime &ct)
{
	if (ifCheckSignal() && checkInterval(ct))
	{
		if (checkTick())
		{
			//�����ź���ָ��ʱ�䷶Χ��û�з���
			Terminate(LTCPTask::terminate_active);
		}
		else
		{
#ifdef _NULLCMD
			//���Ͳ����ź�
			Cmd::t_NullCmd tNullCmd;
			//Zebra::logger->debug("����˷��Ͳ����ź�");
			if (sendCmd(&tNullCmd, sizeof(tNullCmd)))
				setTick();
#endif
		}
	}
}

