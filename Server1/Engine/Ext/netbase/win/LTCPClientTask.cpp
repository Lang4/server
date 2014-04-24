/**
* \brief ʵ����zTCPClientTask,TCP���ӿͻ��ˡ�
*
* 
*/
#include "LTCPClientTask.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

/**
* \brief ����һ������������TCP����
*
*
* \return �����Ƿ�ɹ�
*/
bool LTCPClientTask::connect()
{
	//Zebra::logger->debug("LTCPClientTask::connect");
	int retcode;
	int nSocket;
	struct sockaddr_in addr;

	nSocket = ::socket(PF_INET,SOCK_STREAM,0);
	if (-1 == nSocket)
	{
		printf("�����׽ӿ�ʧ��: %s",strerror(errno));
		return false;
	}

	//�����׽ӿڷ��ͽ��ջ���,���ҿͻ��˵ı�����connect֮ǰ����
	int window_size = 128 * 1024;
	retcode = ::setsockopt(nSocket,SOL_SOCKET,SO_RCVBUF,(char*)&window_size,sizeof(window_size));
	if (-1 == retcode)
	{
		::closesocket(nSocket);
		return false;
	}
	retcode = ::setsockopt(nSocket,SOL_SOCKET,SO_SNDBUF,(char*)&window_size,sizeof(window_size));
	if (-1 == retcode)
	{
		::closesocket(nSocket);
		return false;
	}

	bzero(&addr,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip.c_str());
	addr.sin_port = htons(port);

	retcode = ::connect(nSocket,(struct sockaddr *) &addr,sizeof(addr));
	if (-1 == retcode)
	{
		printf("������������ %s(%u) ������ʧ��\n",ip.c_str(),port);
		::closesocket(nSocket);
		return false;
	}

	pSocket = new LSocket(nSocket,&addr,compress);
	if (NULL == pSocket)
	{
		printf("û���㹻���ڴ�,���ܴ���zSocketʵ��\n");
		::closesocket(nSocket);
		return false;
	}

	printf("������������ %s:%u �����ӳɹ�\n",ip.c_str(),port);

	return true;
}

void LTCPClientTask::checkConn()
{
	//Zebra::logger->debug("LTCPClientTask::checkConn");
	LRTime currentTime;
	if (_ten_min(currentTime))
	{
		Cmd::t_NullCmd tNullCmd;    
		sendCmd(&tNullCmd,sizeof(tNullCmd));
	}
}

/**
* \brief ���׽ӿڷ���ָ��
* \param pstrCmd �����͵�ָ��
* \param nCmdLen ������ָ��Ĵ�С
* \return �����Ƿ�ɹ�
*/
bool LTCPClientTask::sendCmd(const void *pstrCmd,const int nCmdLen)
{
	//Zebra::logger->debug("LTCPClientTask::sendCmd");
	switch(state)
	{
	case close:
	case sync:
		if (NULL == pSocket) 
			return false;
		else
			return pSocket->sendCmd(pstrCmd,nCmdLen);
		break;
	case okay:
	case recycle:
		if (NULL == pSocket)
			return false;
		else
			return pSocket->sendCmd(pstrCmd,nCmdLen,true);
		break;
	}

	return false;
}

/**
* \brief ���׽ӿ��н�������,���Ҳ�����д���,�ڵ����������֮ǰ��֤�Ѿ����׽ӿڽ�������ѯ
*
* \param needRecv �Ƿ���Ҫ�������׽ӿڽ�������,false����Ҫ����,ֻ�Ǵ�������ʣ���ָ��,true��Ҫʵ�ʽ�������,Ȼ��Ŵ���
* \return �����Ƿ�ɹ�,true��ʾ���ճɹ�,false��ʾ����ʧ��,������Ҫ�Ͽ����� 
*/
bool LTCPClientTask::ListeningRecv(bool needRecv)
{
	//Zebra::logger->debug("LTCPClientTask::ListeningRecv");
	if( pSocket == NULL ) return false;

	int retcode = 0;
	if (needRecv) {
		retcode = pSocket->recvToBuf_NoPoll();
	}
	if (-1 == retcode)
	{
		printf("LTCPClientTask::ListeningRecv");
		return false;
	}
	else
	{
		do
		{
			BYTE pstrCmd[LSocket::MAX_DATASIZE];
			int nCmdLen = pSocket->recvToCmd_NoPoll(pstrCmd,sizeof(pstrCmd));
			if (nCmdLen <= 0)
				//����ֻ�Ǵӻ���ȡ���ݰ�,���Բ������,û������ֱ�ӷ���
				break;
			else
			{
				Cmd::t_NullCmd *pNullCmd = (Cmd::t_NullCmd *)pstrCmd;
				if (Cmd::CMD_NULL == pNullCmd->cmd
					&& Cmd::PARA_NULL == pNullCmd->para)
				{
					//Zebra::logger->debug("�ͻ����յ������ź�");
					if (!sendCmd(pstrCmd,nCmdLen))
					{
						//����ָ��ʧ��,�˳�ѭ��,�����߳�
						return false;
					}
				}
				else
					msgParse(pNullCmd,nCmdLen);
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
bool LTCPClientTask::ListeningSend()
{
	//Zebra::logger->debug("LTCPClientTask::ListeningSend");
	if (pSocket)
		return pSocket->sync();
	else
		return false;
}

/**
* \brief ��TCP�������񽻸���һ���������,�л�״̬
*
*/
void LTCPClientTask::getNextState()
{
	//Zebra::logger->debug("LTCPClientTask::getNextState");
	ConnState old_state = getState();

	lifeTime.now();
	switch(old_state)
	{
	case close:
		setState(sync);
		break;
	case sync:
		addToContainer();
		setState(okay);
		break;
	case okay:
		removeFromContainer();
		setState(recycle);
		break;
	case recycle:
		if (terminate == TM_service_close)
			recycleConn();
		setState(close);
		final();
		break;
	}

	printf("LTCPClientTask::getNextState(%s,%u,%s -> %s)",ip.c_str(),port,getStateString(old_state),getStateString(getState()));
}

/**
* \brief ��ֵ��������״̬,��������
*
*/
void LTCPClientTask::resetState()
{
	//Zebra::logger->debug("LTCPClientTask::resetState");
	ConnState old_state = getState();

	lifeTime.now();
	setState(close);
	final();

	printf("LTCPClientTask::resetState(%s,%u,%s -> %s)",ip.c_str(),port,getStateString(old_state),getStateString(getState()));
}

