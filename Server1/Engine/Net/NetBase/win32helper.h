#pragma once

#include <winsock2.h>
#include <MSWSock.h>
#pragma comment(lib,"ws2_32.lib")
class AcceptHelper{
public:
	AcceptHelper(){initLib = false;}
	void init(unsigned int socket)
	{
		if (initLib) return;
		initLib = true;
		GUID GuidAcceptEx = WSAID_ACCEPTEX;  
		DWORD dwBytes = 0;  
		if(SOCKET_ERROR == WSAIoctl(
			socket, 
			SIO_GET_EXTENSION_FUNCTION_POINTER, 
			&GuidAcceptEx, 
			sizeof(GuidAcceptEx), 
			&m_lpfnAcceptEx, 
			sizeof(m_lpfnAcceptEx), 
			&dwBytes, 
			NULL, 
			NULL))  
		{  
			initLib = false;
		}
	}
	static AcceptHelper &getMe()
	{
		static AcceptHelper helper;
		return helper;
	}
	void doAccept(SOCKET socket,SOCKET handle,WSABUF *lbuf,OVERLAPPED *lp)
	{
		init(socket);
		DWORD dwBytes = 0;  
		if(FALSE == (m_lpfnAcceptEx)(socket,
								handle,
								lbuf->buf,0,   
								sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN)+16, &dwBytes, lp)) 
			{  
				
			}
	}
	bool initLib;
	LPFN_ACCEPTEX m_lpfnAcceptEx;
};