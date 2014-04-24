/**
* \brief ʵ���̳߳���,���ڴ�������ӷ�����
*
* 
*/
#include "LTCPTask.h"

#include <assert.h>

#include <iostream>

CmdAnalysis LTCPTask::analysis("Taskָ�����ͳ��",600);
/**
* \brief ���׽ӿڷ���ָ��,��������־����,������ֱ�ӿ�����������������,ʵ�ʵķ��Ͷ���������һ���߳���
*
*
* \param pstrCmd �����͵�ָ��
* \param nCmdLen ������ָ��Ĵ�С
* \return �����Ƿ�ɹ�
*/
bool LTCPTask::sendCmd(const void *pstrCmd,int nCmdLen)
{
	//Zebra::logger->debug("LTCPTask::sendCmd");  
	//static CmdAnalysis analysis("Taskָ���ͳ��",600);
	/*
	Cmd::t_NullCmd *pNullCmd = (Cmd::t_NullCmd *)pstrCmd;
	analysis.add(pNullCmd->cmd,pNullCmd->para,nCmdLen);
	// */
	return mSocket->sendCmd(pstrCmd,nCmdLen,buffered);
}

bool LTCPTask::sendCmdNoPack(const void *pstrCmd,int nCmdLen)
{
	//Zebra::logger->debug("LTCPTask::sendCmdNoPack");
	return mSocket->sendCmdNoPack(pstrCmd,nCmdLen,buffered);
}

/**
* \brief ���׽ӿ��н�������,���Ҳ�����д���,�ڵ����������֮ǰ��֤�Ѿ����׽ӿڽ�������ѯ
*
* \param needRecv �Ƿ���Ҫ�������׽ӿڽ�������,false����Ҫ����,ֻ�Ǵ�������ʣ���ָ��,true��Ҫʵ�ʽ�������,Ȼ��Ŵ���
* \return �����Ƿ�ɹ�,true��ʾ���ճɹ�,false��ʾ����ʧ��,������Ҫ�Ͽ����� 
*/
bool LTCPTask::ListeningRecv(bool needRecv)
{
	//Zebra::logger->debug("LTCPTask::ListeningRecv");




	int retcode = 0;
	if (needRecv) {
		retcode = mSocket->recvToBuf_NoPoll();
	}
	//struct timeval tv_2;
	if (-1 == retcode)
	{
		printf("LTCPTask::ListeningRecv -1");  
		printf("LTCPTask::remoteport= %u localport = %u",mSocket->getPort(),mSocket->getLocalPort());
		return false;
	}
	else
	{
		do
		{
			BYTE pstrCmd[LSocket::MAX_DATASIZE];
			memset(pstrCmd,0,LSocket::MAX_DATASIZE);
			int nCmdLen = mSocket->recvToCmd_NoPoll(pstrCmd,sizeof(pstrCmd));
			if (nCmdLen <= 0)
				//����ֻ�Ǵӻ���ȡ���ݰ�,���Բ������,û������ֱ�ӷ���
				break;
			else
			{
				Cmd::t_NullCmd *pNullCmd = (Cmd::t_NullCmd *)pstrCmd;
				if (Cmd::CMD_NULL == pNullCmd->cmd
					&& Cmd::PARA_NULL == pNullCmd->para)
				{
					//���صĲ���ָ��,��Ҫ�ݼ�����
					//Zebra::logger->debug("������յ����ز����ź�");
					clearTick();
				}
				else
				{
					msgParse(pNullCmd,nCmdLen);
					/*
					analysis.add(pNullCmd->cmd,pNullCmd->para,nCmdLen);
					// */
				}
			}
		}
		while(true);
	}

	return true;
}

/**
* \brief ���ͻ����е����ݵ��׽ӿ�,�ٵ������֮ǰ��֤�Ѿ����׽ӿڽ�������ѯ
*
* \return �����Ƿ�ɹ�,true��ʾ���ͳɹ�,false��ʾ����ʧ��,������Ҫ�Ͽ�����
*/
bool LTCPTask::ListeningSend()
{
	//Zebra::logger->debug("LTCPTask::ListeningSend");
	return mSocket->sync();
}

/**
* \brief ��TCP�������񽻸���һ���������,�л�״̬
*
*/
void LTCPTask::getNextState()
{
	//Zebra::logger->debug("LTCPTask::getNextState()");
	LTCPTask_State old_state = getState();

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

	printf("LTCPTask::getNextState(%s:%u),%s -> %s)",getIP(),getPort(),getStateString(old_state),getStateString(getState()));
}

/**
* \brief ��ֵ��������״̬,��������
*
*/
void LTCPTask::resetState()
{
	//Zebra::logger->debug("LTCPTask::resetState");
	LTCPTask_State old_state = getState();

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
		printf("LTCPTask::resetState:������ recycle -> recycle");
		break;
	case verify:
	case sync:
	case okay:
		//TODO ��ͬ�Ĵ���ʽ
		break;
	}

	setState(recycle);
	printf("LTCPTask::resetState(%s:%u),%s -> %s)",getIP(),getPort(),getStateString(old_state),getStateString(getState()));
}

void LTCPTask::checkSignal(const LRTime &ct)
{
	///Zebra::logger->debug("LTCPTask::checkSignal");
	if (ifCheckSignal() && checkInterval(ct))
	{
		if (checkTick())
		{
			//�����ź���ָ��ʱ�䷶Χ��û�з���
			printf("�׽ӿڼ������ź�ʧ��");
			Terminate(LTCPTask::terminate_active);
		}
		else
		{
			//���Ͳ����ź�
			Cmd::t_NullCmd tNullCmd;
			//Zebra::logger->debug("����˷��Ͳ����ź�");
			if (sendCmd(&tNullCmd,sizeof(tNullCmd)))
				setTick();
		}
	}
}

