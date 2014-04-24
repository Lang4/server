/**
* \brief 实现类zSocket
*
* 
*/
#include "LSocket.h"
#include <assert.h>
#include "LIocp.h"
/**
* 构造函数
* 模板偏特化
* 动态分配内存的缓冲区,大小可以随时扩展
*/
template <>
t_BufferCmdQueue::ByteBuffer()
: _maxSize(trunkSize),_offPtr(0),_currPtr(0),_buffer(_maxSize) { }

/**
* 构造函数
* 模板偏特化
* 静态数组的缓冲区,大小不能随时改变
*/
template <>
t_StackCmdQueue::ByteBuffer()
: _maxSize(PACKET_ZIP_BUFFER),_offPtr(0),_currPtr(0) { }

/**
* \brief 构造函数,初始化对象
* \param sock 套接口
* \param addr 地址
* \param compress 底层数据传输是否支持压缩
*/

LSocket::LSocket(
				 const SOCKET sock,
				 const struct sockaddr_in *addr,
				 const bool compress,
				 const bool useIocp,
				 LTCPTask* pTask)
				 :m_ovIn( IO_Read),
				 m_ovOut( IO_Write ),
				 m_dwSendCount(0),
				 m_dwRecvCount(0),
				 m_dwMySendCount(0)
{
	//	Zebra::logger->debug("新建连接 %0.8X", (DWORD)pTask );

	m_bIocpDeleted = false;
	m_bTaskDeleted = false;

	assert(INVALID_SOCKET != sock);

	this->m_bUseIocp = useIocp;

	this->sock = sock;

	this->pTask = pTask;

	bzero(&this->addr,sizeof(struct sockaddr_in));
	if (NULL == addr) 
	{
		int len = sizeof(struct sockaddr);
		getpeername(this->sock,(struct sockaddr *)&this->addr,&len);
	}
	else 
	{
		bcopy(addr,&this->addr,sizeof(struct sockaddr_in),sizeof(struct sockaddr_in));
	}
	bzero(&this->local_addr,sizeof(struct sockaddr_in));
	{
		int len = sizeof(struct sockaddr_in);
		getsockname(this->sock,(struct sockaddr *)&this->local_addr,&len);
	}

	setNonblock();

	rd_msec = T_RD_MSEC;
	wr_msec = T_WR_MSEC;
	_rcv_raw_size = 0;
	_current_cmd = 0; 

	// [ranqd]建立接收通知事件
	//	m_hRecvEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	if( useIocp )
	{
		// [ranqd] 绑定IO到完成端口上
		LIocp::getInstance().BindIocpPort( (HANDLE)sock, this );
	}

	set_flag(INCOMPLETE_READ | INCOMPLETE_WRITE);
	if (compress)
		bitmask |= PACKET_ZIP;
}


/**
* \brief 析构函数
*/
LSocket::~LSocket()
{
	//	Zebra::logger->debug("关闭套接口连接");

	::shutdown(sock,0x02);
	::closesocket(sock);
	sock = INVALID_SOCKET;
}

LPTSTR GetErrMsg(DWORD dwErr)
{
	LPTSTR lpTStr;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		dwErr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpTStr,
		0x100,
		NULL);
	return lpTStr;
}
// [ranqd] 投递IO读请求
void LSocket::ReadByte( DWORD size )
{
	BOOL    bresult;
	int     err;
	DWORD   numRead;

	m_RecvBuffer.wr_reserve( size );

	if( size == 0 ) 
	{
		int iii = 0;
	}
	// 请求读取数据
	bresult = ReadFile(
		(HANDLE)sock,
		m_RecvBuffer.wr_buf(),
		size,
		&numRead,
		(LPOVERLAPPED)&m_ovIn );
	// 这里,等待一个信息包.
	if (bresult) return;
	err = GetLastError();
	if (err == 64)//连接已关闭
	{
		LIocp::getInstance().PostStatus(this,&m_ovIn);//发送读取失败的完成信息
		return;
	}
	if( err != 997 )
	{
		printf( "接收数据时重叠I/O操作发生错误： %s",GetErrMsg(err) );
	}
}
// [ranqd] 通过Iocp发送数据
int LSocket::SendData( DWORD dwNum = 0 )
{
	BOOL    bresult;
	int     err;
	DWORD   numRead;

	// [ranqd] 记录发送数据量
	InterlockedExchangeAdd(&g_SendSize, (long)dwNum);

	tQueue.rd_flip( dwNum );

	m_dwSendCount += dwNum;

	int size = tQueue.rd_size();

	// 没有数据可发送了，返回
	if( size == 0 )
	{
		return dwNum;
	}

	if( sock == INVALID_SOCKET ) return -1;

	tQueue.Lock();

	bresult = WriteFile(
		(HANDLE)sock,
		tQueue.rd_buf(),
		size,
		&numRead,
		(LPOVERLAPPED)&m_ovOut );

	tQueue.UnLock();

	// 这里,等待一个信息包.
	if (bresult) return size;
	err = GetLastError();

	if( err == 64 )//连接已关闭
	{
		LIocp::getInstance().PostStatus(this,&m_ovOut);//发送写数据失败的完成信息
		return -1;
	}

	if( err != 997 )
	{
		printf( "发送数据时重叠I/O操作发生错误： %s",GetErrMsg(err) );
		return -1;
	}

	return size;
}

void LSocket::RecvData( DWORD dwNum )
{
	unsigned short nLen = 0;
	if( dwNum == 0 )
	{
		return;
	}
	// [ranqd] 记录接收数据量
	InterlockedExchangeAdd( &g_RecvSize, (LONG)dwNum );

	m_dwRecvCount += dwNum;	
	static PACK_HEAD head;
	
	m_RecvBuffer.wr_flip( dwNum );
	m_RecvLock.lock();
	
	if( m_RecvBuffer.rd_size() == PACKHEADSIZE )
	{
		// [ranqd] 收到的数据错误，需要关闭连接，为便于调试，暂不关闭连接
		if(( m_RecvBuffer.rd_buf()[0] != head.Header[0] || m_RecvBuffer.rd_buf()[1] != head.Header[1]) )
		{
			printf("Iocp收到错误的包头来自： %u.%u.%u.%u : %u", addr.sin_addr.S_un.S_un_b.s_b1,addr.sin_addr.S_un.S_un_b.s_b2,addr.sin_addr.S_un.S_un_b.s_b3,addr.sin_addr.S_un.S_un_b.s_b4, addr.sin_port / 0xFF + addr.sin_port % 256 * 256);
			printf("错误数据：", m_RecvBuffer.rd_buf(), m_RecvBuffer.rd_size() );
			DisConnet();
			goto ret;
		}
		else
		{
			nLen = ((PACK_HEAD*)m_RecvBuffer.rd_buf())->Len;
			ReadByte( nLen );
			goto ret;
		}
	}
	else if( m_RecvBuffer.rd_size() < PACKHEADSIZE )
	{
		// [ranqd] 包头没有收完，继续请求后面的数据
		ReadByte( PACKHEADSIZE - m_RecvBuffer.rd_size() );
		goto ret;
	}
	else
	{
		nLen = ((PACK_HEAD*)m_RecvBuffer.rd_buf())->Len;

		if( nLen  > MAX_DATASIZE )
		{
			//[ranqd] 收到的长度有误，应该干掉
			m_RecvBuffer.reset();
			m_ovIn = OverlappedData( IO_Read );
			ReadByte( sizeof(PACK_HEAD) );
			goto ret;
		}
		if( m_RecvBuffer.rd_size() == nLen + PACKHEADSIZE )
		{
			// [ranqd] 收到一个完整的包，放入缓冲
			_rcv_queue.Lock();
			_rcv_queue.put( m_RecvBuffer.rd_buf(), nLen + PACKHEADSIZE );
			_rcv_queue.UnLock();
			m_RecvBuffer.rd_flip( nLen + PACKHEADSIZE );
			m_ovIn = OverlappedData( IO_Read );
			ReadByte( sizeof( PACK_HEAD ) );
			goto ret;
		}
		else
		{
			ReadByte( PACKHEADSIZE + nLen - m_RecvBuffer.rd_size() );// 请求接收后面的数据
		}
	}
ret:
	m_RecvLock.unlock();
}

// [ranqd] 等待数据接收完成，bWait 表示是否要等到完成后才返回, timeout表示等待超时时间（单位秒）
// 返回值： 0 数据接收还没有完成， -1 连接已经断开, -2 等待超时 > 0 已经收到的数据长度
int LSocket::WaitRecv( bool bWait, int timeout )
{
	if( bWait )
	{
		DWORD EnterTime = GetTickCount();
		do// 等到数据接收完成或者断开连接
		{
			if( _rcv_queue.rd_size() > 0 )
			{
				return _rcv_queue.rd_size();
			}
			// 检测连接是否已断开      
			if( sock == INVALID_SOCKET )
			{
				return -1;
			}
			::Sleep(10);

		}while( timeout == 0 || GetTickCount() - EnterTime < timeout * 1000 );

		return 0; // 超时
	}
	else // 不等待，只是判断
	{
		if( sock == INVALID_SOCKET )
			return -1;

		return _rcv_queue.rd_size();
	}
}
// [ranqd] 等待数据发送完成，bWait表示是否要等到完成后才返回，timeout表示等待超时时间(单位秒)
// 返回值：0 数据发送还没完成， -1 连接已断开， 1 已准备好发送数据
int LSocket::WaitSend( bool bWait, int timeout )
{
	if( bWait )
	{
		DWORD EnterTime = GetTickCount();
		do// 等到数据接收完成或者断开连接
		{
			if( tQueue.rd_size() == 0 )
			{
				return 1;
			}
			// 检测连接是否已断开      
			if( sock == INVALID_SOCKET )
			{
				return -1;
			}
			::Sleep(10);

		}while( timeout == 0 || GetTickCount() - EnterTime < timeout * 1000 );

		return 0; // 超时
	}
	else // 不等待，只是判断
	{
		if( sock == INVALID_SOCKET )
			return -1;

		return tQueue.rd_size() == 0;
	}
}
#define success_unpack() \
	do { \
	if (_rcv_raw_size >= packetMinSize()/* && _rcv_queue.rd_size() >= packetMinSize()*/) \
{ \
	DWORD nRecordLen = packetSize(_rcv_queue.rd_buf() + PACKHEADSIZE); \
	if (_rcv_raw_size >= nRecordLen/* && _rcv_queue.rd_size() >= nRecordLen*/) \
{ \
	_rcv_queue.Lock();\
	int retval = packetUnpack(_rcv_queue.rd_buf(),nRecordLen,(BYTE*)pstrCmd); \
	_rcv_queue.UnLock();\
	_rcv_queue.rd_flip(nRecordLen + PACKHEADLASTSIZE); \
	InterlockedExchangeAdd((LONG*)&_rcv_raw_size, -( nRecordLen + PACKHEADLASTSIZE)); \
	return retval; \
} \
} \
	} while(0)

/**
* \brief 接收指令到缓冲区
* \param pstrCmd 指令缓冲区
* \param nCmdLen 指令缓冲区的大小
* \param wait 当套接口数据没有准备好的时候,是否需要等待
* \return 实际接收的指令大小
*       返回-1,表示接收错误
*       返回0,表示接收超时
*       返回整数,表示实际接收的字节数
*/
int LSocket::recvToCmd(void *pstrCmd,const int nCmdLen,const bool wait)
{
	//Zebra::logger->debug("LSocket::recvToCmd");
	//够一个完整的纪录,不需要向套接口接收数据
	do { 
		if (_rcv_raw_size >= packetMinSize()/* && _rcv_queue.rd_size() >= packetMinSize()*/) 
		{ 
			DWORD nRecordLen = packetSize(&_rcv_queue.rd_buf()[PACKHEADSIZE]); 
			if (_rcv_raw_size >= nRecordLen/* && _rcv_queue.rd_size() >= nRecordLen*/) 
			{ 
				int retval = packetUnpack(_rcv_queue.rd_buf(),nRecordLen,(BYTE*)pstrCmd); 
				_rcv_queue.rd_flip(nRecordLen + PACKHEADLASTSIZE); 
				InterlockedExchangeAdd( (LONG*)&_rcv_raw_size, -( nRecordLen + PACKHEADLASTSIZE)); 
				return retval; 
			} 
		}
		else
		{
			break;
		}
	} while(0);

	do {
		int retval = recvToBuf();
		if (-1 == retval || (0 == retval && !wait))
			return retval;

		//够一个完整的纪录,不需要向套接口接收数据
		do { 
			if (_rcv_raw_size >= packetMinSize()/* && _rcv_queue.rd_size() >= packetMinSize()*/) 
			{ 
				DWORD nRecordLen = packetSize(&_rcv_queue.rd_buf()[PACKHEADSIZE]); 
				if (_rcv_raw_size >= nRecordLen/* && _rcv_queue.rd_size() >= nRecordLen*/) 
				{ 
					int retval = packetUnpack(_rcv_queue.rd_buf(),nRecordLen,(BYTE*)pstrCmd); 
					_rcv_queue.rd_flip(nRecordLen + PACKHEADLASTSIZE); 
					InterlockedExchangeAdd((LONG*)&_rcv_raw_size, -( nRecordLen + PACKHEADLASTSIZE )); 
					return retval; 
				} 
			}
			else
			{
				break;
			}
		} while(0);
	} while(true);

	return 0;
}

// [ranqd]  重新封装底层发送函数，可以在这里进行封包的操作
int  LSocket::Send(const SOCKET sock, const void* pBuffer, const int nLen,int flags)
{
	// [ranqd] 检查待发送的数据，是否符合封包格式

	//PACK_HEAD* phead;
	//int offset = 0;
	//do 
	//{
	//	phead = (PACK_HEAD*)((DWORD)pBuffer + offset);
	//	if( phead->Header[0] != 0xAA || phead->Header[1] != 0xDD )
	//	{
	//		Zebra::logger->debug("发送未封包数据！");
	//		return nLen;
	//	}
	//	offset += phead->Len + PACKHEADSIZE;
	//} while(offset < nLen );
	tQueue.Lock();
	tQueue.put((const BYTE*)pBuffer, nLen);
	tQueue.UnLock();

	m_dwMySendCount += nLen;

	/*	Zebra::logger->debug("目标端口： ( %u.%u.%u.%u : %u -> %u.%u.%u.%u : %u )",
	local_addr.sin_addr.S_un.S_un_b.s_b1,local_addr.sin_addr.S_un.S_un_b.s_b2,local_addr.sin_addr.S_un.S_un_b.s_b3,local_addr.sin_addr.S_un.S_un_b.s_b4, local_addr.sin_port / 0xFF + local_addr.sin_port % 256 * 256,
	addr.sin_addr.S_un.S_un_b.s_b1,addr.sin_addr.S_un.S_un_b.s_b2,addr.sin_addr.S_un.S_un_b.s_b3,addr.sin_addr.S_un.S_un_b.s_b4, addr.sin_port / 0xFF + addr.sin_port % 256 * 256);
	Zebra::logger->debug16( "发送", (const BYTE*)tQueue.rd_buf(), tQueue.rd_size());

	Zebra::logger->debug("%s : %u -> %s : %u 封装总长度： %u",getLocalIP(), getLocalPort(), getIP(), getPort(),m_dwMySendCount);
	*/
	int  SendLen = 0;	

	if( m_bUseIocp )
	{
		SendLen = SendData();
	}
	else
	{
		while(SendLen != nLen + PACKHEADLASTSIZE)
		{
			tQueue.Lock();
			int retcode = ::send(sock, (const char*)tQueue.rd_buf(), tQueue.rd_size(), flags);
			tQueue.UnLock();
			if( retcode <= 0 )
				return retcode;
			SendLen += retcode;
			tQueue.rd_flip( retcode );
		}
	}
	//	Zebra::logger->debug("发送结果： %d 包头： %d", SendLen, ((PACK_HEAD*)tQueue.rd_buf())->Len);
	return SendLen;
}
/**
* \brief 向套接口发送原始数据,没有打包的数据,一般发送数据的时候需要加入额外的包头
* \param pBuffer 待发送的原始数据
* \param nSize 待发送的原始数据大小
* \return 实际发送的字节数
*       返回-1,表示发送错误
*       返回0,表示发送超时
*       返回整数,表示实际发送的字节数
*/
int LSocket::sendRawData(const void *pBuffer,const int nSize)
{
	//fprintf(stderr,"LSocket::sendRawData\n");

	/*if(((Cmd::stNullUserCmd *)pBuffer)->byCmd == 5 && ((Cmd::stNullUserCmd *)pBuffer)->byParam == 55)
	{
	Cmd::stMapDataMapScreenUserCmd * tt = (Cmd::stMapDataMapScreenUserCmd *)(pBuffer);
	WORD _size = tt->mdih.size;
	fprintf(stderr,"问题消息\n");
	}*/	
	if (isset_flag(INCOMPLETE_WRITE))
	{
		clear_flag(INCOMPLETE_WRITE);
		goto do_select;
	}
	if( nSize > 10000 )
	{
		int iii = 0;
	}
	int retcode = Send(sock,(const char*)pBuffer,nSize,MSG_NOSIGNAL);
	if (retcode == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
	{
do_select:
		retcode = waitForWrite();
		if (1 == retcode)
		{
			mutex.lock();
			retcode = Send(sock,(const char*)pBuffer,nSize,MSG_NOSIGNAL);
			mutex.unlock();
		}
		else
			return retcode; 
	}

	if (retcode > 0 && retcode < nSize)
		set_flag(INCOMPLETE_WRITE);

	return retcode;
}

/**
* \brief 发送指定字节数的原始数据,忽略超时,无限制时间的发送,直到发送完毕或者发送失败
* \param pBuffer 待发送的原始数据
* \param nSize 待发送的原始数据大小
* \return 发送数据是否成功
*/
bool LSocket::sendRawDataIM(const void *pBuffer,const int nSize)
{
	//Zebra::logger->debug("LSocket::sendRawDataIM");
	if (NULL == pBuffer || nSize <= 0)
		return false;
	int retcode = sendRawData(pBuffer,nSize);
	if (-1 == retcode)
	{
		printf("客户端断开 -1");  
		return false;
	}
	return true;
}

/**
* \brief 发送指令
* \param pstrCmd 待发送的内容
* \param nCmdLen 待发送内容的大小
* \param buffer 是否需要缓冲
* \return 发送是否成功
*/
bool LSocket::sendCmd(const void *pstrCmd,const int nCmdLen,const bool buffer)
{
	//Zebra::logger->debug("LSocket::sendCmd");
	if (NULL == pstrCmd || nCmdLen <= 0)
		return false;
	bool retval = true;
	//	Zebra::logger->debug("期望目标：%s : %u", getIP(), getPort());
	//	Zebra::logger->debug16("希望发送：", (BYTE*)pstrCmd, nCmdLen);
	if (buffer)
	{
		// 直接把数据加密压缩封包后放到发送缓冲_enc_queue
		t_StackCmdQueue _raw_queue;
		packetAppend(pstrCmd,nCmdLen,_raw_queue);
		mutex.lock();
		_enc_queue.put(_raw_queue.rd_buf(),_raw_queue.rd_size());
		mutex.unlock();
	}
	else
	{
		t_StackCmdQueue _raw_queue;
		packetAppend(pstrCmd,nCmdLen,_raw_queue);
		mutex.lock();
		retval = sendRawDataIM(_raw_queue.rd_buf(),_raw_queue.rd_size());
		mutex.unlock();
	}
	return retval;
}

/**
* \brief 发送原始指令数据,不做封包动作 [ranqd] 用于原始数据的转发
* \param pstrCmd 待发送的内容
* \param nCmdLen 待发送内容的大小
* \param buffer 是否需要缓冲
* \return 发送是否成功
*/
bool LSocket::sendCmdNoPack(const void *pstrCmd,const int nCmdLen,const bool buffer)
{
	//Zebra::logger->debug("LSocket::sendCmdNoPack");
	if (NULL == pstrCmd || nCmdLen <= 0)
		return false;

	bool retval = true;

	t_StackCmdQueue _raw_queue;
	_raw_queue.put((BYTE*)pstrCmd,nCmdLen);

	if (buffer)
	{
		if( need_enc() )
		{
			packetPackEnc( _raw_queue,_raw_queue.rd_size() );
		}
		PACK_HEAD head;
		head.Len = _raw_queue.rd_size();
		mutex.lock();
		_enc_queue.put((BYTE*)&head, sizeof(head)); // NoPack也要封包头
		_enc_queue.put( _raw_queue.rd_buf(),_raw_queue.rd_size() );
		mutex.unlock();
	}
	else
	{	
		if ( need_enc() )
		{
			packetPackEnc( _raw_queue,_raw_queue.rd_size() );
			mutex.lock();
			retval = sendRawDataIM( _raw_queue.rd_buf(),_raw_queue.rd_size() );
			mutex.unlock();
		}
		else
		{
			mutex.lock();
			retval = sendRawDataIM( _raw_queue.rd_buf(),_raw_queue.rd_size() );
			mutex.unlock();
		}
	}
	return retval;
}

bool LSocket::sync()
{
	//Zebra::logger->debug("LSocket::sync");
	if (_enc_queue.rd_ready())
	{
		mutex.lock();
		int retcode = sendRawData_NoPoll(_enc_queue.rd_buf(),_enc_queue.rd_size());
		mutex.unlock();
		if (retcode > 0)
		{
			_enc_queue.rd_flip(retcode);
		}
		else if (-1 == retcode)
		{
			return false;
		}
	}
	return true;
}

void LSocket::force_sync()
{
	//Zebra::logger->debug("LSocket::force_sync");
	if (_enc_queue.rd_ready())
	{
		mutex.lock();
		sendRawDataIM(_enc_queue.rd_buf(),_enc_queue.rd_size());
		_enc_queue.reset();
		mutex.unlock();
	}
}

/**
* \brief 检查套接口准备好读取操作
* \return 是否成功
*       -1,表示操作失败
*       0,表示操作超时
*       1,表示等待成功,套接口已经有数据准备读取
*/
int LSocket::checkIOForRead()
{
	//Zebra::logger->debug("LSocket::checkIOForRead");
	int retcode = 0;
	if( m_bUseIocp )
	{
		retcode = WaitRecv( false );
	}
	else
	{
		struct pollfd pfd;

		pfd.fd = sock;
		pfd.events = POLLIN | POLLPRI;
		pfd.revents = 0;

		retcode = ::poll(&pfd,1,0);
		if (retcode > 0 && 0 == (pfd.revents & POLLIN))
			retcode = -1;
	}
	return retcode;
}

/**
* \brief 等待套接口准备好写入操作
* \return 是否成功
*       -1,表示操作失败
*       0,表示操作超时
*       1,表示等待成功,套接口已经可以写入数据
*/
int LSocket::checkIOForWrite()
{
	//Zebra::logger->debug("LSocket::checkIOForWrite");
	if( m_bUseIocp ) // 使用Iocp时随时都可以写入数据
	{
		return WaitSend( false );
	}

	struct pollfd pfd;

	pfd.fd = sock;
	pfd.events = POLLOUT | POLLPRI;
	pfd.revents = 0;

	int retcode = ::poll(&pfd,1,0);
	if (retcode > 0 && 0 == (pfd.revents & POLLOUT))
		retcode = -1;

	return retcode;
}

/**
* \brief 根据网卡名称编号获取指定网卡的IP地址
* \param ifName 需要取地址的网卡名称
* \return 获取的指定网卡上IP地址
*/
const char *LSocket::getIPByIfName(const char *ifName)
{
	static char *none_ip = "0.0.0.0";

	printf("LSocket::getIPByIfName(%s)",ifName);

	if (NULL == ifName) return none_ip;

	if (0 != inet_addr(ifName)) return ifName;

	return none_ip;
}

#define success_recv_and_dec() \
	do { \
	if ((DWORD)retcode < _rcv_queue.wr_size()) \
{ \
	set_flag(INCOMPLETE_READ); \
} \
	if( !m_bUseIocp )\
{\
	_rcv_queue.wr_flip(retcode); \
}\
	DWORD size = _rcv_queue.rd_size() - PACKHEADLASTSIZE - _rcv_raw_size - ((_rcv_queue.rd_size() - PACKHEADLASTSIZE - _rcv_raw_size) % 8); \
	if (size) \
{ \
	_rcv_queue.Lock();\
	enc.encdec(&_rcv_queue.rd_buf()[_rcv_raw_size + PACKHEADSIZE],size,false); \
	_rcv_queue.UnLock();\
	InterlockedExchangeAdd((LONG*)&_rcv_raw_size,size + PACKHEADLASTSIZE); \
} \
	} while(0)

#define success_recv() \
	do { \
	if ((DWORD)retcode < _rcv_queue.wr_size()) \
{ \
	set_flag(INCOMPLETE_READ); \
} \
	if( !m_bUseIocp )\
{\
	_rcv_queue.wr_flip(retcode); \
}\
	InterlockedExchangeAdd((LONG*)&_rcv_raw_size,retcode); \
	} while(0)\

/**
* \brief   接收数据到缓冲区,保证在调用这个函数之前套接口准备好了接收,也就是使用poll轮询过
*       如果是加密包需要先解密到解密缓冲区
* \return   实际接收字节数
*       返回-1,表示接收错误
*       返回0,表示接收超时
*       返回整数,不加密时表示实际接收的字节数,加密时返回解密到的字节数
*/
int LSocket::recvToBuf_NoPoll()
{
	//Zebra::logger->debug("LSocket::recvToBuf_NoPoll");
	int retcode =0; 
	if (need_enc())
	{
		_rcv_queue.wr_reserve(MAX_DATABUFFERSIZE);
		if( m_bUseIocp )
		{
			retcode = _rcv_queue.rd_size();
		}
		else
		{
			retcode = ::recv(sock,(char*)_rcv_queue.wr_buf(),_rcv_queue.wr_size(),MSG_NOSIGNAL);
		}
		if (retcode == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
			return 0;//should retry

		if( retcode == 0 && m_bUseIocp )
			return 0;

		DWORD old_rcv_raw_size = _rcv_raw_size;
		if (retcode > 0) 
		{
			// [ranqd] 要可以同时处理多个加密包
			if ((DWORD)retcode < _rcv_queue.wr_size()) 
			{ 
				set_flag(INCOMPLETE_READ); 
			} 
			if( !m_bUseIocp )
			{
				_rcv_queue.wr_flip(retcode);
			}
			do {
				DWORD nLen = ((PACK_HEAD*)&_rcv_queue.rd_buf()[_rcv_raw_size])->Len;

				DWORD size = nLen  - (nLen % 8); 
				if (size) 
				{
					enc.encdec(&_rcv_queue.rd_buf()[_rcv_raw_size + PACKHEADSIZE],size,false); 
					InterlockedExchangeAdd((LONG*)&_rcv_raw_size, size + PACKHEADLASTSIZE); 
				} 
				else
				{
					break;
				}
			} while((long)(_rcv_raw_size - old_rcv_raw_size) < retcode);
		}
	}
	else
	{
		_rcv_queue.wr_reserve(MAX_DATABUFFERSIZE);
		if( m_bUseIocp )
		{
			retcode = _rcv_queue.rd_size();
		}
		else
		{
			retcode = ::recv(sock,(char*)_rcv_queue.wr_buf(),_rcv_queue.wr_size(),MSG_NOSIGNAL);
		}
		if (retcode == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
			return 0;//should retry

		if( retcode == 0 && m_bUseIocp )
			return 0;

		if (retcode > 0)
		{
			success_recv();
		}
	}

	if (0 == retcode)
		return -1;//EOF 

	return retcode;
}

/**
* \brief 接收指令到缓冲区,不从套接口接收指令,只是把接收缓冲的数据解包
* \param pstrCmd 指令缓冲区
* \param nCmdLen 指令缓冲区的大小
* \return 实际接收的指令大小
*       返回-1,表示接收错误
*       返回0,表示接收超时
*       返回整数,表示实际接收的字节数
*/
int LSocket::recvToCmd_NoPoll(void *pstrCmd,const int nCmdLen)
{
	//够一个完整的纪录,不需要向套接口接收数据
	//printf("-----------------------------------------:%d %d\n",_rcv_raw_size,packetMinSize());
	do { 
		if (_rcv_raw_size >= packetMinSize()/* && _rcv_queue.rd_size() >= packetMinSize()*/) 
		{ 
			DWORD nRecordLen = packetSize(_rcv_queue.rd_buf() + PACKHEADSIZE); 
			if (_rcv_raw_size >= nRecordLen/* && _rcv_queue.rd_size() >= nRecordLen*/) 
			{ 
				int retval = packetUnpack(_rcv_queue.rd_buf(),nRecordLen,(BYTE*)pstrCmd); 
				_rcv_queue.rd_flip(nRecordLen + PACKHEADLASTSIZE); 
				InterlockedExchangeAdd((LONG*)&_rcv_raw_size, -( nRecordLen + PACKHEADLASTSIZE )); 
				return retval; 
			} 
			else
			{
				printf("处理错误的包！");
			}
			
		} 
	} while(0);
	return 0;
}

/**
* \brief 向套接口发送原始数据,没有打包的数据,一般发送数据的时候需要加入额外的包头,保证在调用这个函数之前套接口准备好了接收,也就是使用poll轮询过
* \param pBuffer 待发送的原始数据
* \param nSize 待发送的原始数据大小
* \return 实际发送的字节数
*       返回-1,表示发送错误
*       返回0,表示发送超时
*       返回整数,表示实际发送的字节数
*/
int LSocket::sendRawData_NoPoll(const void *pBuffer,const int nSize)
{

	/*fprintf(stderr,"LSocket::sendRawData_NoPoll\n");


	if(((Cmd::stNullUserCmd *)pBuffer)->byCmd == 5 && ((Cmd::stNullUserCmd *)pBuffer)->byParam == 55)
	{
	Cmd::stMapDataMapScreenUserCmd * tt = (Cmd::stMapDataMapScreenUserCmd *)(pBuffer);
	WORD _size = tt->mdih.size;
	fprintf(stderr,"问题消息\n");
	}*/
	int retcode = Send(sock,(const char*)pBuffer,nSize,MSG_NOSIGNAL);
	if (retcode == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
		return 0;//should retry

	if (retcode > 0 && retcode < nSize)
		set_flag(INCOMPLETE_WRITE);

	return retcode;
}

/**
* \brief 设置套接口为非阻塞模式
* \return 操作是否成功
*/
bool LSocket::setNonblock()
{
	//Zebra::logger->debug("LSocket::setNonblock");
	int nodelay = 1;

	if (::setsockopt(sock,IPPROTO_TCP,TCP_NODELAY,(const char *)&nodelay,sizeof(nodelay)))
		return false;

	return true;
}

/**
* \brief 等待套接口准备好读取操作
* \return 是否成功
*       -1,表示操作失败
*       0,表示操作超时
*       1,表示等待成功,套接口已经有数据准备读取
*/
int LSocket::waitForRead()
{
	//Zebra::logger->debug("LSocket::waitForRead");
	int retcode = -1;

	if( m_bUseIocp )
	{
		retcode = WaitRecv( false );
	}
	else
	{
		struct pollfd pfd;

		pfd.fd = sock;
		pfd.events = POLLIN | POLLPRI;
		pfd.revents = 0;

		retcode = ::poll(&pfd,1,rd_msec);
		if (retcode > 0 && 0 == (pfd.revents & POLLIN))
			retcode = -1;
	}
	return retcode;
}


/**
* \brief 等待套接口准备好写入操作
* \return 是否成功
*       -1,表示操作失败
*       0,表示操作超时
*       1,表示等待成功,套接口已经可以写入数据
*/
int LSocket::waitForWrite()
{
	//Zebra::logger->debug("LSocket::waitForWrite");
	if( m_bUseIocp )
	{
		return WaitSend( true, wr_msec );
	}

	struct pollfd pfd;

	pfd.fd = sock;
	pfd.events = POLLOUT | POLLPRI;
	pfd.revents = 0;

	int retcode = ::poll(&pfd,1,wr_msec);
	if (retcode > 0 && 0 == (pfd.revents & POLLOUT))
		retcode = -1;

	return retcode;
}

/**
* \brief   接收数据到缓冲区
*       如果是加密包需要解密到解密缓冲区
* \return   实际接收字节数
*       返回-1,表示接收错误
*       返回0,表示接收超时
*       返回整数,不加密包表示实际接收的字节数,加密包返回解密后实际可用的字节数
*/
int LSocket::recvToBuf()
{
	//Zebra::logger->debug("LSocket::recvToBuf");
	int retcode = 0;

	if (need_enc())
	{
		if (isset_flag(INCOMPLETE_READ))
		{
			clear_flag(INCOMPLETE_READ);
			goto do_select_enc;
		}

		_rcv_queue.wr_reserve(MAX_DATABUFFERSIZE);
		if( m_bUseIocp )
		{
			retcode = WaitRecv( true, rd_msec / 1000 );
			if( retcode <= 0 ) return retcode;
		}
		else
		{
			retcode = ::recv(sock,(char*)_rcv_queue.wr_buf(),_rcv_queue.wr_size(),MSG_NOSIGNAL);
		}
		if ( retcode == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) && !m_bUseIocp )
		{
do_select_enc:
			if( m_bUseIocp )
			{
				retcode = WaitRecv( true, rd_msec / 1000 );
				if( retcode <= 0 ) return retcode;
			}
			else
			{
				retcode = waitForRead();
				if (1 == retcode)
					retcode = ::recv(sock,(char*)_rcv_queue.wr_buf(),_rcv_queue.wr_size(),MSG_NOSIGNAL);
				else
					return retcode;
			}
		}
		if (retcode > 0) 
			success_recv_and_dec();
	}
	else
	{
		if (isset_flag(INCOMPLETE_READ))
		{
			clear_flag(INCOMPLETE_READ);
			goto do_select;
		}
		_rcv_queue.wr_reserve(MAX_DATABUFFERSIZE);
		if( m_bUseIocp )
		{
			retcode = WaitRecv( true, rd_msec / 1000 );
			if( retcode <= 0 ) return retcode;
		}
		else
		{
			retcode = ::recv(sock,(char*)_rcv_queue.wr_buf(),_rcv_queue.wr_size(),MSG_NOSIGNAL);
		}
		if ( retcode == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) && !m_bUseIocp )
		{
do_select:
			if( m_bUseIocp )
			{
				retcode = WaitRecv( true, rd_msec / 1000 );
				if( retcode <= 0 ) return retcode;
			}
			else
			{
				retcode = waitForRead();
				if (1 == retcode)
					retcode = ::recv(sock,(char*)_rcv_queue.wr_buf(),_rcv_queue.wr_size(),MSG_NOSIGNAL);
				else
					return retcode;
			}
		}

		if (retcode >  0)
			success_recv();
	}

	if (0 == retcode)
		return -1;//EOF 

	return retcode;
}

/**
* \brief         拆包
* \param in       输入数据
* \param nPacketLen   输入时：整个数据包长度,包括PH_LEN
* \param out       输出数据
* \return         拆包以后的数据包长度
*/
DWORD LSocket::packetUnpack(BYTE *in,const DWORD nPacketLen,BYTE *out)
{
	//Zebra::logger->debug("LSocket::packetUnpack");
	if( nPacketLen > 10000 )
	{
		int iiii  = 0;
	}
	DWORD nRecvCmdLen = nPacketLen - PH_LEN;

	DWORD head = (*(DWORD*)(in + PACKHEADSIZE));
	//数据包压缩过
	if (PACKET_ZIP == ((*(DWORD*)(in + PACKHEADSIZE)) & PACKET_ZIP))
	{
		uLong nUnzipLen = MAX_DATASIZE;

		int retcode = uncompress(out,&nUnzipLen,&(in[PH_LEN + PACKHEADSIZE]),nRecvCmdLen);
		if (Z_OK != retcode)
		{
			printf("LSocket::packetUnpack %d.",retcode);
		}
		//返回得到的字节数
		//		Zebra::logger->debug16("1收到数据：", out, nUnzipLen);
		return nUnzipLen;
	}
	else
	{
		bcopy(&(in[PH_LEN + PACKHEADSIZE]),out,nRecvCmdLen,nRecvCmdLen);
		//		Zebra::logger->debug16("2收到数据：", out, nRecvCmdLen );
		return nRecvCmdLen;
	}
}

typedef  long          fd_mask;

#define  NBBY          8    /* number of bits in a byte */
#define  NFDBITS          (sizeof (fd_mask) * NBBY)  /* bits per mask */
#define  howmany(x,y)  (((x)+((y)-1))/(y))

extern "C"{

	int poll (struct pollfd *fds,unsigned int nfds,int timeout)
	{
		int max_fd = 0;
		fd_set *read_fds,*write_fds,*except_fds;
		struct timeval tv = { timeout / 1000,(timeout % 1000) * 1000 };

		for (DWORD i = 0; i < nfds; ++i)
		{
			if (fds[i].fd > max_fd)
			{
				max_fd = fds[i].fd;
			}
		}

		size_t fds_size = howmany (max_fd + 1,NFDBITS) * sizeof (fd_mask);

		read_fds = (fd_set *) calloc (fds_size,1);
		write_fds = (fd_set *) calloc (fds_size,1);
		except_fds = (fd_set *) calloc (fds_size,1);

		if (!read_fds || !write_fds || !except_fds)
		{
			errno = EINVAL;  /* According to SUSv3. */
			if (NULL != read_fds) free(read_fds);
			if (NULL != write_fds) free(write_fds);
			if (NULL != except_fds) free(except_fds);
			return -1;
		}

		for (DWORD i = 0; i < nfds; ++i)
		{
			fds[i].revents = 0;
			if (fds[i].events & POLLIN) FD_SET(fds[i].fd,read_fds);
			if (fds[i].events & POLLOUT) FD_SET(fds[i].fd,write_fds);
			if (fds[i].events & POLLPRI) FD_SET(fds[i].fd,except_fds);
		}

		int ret = select (0,read_fds,write_fds,except_fds,timeout < 0 ? NULL : &tv);

		if (ret > 0)
		{
			for (DWORD i = 0; i < nfds; ++i)
			{
				if (FD_ISSET(fds[i].fd,read_fds)) fds[i].revents |= POLLIN;
				if (FD_ISSET(fds[i].fd,write_fds)) fds[i].revents |= POLLOUT;
				if (FD_ISSET(fds[i].fd,except_fds)) fds[i].revents |= POLLPRI;
			}
		}

		free(read_fds);
		free(write_fds);
		free(except_fds);
		return ret;
	}

}

int WaitRecvAll( struct pollfd *fds,unsigned int nfds,int timeout )
{
	int ret = 0;
	DWORD time = GetTickCount();
	do 
	{
		for(DWORD i = 0;i < nfds; i++)
		{
			LSocket* pSock = fds[i].pSock;
			if( pSock )
			{
				int retcode = pSock->WaitRecv( false );
				if( retcode == -1 )
				{
					fds[i].events = POLLPRI;
					fds[i].revents = POLLPRI;
					ret ++;
				}
				else if( retcode > 0 )
				{
					fds[i].events = POLLIN;
					fds[i].revents = POLLIN;
					ret ++;
				}
				else
				{
					fds[i].events = 0;
					fds[i].revents = 0;
				}
				retcode = pSock->WaitSend( false );
				if( retcode == -1 )
				{
					fds[i].events |= POLLPRI;
					fds[i].revents |= POLLPRI;
				}
				else if( retcode == 1 )
				{
					fds[i].events |= POLLOUT;
					fds[i].revents |= POLLOUT;
					ret ++;
				}
			}
		}
		if( ret > 0 ) return ret;
		Sleep(5);
	} while( GetTickCount() - time < timeout * 1000 );
	return ret;
}