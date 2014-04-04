/**
 * \file
 * \version  $Id: LTCPServer.cpp  $
 * \author  
 * \date 2004��11��02�� 17ʱ31��02�� CST
 * \brief ʵ����zTCPServer
 *
 * 
 */

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>

#include "LSocket.h"
#include "LTCPServer.h"

/**
 * \brief ���캯�������ڹ���һ��������zTCPServer����
 * \param name ����������
 */
LTCPServer::LTCPServer(const std::string &name)
: name(name),
	sock(-1)
{
	kdpfd = epoll_create(1);
	assert(-1 != kdpfd);
}

/**
 * \brief ������������������һ��zTCPServer����
 *
 *
 */
LTCPServer::~LTCPServer() 
{
	TEMP_FAILURE_RETRY(::close(kdpfd));
	if (-1 != sock) 
	{
		::shutdown(sock, SHUT_RD);
		TEMP_FAILURE_RETRY(::close(sock));
		sock = -1;
	}
}

/**
 * \brief �󶨼�������ĳһ���˿�
 * \param name �󶨶˿�����
 * \param port ����󶨵Ķ˿�
 * \return ���Ƿ�ɹ�
 */
bool LTCPServer::bind(const std::string &name, const unsigned short port)
{
	struct sockaddr_in addr;

	if (-1 != sock) 
	{
		return false;
	}

	sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (-1 == sock) 
	{
		return false;
	}

	//�����׽ӿ�Ϊ������״̬
	int reuse = 1;
	if (-1 == ::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) 
	{
		TEMP_FAILURE_RETRY(::close(sock));
		sock = -1;
		return false;
	}

	//�����׽ӿڷ��ͽ��ջ��壬���ҷ������ı�����accept֮ǰ����
	socklen_t window_size = 128 * 1024;
	if (-1 == ::setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &window_size, sizeof(window_size)))
	{
		TEMP_FAILURE_RETRY(::close(sock));
		return false;
	}
	if (-1 == ::setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &window_size, sizeof(window_size)))
	{
		TEMP_FAILURE_RETRY(::close(sock));
		return false;
	}

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	int retcode = ::bind(sock, (struct sockaddr *) &addr, sizeof(addr));
	if (-1 == retcode) 
	{
		TEMP_FAILURE_RETRY(::close(sock));
		sock = -1;
		return false;
	}

	retcode = ::listen(sock, MAX_WAITQUEUE);
	if (-1 == retcode) 
	{
		TEMP_FAILURE_RETRY(::close(sock));
		sock = -1;
		return false;
	}

#ifdef _USE_EPOLL_
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.ptr = NULL;
	assert(0 == epoll_ctl(kdpfd, EPOLL_CTL_ADD, sock, &ev));
#endif


	return true;
}

/**
 * \brief ���ܿͻ��˵�����
 *
 *
 * \param addr ���صĵ�ַ
 * \return ���صĿͻ����׽ӿ�
 */
int LTCPServer::accept(struct sockaddr_in *addr)
{
	socklen_t len = sizeof(struct sockaddr_in);
	bzero(addr, sizeof(struct sockaddr_in));

#ifdef _USE_EPOLL_
	struct epoll_event ev;
	int rc = epoll_wait(kdpfd, &ev, 1, T_MSEC);
	if (1 == rc && (ev.events & EPOLLIN))
		//׼���ý���
		return TEMP_FAILURE_RETRY(::accept(sock, (struct sockaddr *)addr, &len));
#else
	struct pollfd pfd;
	pfd.fd = sock;
	pfd.events = POLLIN;
	pfd.revents = 0;
	int rc = TEMP_FAILURE_RETRY(::poll(&pfd, 1, T_MSEC));
	if (1 == rc && (pfd.revents & POLLIN))
		//׼���ý���
		return TEMP_FAILURE_RETRY(::accept(sock, (struct sockaddr *)addr, &len));
#endif

	return -1;
}

