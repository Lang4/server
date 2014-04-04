/**
 * \file
 * \version  $Id: LTCPServer.h  $
 * \author  
 * \date 
 * \brief ��װTCP�ķ���������ģ��
 *
 * 
 */

#ifndef _zTCPServer_h_
#define _zTCPServer_h_

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string>

#include "LCantCopy.h"
#include "LSocket.h"

/**
 * \brief zTCPServer�࣬��װ�˷���������ģ�飬���Է���Ĵ���һ�����������󣬵ȴ��ͻ��˵�����
 *
 */
class LTCPServer : private LCantCopy
{

	public:

		LTCPServer(const std::string &name);
		~LTCPServer();
		bool bind(const std::string &name, const unsigned short port);
		int accept(struct sockaddr_in *addr);

	private:

		static const int T_MSEC =2100;			/**< ��ѯ��ʱ������ */
		static const int MAX_WAITQUEUE = 2000;	/**< ���ȴ����� */

		std::string name;						/**< ���������� */
		int sock;								/**< �׽ӿ� */
#ifdef _USE_EPOLL_
		int kdpfd;
#endif

}; 


#endif
