/**
 * \file
 * \version  $Id: LService.cpp  $
 * \author  
 * \date 
 * \brief 实现服务器框架类
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
 * \brief CTRL + C等信号的处理函数，结束程序
 *
 * \param signum 信号编号
 */
static void ctrlcHandler(int signum)
{
	//如果没有初始化zService实例，表示出错
	LService *instance = LService::serviceInstance();
	instance->Terminate();
}

/**
 * \brief HUP信号处理函数
 *
 * \param signum 信号编号
 */
static void hupHandler(int signum)
{
	//如果没有初始化zService实例，表示出错
	LService *instance = LService::serviceInstance();
	instance->reloadConfig();
}

LService *LService::serviceInst = NULL;

/**
 * \brief 初始化服务器程序，子类需要实现这个函数
 *
 * \return 是否成功
 */
bool LService::init()
{
	//设置信号处理
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

	//初始化随机数
	srand(time(NULL));
	
	return true;
}

/**
 * \brief 服务程序框架的主函数
 */
void LService::main()
{
	//初始化程序，并确认服务器启动成功
	if (init()
	&& validate())
	{
		//运行主回调线程
		while(!isTerminate())
		{
			if (!serviceCallback())
			{
				break;
			}
		}
	}

	//结束程序，释放相应的资源
	final();
}

