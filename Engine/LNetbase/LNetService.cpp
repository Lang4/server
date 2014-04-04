/**
 * \file
 * \version  $Id: LNetService.cpp  $
 * \author  
 * \date 
 * \brief 实现网络服务器
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
 * \brief 初始化服务器程序
 *
 * 实现<code>LService::init</code>的虚函数
 *
 * \param port 端口
 * \return 是否成功
 */
bool LNetService::init(unsigned short port)
{
	if (!LService::init())
		return false;
	
	//初始化服务器
	tcpServer = new LTCPServer(serviceName);
	NEW_CHECK(tcpServer);
	if (NULL == tcpServer)
		return false;
	if (!tcpServer->bind(serviceName, port))
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
	struct sockaddr_in addr;
	int retcode = tcpServer->accept(&addr);
	if (retcode >= 0) 
	{
		//接收连接成功，处理连接
		newTCPTask(retcode, &addr);
	}

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

