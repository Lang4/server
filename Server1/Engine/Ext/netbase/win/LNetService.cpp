/**
* \brief 实现网络服务器
*
* 
*/
#include "LNetService.h"
#include "LIocp.h"
#include <iostream>


LNetService *LNetService::instance = NULL;

/**
* \brief 初始化服务器程序
*
* 实现<code>LService::init</code>的虚函数
*
* \param port 端口
* \return 是否成功
*/
bool LNetService::init(WORD port)
{
	if (!LService::init())
		return false;

	//初始化服务器
	tcpServer = new LTCPServer(serviceName);
	if (NULL == tcpServer)
		return false;
	if (!tcpServer->bind(serviceName,port))
		return false;

	// [ranqd] 初始化监听线程
	pAcceptThread = new LAcceptThread( this, serviceName );
	if( pAcceptThread == NULL )
		return false;
	if(!pAcceptThread->start())
		return false;

	return true;
}

/**
* \brief 网络服务程序的主回调函数
*
* 实现虚函数<code>LService::serviceCallback</code>，主要用于监听服务端口，如果返回false将结束程序，返回true继续执行服务
*
* \return 回调是否成功
*/
bool LNetService::serviceCallback()
{
	// [ranqd] 每秒更新一次网络流量监测
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
* \brief 结束网络服务器程序
*
* 实现纯虚函数<code>LService::final</code>，回收资源
*
*/
void LNetService::final()
{
	SAFE_DELETE(tcpServer);
}

