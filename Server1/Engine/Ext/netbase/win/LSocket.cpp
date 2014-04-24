/**
* \brief ʵ����zSocket
*
* 
*/
#include "LSocket.h"
#include <assert.h>
#include "LIocp.h"
/**
* ���캯��
* ģ��ƫ�ػ�
* ��̬�����ڴ�Ļ�����,��С������ʱ��չ
*/
template <>
t_BufferCmdQueue::ByteBuffer()
: _maxSize(trunkSize),_offPtr(0),_currPtr(0),_buffer(_maxSize) { }

/**
* ���캯��
* ģ��ƫ�ػ�
* ��̬����Ļ�����,��С������ʱ�ı�
*/
template <>
t_StackCmdQueue::ByteBuffer()
: _maxSize(PACKET_ZIP_BUFFER),_offPtr(0),_currPtr(0) { }

/**
* \brief ���캯��,��ʼ������
* \param sock �׽ӿ�
* \param addr ��ַ
* \param compress �ײ����ݴ����Ƿ�֧��ѹ��
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
	//	Zebra::logger->debug("�½����� %0.8X", (DWORD)pTask );

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

	// [ranqd]��������֪ͨ�¼�
	//	m_hRecvEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	if( useIocp )
	{
		// [ranqd] ��IO����ɶ˿���
		LIocp::getInstance().BindIocpPort( (HANDLE)sock, this );
	}

	set_flag(INCOMPLETE_READ | INCOMPLETE_WRITE);
	if (compress)
		bitmask |= PACKET_ZIP;
}


/**
* \brief ��������
*/
LSocket::~LSocket()
{
	//	Zebra::logger->debug("�ر��׽ӿ�����");

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
// [ranqd] Ͷ��IO������
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
	// �����ȡ����
	bresult = ReadFile(
		(HANDLE)sock,
		m_RecvBuffer.wr_buf(),
		size,
		&numRead,
		(LPOVERLAPPED)&m_ovIn );
	// ����,�ȴ�һ����Ϣ��.
	if (bresult) return;
	err = GetLastError();
	if (err == 64)//�����ѹر�
	{
		LIocp::getInstance().PostStatus(this,&m_ovIn);//���Ͷ�ȡʧ�ܵ������Ϣ
		return;
	}
	if( err != 997 )
	{
		printf( "��������ʱ�ص�I/O������������ %s",GetErrMsg(err) );
	}
}
// [ranqd] ͨ��Iocp��������
int LSocket::SendData( DWORD dwNum = 0 )
{
	BOOL    bresult;
	int     err;
	DWORD   numRead;

	// [ranqd] ��¼����������
	InterlockedExchangeAdd(&g_SendSize, (long)dwNum);

	tQueue.rd_flip( dwNum );

	m_dwSendCount += dwNum;

	int size = tQueue.rd_size();

	// û�����ݿɷ����ˣ�����
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

	// ����,�ȴ�һ����Ϣ��.
	if (bresult) return size;
	err = GetLastError();

	if( err == 64 )//�����ѹر�
	{
		LIocp::getInstance().PostStatus(this,&m_ovOut);//����д����ʧ�ܵ������Ϣ
		return -1;
	}

	if( err != 997 )
	{
		printf( "��������ʱ�ص�I/O������������ %s",GetErrMsg(err) );
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
	// [ranqd] ��¼����������
	InterlockedExchangeAdd( &g_RecvSize, (LONG)dwNum );

	m_dwRecvCount += dwNum;	
	static PACK_HEAD head;
	
	m_RecvBuffer.wr_flip( dwNum );
	m_RecvLock.lock();
	
	if( m_RecvBuffer.rd_size() == PACKHEADSIZE )
	{
		// [ranqd] �յ������ݴ�����Ҫ�ر����ӣ�Ϊ���ڵ��ԣ��ݲ��ر�����
		if(( m_RecvBuffer.rd_buf()[0] != head.Header[0] || m_RecvBuffer.rd_buf()[1] != head.Header[1]) )
		{
			printf("Iocp�յ�����İ�ͷ���ԣ� %u.%u.%u.%u : %u", addr.sin_addr.S_un.S_un_b.s_b1,addr.sin_addr.S_un.S_un_b.s_b2,addr.sin_addr.S_un.S_un_b.s_b3,addr.sin_addr.S_un.S_un_b.s_b4, addr.sin_port / 0xFF + addr.sin_port % 256 * 256);
			printf("�������ݣ�", m_RecvBuffer.rd_buf(), m_RecvBuffer.rd_size() );
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
		// [ranqd] ��ͷû�����꣬����������������
		ReadByte( PACKHEADSIZE - m_RecvBuffer.rd_size() );
		goto ret;
	}
	else
	{
		nLen = ((PACK_HEAD*)m_RecvBuffer.rd_buf())->Len;

		if( nLen  > MAX_DATASIZE )
		{
			//[ranqd] �յ��ĳ�������Ӧ�øɵ�
			m_RecvBuffer.reset();
			m_ovIn = OverlappedData( IO_Read );
			ReadByte( sizeof(PACK_HEAD) );
			goto ret;
		}
		if( m_RecvBuffer.rd_size() == nLen + PACKHEADSIZE )
		{
			// [ranqd] �յ�һ�������İ������뻺��
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
			ReadByte( PACKHEADSIZE + nLen - m_RecvBuffer.rd_size() );// ������պ��������
		}
	}
ret:
	m_RecvLock.unlock();
}

// [ranqd] �ȴ����ݽ�����ɣ�bWait ��ʾ�Ƿ�Ҫ�ȵ���ɺ�ŷ���, timeout��ʾ�ȴ���ʱʱ�䣨��λ�룩
// ����ֵ�� 0 ���ݽ��ջ�û����ɣ� -1 �����Ѿ��Ͽ�, -2 �ȴ���ʱ > 0 �Ѿ��յ������ݳ���
int LSocket::WaitRecv( bool bWait, int timeout )
{
	if( bWait )
	{
		DWORD EnterTime = GetTickCount();
		do// �ȵ����ݽ�����ɻ��߶Ͽ�����
		{
			if( _rcv_queue.rd_size() > 0 )
			{
				return _rcv_queue.rd_size();
			}
			// ��������Ƿ��ѶϿ�      
			if( sock == INVALID_SOCKET )
			{
				return -1;
			}
			::Sleep(10);

		}while( timeout == 0 || GetTickCount() - EnterTime < timeout * 1000 );

		return 0; // ��ʱ
	}
	else // ���ȴ���ֻ���ж�
	{
		if( sock == INVALID_SOCKET )
			return -1;

		return _rcv_queue.rd_size();
	}
}
// [ranqd] �ȴ����ݷ�����ɣ�bWait��ʾ�Ƿ�Ҫ�ȵ���ɺ�ŷ��أ�timeout��ʾ�ȴ���ʱʱ��(��λ��)
// ����ֵ��0 ���ݷ��ͻ�û��ɣ� -1 �����ѶϿ��� 1 ��׼���÷�������
int LSocket::WaitSend( bool bWait, int timeout )
{
	if( bWait )
	{
		DWORD EnterTime = GetTickCount();
		do// �ȵ����ݽ�����ɻ��߶Ͽ�����
		{
			if( tQueue.rd_size() == 0 )
			{
				return 1;
			}
			// ��������Ƿ��ѶϿ�      
			if( sock == INVALID_SOCKET )
			{
				return -1;
			}
			::Sleep(10);

		}while( timeout == 0 || GetTickCount() - EnterTime < timeout * 1000 );

		return 0; // ��ʱ
	}
	else // ���ȴ���ֻ���ж�
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
* \brief ����ָ�������
* \param pstrCmd ָ�����
* \param nCmdLen ָ������Ĵ�С
* \param wait ���׽ӿ�����û��׼���õ�ʱ��,�Ƿ���Ҫ�ȴ�
* \return ʵ�ʽ��յ�ָ���С
*       ����-1,��ʾ���մ���
*       ����0,��ʾ���ճ�ʱ
*       ��������,��ʾʵ�ʽ��յ��ֽ���
*/
int LSocket::recvToCmd(void *pstrCmd,const int nCmdLen,const bool wait)
{
	//Zebra::logger->debug("LSocket::recvToCmd");
	//��һ�������ļ�¼,����Ҫ���׽ӿڽ�������
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

		//��һ�������ļ�¼,����Ҫ���׽ӿڽ�������
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

// [ranqd]  ���·�װ�ײ㷢�ͺ�����������������з���Ĳ���
int  LSocket::Send(const SOCKET sock, const void* pBuffer, const int nLen,int flags)
{
	// [ranqd] �������͵����ݣ��Ƿ���Ϸ����ʽ

	//PACK_HEAD* phead;
	//int offset = 0;
	//do 
	//{
	//	phead = (PACK_HEAD*)((DWORD)pBuffer + offset);
	//	if( phead->Header[0] != 0xAA || phead->Header[1] != 0xDD )
	//	{
	//		Zebra::logger->debug("����δ������ݣ�");
	//		return nLen;
	//	}
	//	offset += phead->Len + PACKHEADSIZE;
	//} while(offset < nLen );
	tQueue.Lock();
	tQueue.put((const BYTE*)pBuffer, nLen);
	tQueue.UnLock();

	m_dwMySendCount += nLen;

	/*	Zebra::logger->debug("Ŀ��˿ڣ� ( %u.%u.%u.%u : %u -> %u.%u.%u.%u : %u )",
	local_addr.sin_addr.S_un.S_un_b.s_b1,local_addr.sin_addr.S_un.S_un_b.s_b2,local_addr.sin_addr.S_un.S_un_b.s_b3,local_addr.sin_addr.S_un.S_un_b.s_b4, local_addr.sin_port / 0xFF + local_addr.sin_port % 256 * 256,
	addr.sin_addr.S_un.S_un_b.s_b1,addr.sin_addr.S_un.S_un_b.s_b2,addr.sin_addr.S_un.S_un_b.s_b3,addr.sin_addr.S_un.S_un_b.s_b4, addr.sin_port / 0xFF + addr.sin_port % 256 * 256);
	Zebra::logger->debug16( "����", (const BYTE*)tQueue.rd_buf(), tQueue.rd_size());

	Zebra::logger->debug("%s : %u -> %s : %u ��װ�ܳ��ȣ� %u",getLocalIP(), getLocalPort(), getIP(), getPort(),m_dwMySendCount);
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
	//	Zebra::logger->debug("���ͽ���� %d ��ͷ�� %d", SendLen, ((PACK_HEAD*)tQueue.rd_buf())->Len);
	return SendLen;
}
/**
* \brief ���׽ӿڷ���ԭʼ����,û�д��������,һ�㷢�����ݵ�ʱ����Ҫ�������İ�ͷ
* \param pBuffer �����͵�ԭʼ����
* \param nSize �����͵�ԭʼ���ݴ�С
* \return ʵ�ʷ��͵��ֽ���
*       ����-1,��ʾ���ʹ���
*       ����0,��ʾ���ͳ�ʱ
*       ��������,��ʾʵ�ʷ��͵��ֽ���
*/
int LSocket::sendRawData(const void *pBuffer,const int nSize)
{
	//fprintf(stderr,"LSocket::sendRawData\n");

	/*if(((Cmd::stNullUserCmd *)pBuffer)->byCmd == 5 && ((Cmd::stNullUserCmd *)pBuffer)->byParam == 55)
	{
	Cmd::stMapDataMapScreenUserCmd * tt = (Cmd::stMapDataMapScreenUserCmd *)(pBuffer);
	WORD _size = tt->mdih.size;
	fprintf(stderr,"������Ϣ\n");
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
* \brief ����ָ���ֽ�����ԭʼ����,���Գ�ʱ,������ʱ��ķ���,ֱ��������ϻ��߷���ʧ��
* \param pBuffer �����͵�ԭʼ����
* \param nSize �����͵�ԭʼ���ݴ�С
* \return ���������Ƿ�ɹ�
*/
bool LSocket::sendRawDataIM(const void *pBuffer,const int nSize)
{
	//Zebra::logger->debug("LSocket::sendRawDataIM");
	if (NULL == pBuffer || nSize <= 0)
		return false;
	int retcode = sendRawData(pBuffer,nSize);
	if (-1 == retcode)
	{
		printf("�ͻ��˶Ͽ� -1");  
		return false;
	}
	return true;
}

/**
* \brief ����ָ��
* \param pstrCmd �����͵�����
* \param nCmdLen ���������ݵĴ�С
* \param buffer �Ƿ���Ҫ����
* \return �����Ƿ�ɹ�
*/
bool LSocket::sendCmd(const void *pstrCmd,const int nCmdLen,const bool buffer)
{
	//Zebra::logger->debug("LSocket::sendCmd");
	if (NULL == pstrCmd || nCmdLen <= 0)
		return false;
	bool retval = true;
	//	Zebra::logger->debug("����Ŀ�꣺%s : %u", getIP(), getPort());
	//	Zebra::logger->debug16("ϣ�����ͣ�", (BYTE*)pstrCmd, nCmdLen);
	if (buffer)
	{
		// ֱ�Ӱ����ݼ���ѹ�������ŵ����ͻ���_enc_queue
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
* \brief ����ԭʼָ������,����������� [ranqd] ����ԭʼ���ݵ�ת��
* \param pstrCmd �����͵�����
* \param nCmdLen ���������ݵĴ�С
* \param buffer �Ƿ���Ҫ����
* \return �����Ƿ�ɹ�
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
		_enc_queue.put((BYTE*)&head, sizeof(head)); // NoPackҲҪ���ͷ
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
* \brief ����׽ӿ�׼���ö�ȡ����
* \return �Ƿ�ɹ�
*       -1,��ʾ����ʧ��
*       0,��ʾ������ʱ
*       1,��ʾ�ȴ��ɹ�,�׽ӿ��Ѿ�������׼����ȡ
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
* \brief �ȴ��׽ӿ�׼����д�����
* \return �Ƿ�ɹ�
*       -1,��ʾ����ʧ��
*       0,��ʾ������ʱ
*       1,��ʾ�ȴ��ɹ�,�׽ӿ��Ѿ�����д������
*/
int LSocket::checkIOForWrite()
{
	//Zebra::logger->debug("LSocket::checkIOForWrite");
	if( m_bUseIocp ) // ʹ��Iocpʱ��ʱ������д������
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
* \brief �����������Ʊ�Ż�ȡָ��������IP��ַ
* \param ifName ��Ҫȡ��ַ����������
* \return ��ȡ��ָ��������IP��ַ
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
* \brief   �������ݵ�������,��֤�ڵ����������֮ǰ�׽ӿ�׼�����˽���,Ҳ����ʹ��poll��ѯ��
*       ����Ǽ��ܰ���Ҫ�Ƚ��ܵ����ܻ�����
* \return   ʵ�ʽ����ֽ���
*       ����-1,��ʾ���մ���
*       ����0,��ʾ���ճ�ʱ
*       ��������,������ʱ��ʾʵ�ʽ��յ��ֽ���,����ʱ���ؽ��ܵ����ֽ���
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
			// [ranqd] Ҫ����ͬʱ���������ܰ�
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
* \brief ����ָ�������,�����׽ӿڽ���ָ��,ֻ�ǰѽ��ջ�������ݽ��
* \param pstrCmd ָ�����
* \param nCmdLen ָ������Ĵ�С
* \return ʵ�ʽ��յ�ָ���С
*       ����-1,��ʾ���մ���
*       ����0,��ʾ���ճ�ʱ
*       ��������,��ʾʵ�ʽ��յ��ֽ���
*/
int LSocket::recvToCmd_NoPoll(void *pstrCmd,const int nCmdLen)
{
	//��һ�������ļ�¼,����Ҫ���׽ӿڽ�������
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
				printf("�������İ���");
			}
			
		} 
	} while(0);
	return 0;
}

/**
* \brief ���׽ӿڷ���ԭʼ����,û�д��������,һ�㷢�����ݵ�ʱ����Ҫ�������İ�ͷ,��֤�ڵ����������֮ǰ�׽ӿ�׼�����˽���,Ҳ����ʹ��poll��ѯ��
* \param pBuffer �����͵�ԭʼ����
* \param nSize �����͵�ԭʼ���ݴ�С
* \return ʵ�ʷ��͵��ֽ���
*       ����-1,��ʾ���ʹ���
*       ����0,��ʾ���ͳ�ʱ
*       ��������,��ʾʵ�ʷ��͵��ֽ���
*/
int LSocket::sendRawData_NoPoll(const void *pBuffer,const int nSize)
{

	/*fprintf(stderr,"LSocket::sendRawData_NoPoll\n");


	if(((Cmd::stNullUserCmd *)pBuffer)->byCmd == 5 && ((Cmd::stNullUserCmd *)pBuffer)->byParam == 55)
	{
	Cmd::stMapDataMapScreenUserCmd * tt = (Cmd::stMapDataMapScreenUserCmd *)(pBuffer);
	WORD _size = tt->mdih.size;
	fprintf(stderr,"������Ϣ\n");
	}*/
	int retcode = Send(sock,(const char*)pBuffer,nSize,MSG_NOSIGNAL);
	if (retcode == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
		return 0;//should retry

	if (retcode > 0 && retcode < nSize)
		set_flag(INCOMPLETE_WRITE);

	return retcode;
}

/**
* \brief �����׽ӿ�Ϊ������ģʽ
* \return �����Ƿ�ɹ�
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
* \brief �ȴ��׽ӿ�׼���ö�ȡ����
* \return �Ƿ�ɹ�
*       -1,��ʾ����ʧ��
*       0,��ʾ������ʱ
*       1,��ʾ�ȴ��ɹ�,�׽ӿ��Ѿ�������׼����ȡ
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
* \brief �ȴ��׽ӿ�׼����д�����
* \return �Ƿ�ɹ�
*       -1,��ʾ����ʧ��
*       0,��ʾ������ʱ
*       1,��ʾ�ȴ��ɹ�,�׽ӿ��Ѿ�����д������
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
* \brief   �������ݵ�������
*       ����Ǽ��ܰ���Ҫ���ܵ����ܻ�����
* \return   ʵ�ʽ����ֽ���
*       ����-1,��ʾ���մ���
*       ����0,��ʾ���ճ�ʱ
*       ��������,�����ܰ���ʾʵ�ʽ��յ��ֽ���,���ܰ����ؽ��ܺ�ʵ�ʿ��õ��ֽ���
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
* \brief         ���
* \param in       ��������
* \param nPacketLen   ����ʱ���������ݰ�����,����PH_LEN
* \param out       �������
* \return         ����Ժ�����ݰ�����
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
	//���ݰ�ѹ����
	if (PACKET_ZIP == ((*(DWORD*)(in + PACKHEADSIZE)) & PACKET_ZIP))
	{
		uLong nUnzipLen = MAX_DATASIZE;

		int retcode = uncompress(out,&nUnzipLen,&(in[PH_LEN + PACKHEADSIZE]),nRecvCmdLen);
		if (Z_OK != retcode)
		{
			printf("LSocket::packetUnpack %d.",retcode);
		}
		//���صõ����ֽ���
		//		Zebra::logger->debug16("1�յ����ݣ�", out, nUnzipLen);
		return nUnzipLen;
	}
	else
	{
		bcopy(&(in[PH_LEN + PACKHEADSIZE]),out,nRecvCmdLen,nRecvCmdLen);
		//		Zebra::logger->debug16("2�յ����ݣ�", out, nRecvCmdLen );
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