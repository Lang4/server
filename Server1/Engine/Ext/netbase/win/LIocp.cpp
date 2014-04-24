#include "LIocp.h"
#include <shlwapi.h>
#include "LSocket.h"
std::vector<void*> G_MemList;
long g_RecvSize = 0;
long g_SendSize = 0;
long g_WantSendSize = 0;
void clrscr(int lines) 
{ 
	int s; 
	COORD c={0,0}; 
	HANDLE h=GetStdHandle(STD_OUTPUT_HANDLE); 
	//c.X=c.Y=0; 
	DWORD dwL; 
	TCHAR tc=32; 
	CONSOLE_SCREEN_BUFFER_INFO inf; 
	GetConsoleScreenBufferInfo(h,&inf); 
	if (lines<0) lines=inf.dwSize.Y; else { 
		if (inf.dwSize.Y < lines) lines=inf.dwSize.Y; 
	} 
	s=inf.dwSize.X * lines; 
	FillConsoleOutputCharacter(h,tc,s,c,&dwL); 
	SetConsoleCursorPosition (h,c); 

} 

void LIocp::UpdateNetLog()
{
	// [ranqd] 每秒更新一次网络数据流量
	char OutStr[MAX_PATH * 10];
	char RecStr[MAX_PATH];
	char SendStr[MAX_PATH];
	char WantSendStr[MAX_PATH];
	char szName[MAX_PATH];
	char szCode[MAX_PATH];

	if( g_WantSendSize < 1024 )
	{
		sprintf( WantSendStr, "%dB", g_WantSendSize );
	}
	else if( g_WantSendSize < 1024 * 1024 )
	{
		sprintf( WantSendStr, "%dKB", g_WantSendSize / 1024 );
	}
	else if( g_WantSendSize < 1024 * 1024 * 1024 )
	{
		sprintf( WantSendStr, "%dMB", g_WantSendSize / 1024 / 1024 );
	}
	else
	{
		sprintf( WantSendStr, "%dGB", g_WantSendSize / 1024 / 1024 / 1024 );
	}

	if( g_RecvSize < 1024 )
	{
		sprintf( RecStr, "%dB", g_RecvSize );
	}
	else if( g_RecvSize < 1024 * 1024 )
	{
		sprintf( RecStr, "%dKB", g_RecvSize / 1024 );
	}
	else if( g_RecvSize < 1024 * 1024 * 1024 )
	{
		sprintf( RecStr, "%dMB", g_RecvSize / 1024 / 1024 );
	}
	else
	{
		sprintf( RecStr, "%dGB", g_RecvSize / 1024 / 1024 / 1024 );
	}
	if( g_SendSize < 1024 )
	{
		sprintf( SendStr, "%dB", g_SendSize );
	}
	else if( g_SendSize < 1024 * 1024 )
	{
		sprintf( SendStr, "%dKB", g_SendSize / 1024 );
	}
	else if( g_SendSize < 1024 * 1024 * 1024 )
	{
		sprintf( SendStr, "%dMB", g_SendSize / 1024 / 1024 );
	}
	else
	{
		sprintf( SendStr, "%dGB", g_SendSize / 1024 / 1024 / 1024 );
	}
	sprintf( OutStr, "收：%s/s 发：%s/s %s/s", RecStr, SendStr,WantSendStr);
	g_RecvSize = 0;
	g_SendSize = 0;
	g_WantSendSize = 0;
	GetModuleFileName(NULL,szName,sizeof(szName));
	_snprintf(szCode,sizeof(szCode),"我是主角 - %s - %s",PathFindFileName(szName),OutStr);
	SetConsoleTitle(szCode);
}
// [ranqd] 数据收发记录
// [ranqd] 数据收发记录


// [ranqd] 获取系统的CPU数量
DWORD GetCPUCount()
{
	SYSTEM_INFO  sysinfo;
	GetSystemInfo( &sysinfo );
	return sysinfo.dwNumberOfProcessors;
}
LPTSTR AfxGetLastErrMsg()
{
	LPTSTR lpTStr;
	DWORD dwErr=GetLastError();
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		dwErr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpTStr,
		0x100,
		NULL);
	return lpTStr;
}
LPTSTR GetErrMsg(DWORD dwErr);
// Iocp数据接收线程
void LIocpRecvThread::run()
{
	BOOL    bResult;
	DWORD   dwNumRead;
	LPOverlappedData lpOverlapped = NULL;

	DWORD retval;
	LSocket* pSock = NULL;
	printf("IOCP线程建立！\n");
	while( !isFinal() )
	{
		bResult = GetQueuedCompletionStatus( LIocp::getInstance().m_ComPort, &dwNumRead, (LPDWORD)&pSock, (LPOVERLAPPED*)&lpOverlapped, INFINITE );
		if( bResult == FALSE )
		{
			//DWORD err = GetLastError();
			////Zebra::logger->debug("IOCP发生错误，错误信息：%s 错误码：0x%0.8X", GetErrMsg(err),err);
			//printf("IOCP发生错误，错误信息：%s 错误码：0x%0.8X\n", GetErrMsg(err),err);
			if( lpOverlapped == NULL )
			{
				printf("Iocp非正常返回！\n");
			}
			continue;
		}
		else if( pSock == NULL )
		{
			printf("Iocp返回空指针！\n");
			continue;
		}
		else if( dwNumRead == 0 || dwNumRead == 0xFFFFFFFF )
		{
			continue;
		}
		else if( dwNumRead == 0xFFFFFFFF - 1 )// 投递第一个读请求
		{
			pSock->ReadByte( sizeof(PACK_HEAD) );
		}
		else if( lpOverlapped->OperationType == IO_Read )
		{
			{
				pSock->RecvData( dwNumRead );
			}
		}
		else if( lpOverlapped->OperationType == IO_Write )
		{
			{
				pSock->SendData( dwNumRead );
			}
		}
	}
}

LIocp* LIocp::instance = NULL;


LIocp::LIocp()
{
	m_ComPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
}
void LIocp::start()
{
	// [ranqd] 获取系统CPU数量，分配CPU * 2 + 2个数据接收线程
	m_dwThreadCount = GetCPUCount() * 2 + 2;
	for(DWORD i = 0; i < m_dwThreadCount; i++)
	{
		LIocpRecvThread* pThread = new LIocpRecvThread();
		pThread->start();
		m_RecvThreadList.push_back(pThread);
	}
}

LIocp::~LIocp()
{
	for( std::vector<LIocpRecvThread*>::iterator it = m_RecvThreadList.begin(); 
		it != m_RecvThreadList.end(); it++)
	{
		(*it)->final();
		delete (*it);
	}
	m_RecvThreadList.clear();
}

void LIocp::BindIocpPort(HANDLE hIo, LSocket* key)
{
		::CreateIoCompletionPort( (HANDLE)hIo, m_ComPort, (ULONG_PTR)key, m_dwThreadCount );
	// 投递一个读数据请求
	::PostQueuedCompletionStatus( m_ComPort, 0xFFFFFFFF - 1, (ULONG_PTR)key, NULL );
	printf("IOCP端口绑定！\n");
}

void LIocp::PostStatus(LSocket* key, LPOverlappedData lpOverLapped)
{
	::PostQueuedCompletionStatus( m_ComPort, 0xFFFFFFFF, (ULONG_PTR)key, (LPOVERLAPPED)lpOverLapped );
}
