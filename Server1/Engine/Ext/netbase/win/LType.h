#pragma once
#include "windows.h"
#include "errno.h"

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib, "shlwapi.lib") 
extern long g_RecvSize;
extern long g_SendSize;
extern long g_WantSendSize;
extern DWORD g_SocketSize; 

#define SAFE_DELETE(x) { if (NULL != x) { delete (x); (x) = NULL; } }
#define SAFE_DELETE_VEC(x) { if (NULL != x) { delete [] (x); (x) = NULL; } }

/**
 * \brief 双字节符号整数
 *
 */
typedef signed short SWORD;

/**
 * \brief 四字节符号整数
 *
 */
typedef long int SDWORD;

#ifdef _MSC_VER

typedef unsigned __int64 QWORD;
typedef signed __int64 SQWORD;

#else //_MSC_VER

/**
 * \brief 八字节无符号整数
 *
 */
typedef unsigned long long QWORD;

/**
 * \brief 八字节符号整数
 *
 */
typedef signed long long SQWORD;
#endif

#define SHUT_RDWR       SD_BOTH
#define SHUT_RD         SD_RECEIVE
#define SHUT_WR         SD_SEND

#define MSG_NOSIGNAL    0
#define EWOULDBLOCK     WSAEWOULDBLOCK

#define USE_IOCP true // [ranqd] 是否使用IOCP收发数据

// [ranqd] 包头格式定义
#pragma pack(1)
struct PACK_HEAD
{
	unsigned char Header[2];
	unsigned short Len;
	PACK_HEAD()
	{
		Header[0] = 0xAA;
		Header[1] = 0xDD;
	}
};
// [ranqd] 包尾格式定义
struct PACK_LAST
{
	unsigned char Last;
	PACK_LAST()
	{
		Last = 0xAA;
	}
};
#pragma pack()
#define PACKHEADLASTSIZE (sizeof(PACK_HEAD))

#define PACKHEADSIZE    sizeof(PACK_HEAD)

#define PACKLASTSIZE    0

const DWORD trunkSize = 64 * 1024;
#define unzip_size(zip_size) ((zip_size) * 120 / 100 + 12)
const DWORD PACKET_ZIP_BUFFER  =  unzip_size(trunkSize - 1) + sizeof(DWORD) + 8;  /**< 压缩需要的缓冲 */
/*
inline void mymemcpy(void* pDst, DWORD dwDstSize, void* pScr, DWORD dwCpSize )
{
	if(dwCpSize>dwDstSize)
	{
		MessageBox(NULL,"内存操作将越界","错误",MB_ICONERROR);
	}
	memcpy_s(pDst,dwDstSize,pScr,dwCpSize);
}

*/

//#define memcpy(d,s,size,dsize) memcpy_s((void*)(d),dsize,(void*)(s),size)

#ifndef HAVE_BZERO
#define bzero(p,s)      memset(p,0,s)
#define bcopy(s,d,ss,ds) memcpy(d,s,ss)
#endif //HAVE_BZERO


class LTCPTaskPool;
class LSocket;
class LTCPTask;
class Timer;



#define SHUT_RDWR       SD_BOTH
#define SHUT_RD         SD_RECEIVE
#define SHUT_WR         SD_SEND

#define MSG_NOSIGNAL    0
#define EWOULDBLOCK     WSAEWOULDBLOCK

#ifdef __cplusplus
extern "C"{
#endif //__cplusplus

#define POLLIN  1       /* Set if data to read. */
#define POLLPRI 2       /* Set if urgent data to read. */
#define POLLOUT 4       /* Set if writing data wouldn't block. */

	class LSocket;

	struct pollfd {
		int fd;
		short events;
		short revents;
		LSocket* pSock;
	};

	extern int poll(struct pollfd *fds,unsigned int nfds,int timeout);
	extern int WaitRecvAll( struct pollfd *fds,unsigned int nfds,int timeout );

#ifdef __cplusplus
}
#endif //__cplusplus
#pragma pack(1)
namespace Cmd{
	const int CMD_NULL = 0;
	const int PARA_NULL = 0;
	struct t_NullCmd{
		unsigned short para;
		unsigned short cmd;
		t_NullCmd()
		{
			para = cmd = 0;
		}
	};
}
#pragma pack()
// [ranqd] IO操作状态标志
typedef   enum   enum_IOOperationType   
{     
	IO_Write,     // 写
	IO_Read		  // 读

}IOOperationType,   *LPIOOperationType;

// [ranqd] 自定义IO操作结构，指明操作类型
typedef   struct   st_OverlappedData   
{   
	OVERLAPPED Overlapped;
	IOOperationType OperationType;

	st_OverlappedData( enum_IOOperationType type )
	{
		ZeroMemory( &Overlapped, sizeof(OVERLAPPED) );
		OperationType = type;
	}

}OverlappedData,   *LPOverlappedData;

/**
* \brief 定义了消息处理接口，所有接收到的TCP数据指令需要通过这个接口来处理
*/
class LProcessor
{
public:
	virtual bool msgParse(const void *,const unsigned int) = 0;
};
