#pragma once
#include "LCantCopy.h"
#include "LType.h"
#include <string>
/**
* \brief zTCPServer类，封装了服务器监听模块，可以方便的创建一个服务器对象，等待客户端的连接
*
*/
class LTCPServer : private LCantCopy
{

public:

	LTCPServer(const std::string &name);
	~LTCPServer();
	bool bind(const std::string &name,const WORD port);
	int accept(struct sockaddr_in *addr);

private:

	static const int T_MSEC =2100;      /**< 轮询超时，毫秒 */
	static const int MAX_WAITQUEUE = 2000;  /**< 最大等待队列 */

	std::string name;            /**< 服务器名称 */
	SOCKET sock;                /**< 套接口 */
}; 
