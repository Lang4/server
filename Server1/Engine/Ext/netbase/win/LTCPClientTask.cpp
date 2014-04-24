/**
* \brief 实现类zTCPClientTask,TCP连接客户端。
*
* 
*/
#include "LTCPClientTask.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

/**
* \brief 建立一个到服务器的TCP连接
*
*
* \return 连接是否成功
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
		printf("创建套接口失败: %s",strerror(errno));
		return false;
	}

	//设置套接口发送接收缓冲,并且客户端的必须在connect之前设置
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
		printf("创建到服务器 %s(%u) 的连接失败\n",ip.c_str(),port);
		::closesocket(nSocket);
		return false;
	}

	pSocket = new LSocket(nSocket,&addr,compress);
	if (NULL == pSocket)
	{
		printf("没有足够的内存,不能创建zSocket实例\n");
		::closesocket(nSocket);
		return false;
	}

	printf("创建到服务器 %s:%u 的连接成功\n",ip.c_str(),port);

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
* \brief 向套接口发送指令
* \param pstrCmd 待发送的指令
* \param nCmdLen 待发送指令的大小
* \return 发送是否成功
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
* \brief 从套接口中接受数据,并且拆包进行处理,在调用这个函数之前保证已经对套接口进行了轮询
*
* \param needRecv 是否需要真正从套接口接受数据,false则不需要接收,只是处理缓冲中剩余的指令,true需要实际接收数据,然后才处理
* \return 接收是否成功,true表示接收成功,false表示接收失败,可能需要断开连接 
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
				//这里只是从缓冲取数据包,所以不会出错,没有数据直接返回
				break;
			else
			{
				Cmd::t_NullCmd *pNullCmd = (Cmd::t_NullCmd *)pstrCmd;
				if (Cmd::CMD_NULL == pNullCmd->cmd
					&& Cmd::PARA_NULL == pNullCmd->para)
				{
					//Zebra::logger->debug("客户端收到测试信号");
					if (!sendCmd(pstrCmd,nCmdLen))
					{
						//发送指令失败,退出循环,结束线程
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
* \brief 发送缓冲中的数据到套接口,再调用这个之前保证已经对套接口进行了轮询
*
* \return 发送是否成功,true表示发送成功,false表示发送失败,可能需要断开连接
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
* \brief 把TCP连接任务交给下一个任务队列,切换状态
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
* \brief 重值连接任务状态,回收连接
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

