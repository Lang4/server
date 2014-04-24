#pragma once
#include "LType.h"
#include "LService.h"
#include "LTCPServer.h"
#include "LThread.h"
class LAcceptThread;
/**
* \brief 网络服务器类
*
* 实现了网络服务器框架代码，这个类比较通用一点
*
*/
class LNetService : public LService
{

public:
	/**
	* \brief 虚析构函数
	*
	*/
	virtual ~LNetService() { instance = NULL; };

	/**
	* \brief 根据得到的TCP/IP连接获取一个连接任务
	*
	* \param sock TCP/IP套接口
	* \param addr 地址
	*/
	virtual void newTCPTask(const SOCKET sock,const struct sockaddr_in *addr) = 0;

	/**
	* \brief 获取连接池中的连接数
	*
	* \return 连接数
	*/
	virtual const int getPoolSize() const
	{
		return 0;
	}

	/**
	* \brief 获取连接池状态
	*
	* \return 连接池状态
	*/
	virtual const int getPoolState() const
	{
		return 0;
	}

protected:

	/**
	* \brief 构造函数
	* 
	* 受保护的构造函数，实现了Singleton设计模式，保证了一个进程中只有一个类实例
	*
	* \param name 名称
	*/
	LNetService(const std::string &name) : LService(name)
	{
		instance = this;

		serviceName = name;
		tcpServer = NULL;
	}

	bool init(WORD port);
	bool serviceCallback();
	void final();

private:

	static LNetService *instance;    /**< 类的唯一实例指针，包括派生类，初始化为空指针 */
	std::string serviceName;      /**< 网络服务器名称 */

	LAcceptThread* pAcceptThread; // [ranqd] 接收连接线程
public:
	LTCPServer *tcpServer;        /**< TCP服务器实例指针 */
};
// [ranqd] 接收连接线程类
class LAcceptThread : public LThread
{
public:
	LAcceptThread( LNetService* p, const std::string &name ): LThread(name)
	{
		pService = p;
	}
	~LAcceptThread()
	{
		final();
		join();
	}
	LNetService* pService;

	void run()         // [ranqd] 接收连接线程函数
	{
		while(!isFinal())
		{
			//Zebra::logger->debug("接收连接线程建立！");
			struct sockaddr_in addr;
			if( pService->tcpServer != NULL )
			{
				int retcode = pService->tcpServer->accept(&addr);
				if (retcode >= 0) 
				{
					//接收连接成功，处理连接
					pService->newTCPTask(retcode,&addr);
				}
			}
		}
	}
};