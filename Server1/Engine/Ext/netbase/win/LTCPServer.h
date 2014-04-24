#pragma once
#include "LCantCopy.h"
#include "LType.h"
#include <string>
/**
* \brief zTCPServer�࣬��װ�˷���������ģ�飬���Է���Ĵ���һ�����������󣬵ȴ��ͻ��˵�����
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

	static const int T_MSEC =2100;      /**< ��ѯ��ʱ������ */
	static const int MAX_WAITQUEUE = 2000;  /**< ���ȴ����� */

	std::string name;            /**< ���������� */
	SOCKET sock;                /**< �׽ӿ� */
}; 
