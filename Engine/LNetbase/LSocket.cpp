/**
 * \file
 * \version  $Id: LSocket.cpp  $
 * \author  
 * \date 
 * \brief 实现类zSocket
 *
 * 
 */

#include <errno.h>
#include <unistd.h>
#include <cstdlib>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <zlib.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <iostream>

#include "LSocket.h"

/**
 * 构造函数
 * 模板偏特化
 * 动态分配内存的缓冲区，大小可以随时扩展
 */
template <>
t_BufferCmdQueue::ByteBuffer()
	: isClient(false), _maxSize(trunkSize+startpos), _offPtr(startpos), _currPtr(startpos), _buffer(_maxSize) { }

/**
 * 构造函数
 * 模板偏特化
 * 静态数组的缓冲区，大小不能随时改变
 */
template <>
t_StackCmdQueue::ByteBuffer()
	: isClient(false), _maxSize(PACKET_ZIP_BUFFER+startpos), _offPtr(startpos), _currPtr(startpos) { }

/**
 * \brief 构造函数，初始化对象
 * \param sock 套接口
 * \param addr 地址
 * \param compress 底层数据传输是否支持压缩
 */
LSocket::LSocket(
		const int sock, 
		const struct sockaddr_in *addr, 
		const bool compress
		)
{
	assert(-1 != sock);

	isClient = false;
	this->sock = sock;
	bzero(&this->addr, sizeof(struct sockaddr_in));
	if (NULL == addr) 
	{
		socklen_t len = sizeof(struct sockaddr);
		getpeername(this->sock, (struct sockaddr *)&this->addr, &len);
	}
	else 
	{
		bcopy(addr, &this->addr, sizeof(struct sockaddr_in));
	}
	bzero(&this->local_addr, sizeof(struct sockaddr_in));
	{
		socklen_t len = sizeof(struct sockaddr_in);
		getsockname(this->sock, (struct sockaddr *)&this->local_addr, &len);
	}

	fcntl(this->sock, F_SETFD,
			fcntl(this->sock, F_GETFD, 0)
			| FD_CLOEXEC);
	setNonblock();

	rd_msec = T_RD_MSEC;
	wr_msec = T_WR_MSEC;
	_rcv_raw_size = 0;
	_current_cmd = 0; 

	set_flag(INCOMPLETE_READ | INCOMPLETE_WRITE);
	if (compress)
		bitmask |= PACKET_ZIP;
}
void LSocket::setClient() 
{ 
	isClient = true; _rcv_queue.isClient = true; _snd_queue.isClient = true; _enc_queue.isClient = true; 
}

/**
 * \brief 析构函数
 */
LSocket::~LSocket()
{
	::shutdown(sock, SHUT_RDWR);
	TEMP_FAILURE_RETRY(::close(sock));

	sock = -1;
}

#define success_unpack() \
	do { \
		if (_rcv_raw_size >= packetMinSize()/* && _rcv_queue.rd_size() >= packetMinSize()*/) \
		{ \
			unsigned int nRecordLen = packetSize(_rcv_queue.rd_buf()); \
			if (_rcv_raw_size >= nRecordLen && nRecordLen >= PH_LEN/* && _rcv_queue.rd_size() >= nRecordLen*/) \
			{ \
				int retval = packetUnpack(_rcv_queue.rd_buf(), nRecordLen, (unsigned char *)pstrCmd); \
				_rcv_queue.rd_flip(nRecordLen); \
				_rcv_raw_size -= nRecordLen; \
				return retval; \
			} \
		} \
	} while(0)

/**
 * \brief 接收指令到缓冲区
 * \param pstrCmd 指令缓冲区
 * \param nCmdLen 指令缓冲区的大小
 * \param wait 当套接口数据没有准备好的时候，是否需要等待
 * \return 实际接收的指令大小
 * 			返回-1，表示接收错误
 * 			返回0，表示接收超时
 * 			返回整数，表示实际接收的字节数
 */
int LSocket::recvToCmd(void *pstrCmd, const int nCmdLen, const bool wait)
{
	//够一个完整的纪录，不需要向套接口接收数据
	success_unpack();

	do {
		int retval = recvToBuf();
		if (-1 == retval || (0 == retval && !wait))
			return retval;

		//够一个完整的纪录，不需要向套接口接收数据
		success_unpack();
	} while(true);

	return 0;
}

/**
 * \brief 向套接口发送原始数据，没有打包的数据，一般发送数据的时候需要加入额外的包头
 * \param pBuffer 待发送的原始数据
 * \param nSize 待发送的原始数据大小
 * \return 实际发送的字节数
 * 			返回-1，表示发送错误
 * 			返回0，表示发送超时
 * 			返回整数，表示实际发送的字节数
 */
int LSocket::sendRawData(const void *pBuffer, const int nSize)
{
	int retcode = 0; 
	if (isset_flag(INCOMPLETE_WRITE))
	{
		clear_flag(INCOMPLETE_WRITE);
		goto do_select;
	}

	retcode = TEMP_FAILURE_RETRY(::send(sock, pBuffer, nSize, MSG_NOSIGNAL));
	if (retcode == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
	{
do_select:
		retcode = waitForWrite();
		if (1 == retcode)
			retcode = TEMP_FAILURE_RETRY(::send(sock, pBuffer, nSize, MSG_NOSIGNAL));
		else
			return retcode;
	}

	if (retcode > 0 && retcode < nSize)
			set_flag(INCOMPLETE_WRITE);

	return retcode;
}

/**
 * \brief 发送指定字节数的原始数据，忽略超时，无限制时间的发送，直到发送完毕或者发送失败
 * \param pBuffer 待发送的原始数据
 * \param nSize 待发送的原始数据大小
 * \return 发送数据是否成功
 */
bool LSocket::sendRawDataIM(const void *pBuffer, const int nSize)
{
	if (NULL == pBuffer || nSize <= 0)
		return false;

	int offset = 0;
	do
	{
		int retcode = sendRawData(&((char *)pBuffer)[offset], nSize - offset);
		if (-1 == retcode)
		{
			return false;
		}
		offset += retcode;
	}
	while(offset < nSize);

	return (offset == nSize);
}

/**
 * \brief 发送指令
 * \param pstrCmd 待发送的内容
 * \param nCmdLen 待发送内容的大小
 * \param buffer 是否需要缓冲
 * \return 发送是否成功
 */
bool LSocket::sendCmd(const void *pstrCmd, const int nCmdLen, const bool buffer)
{
	if (NULL == pstrCmd || nCmdLen <= 0)
		return false;
	bool retval = true;
	if (buffer)
	{
		t_StackCmdQueue _raw_queue;
		packetAppend(pstrCmd, nCmdLen, _raw_queue);
		mutex.lock();
		_snd_queue.put(_raw_queue.rd_buf(), _raw_queue.rd_size());
		_current_cmd = _raw_queue.rd_size();
		mutex.unlock();
	}
	else
	{
		t_StackCmdQueue _raw_queue;
		_raw_queue.isClient = isClient;
		packetAppend(pstrCmd, nCmdLen, _raw_queue);
		mutex.lock();
		retval = sendRawDataIM(_raw_queue.header(), _raw_queue.headerLen());
		mutex.unlock();
	}
	return retval;
}

/**
 * \brief 发送原始指令数据，不做封包动作
 * \param pstrCmd 待发送的内容
 * \param nCmdLen 待发送内容的大小
 * \param buffer 是否需要缓冲
 * \return 发送是否成功
 */
bool LSocket::sendCmdNoPack(const void *pstrCmd, const int nCmdLen, const bool buffer)
{
	if (NULL == pstrCmd || nCmdLen <= 0)
		return false;
	bool retval = true;
	if (buffer)
	{
		mutex.lock();
		_snd_queue.put((unsigned char *)pstrCmd, nCmdLen);
		_current_cmd = nCmdLen; 
		mutex.unlock();
	}
	else
	{
		t_StackCmdQueue _raw_queue;
		_raw_queue.isClient = isClient;
		_raw_queue.put((unsigned char *)pstrCmd, nCmdLen);
		if (need_enc())
		{
			packetPackEnc(_raw_queue, _raw_queue.rd_size());
			mutex.lock();
			retval = sendRawDataIM(_raw_queue.header(), _raw_queue.headerLen());
			mutex.unlock();
		}
		else
		{
			mutex.lock();
			retval = sendRawDataIM(_raw_queue.header(), _raw_queue.headerLen());
			mutex.unlock();
		}
	}
	return retval;
}

bool LSocket::sync()
{
	if(need_enc())
	{
		unsigned int nSize = 0, current_cmd = 0, offset = 0;
		mutex.lock();
		if (_snd_queue.rd_ready())
		{
			nSize = _snd_queue.rd_size();
			current_cmd = _current_cmd;
			offset = _enc_queue.rd_size();
			if(_enc_queue.isClient)
			{
				_enc_queue.put(_snd_queue.rd_buf(), 4);
				offset += 4;
			}
			_enc_queue.put(_snd_queue.rd_buf(), nSize);
			_snd_queue.rd_flip(nSize);
			_current_cmd = 0;
		}
		mutex.unlock();
		if (nSize)
		{
			packetPackEnc(_enc_queue, current_cmd, offset);
			if(_enc_queue.isClient)
			{
				(*(unsigned int *)(&(_enc_queue.rd_buf()[offset-4]))) = _enc_queue.rd_size()-offset;
				(*(unsigned int *)(&(_enc_queue.rd_buf()[offset-4]))) |= 0x80000000; 
			}
		}
		if (_enc_queue.rd_ready())
		{
			int retcode = 0;
			if(_enc_queue.isClient)
				retcode = sendRawData_NoPoll(_enc_queue.rd_buf(), _enc_queue.rd_size());
			else
				retcode = sendRawData_NoPoll(_enc_queue.header(), _enc_queue.headerLen());
			if (retcode > 0)
			{
				_enc_queue.rd_flip(retcode);
			}
			else if (-1 == retcode)
			{
				return false;
			}
		}
	}
	else
	{
		unsigned int nSize = 0, offset = 0;
		mutex.lock();
		if (_snd_queue.rd_ready())
		{
			nSize = _snd_queue.rd_size();
			offset = _enc_queue.rd_size();
			if(_enc_queue.isClient)
			{
				_enc_queue.put(_snd_queue.rd_buf(), 4);
				offset += 4;
			}
			_enc_queue.put(_snd_queue.rd_buf(), _snd_queue.rd_size());
			_snd_queue.rd_flip(_snd_queue.rd_size());
		}
		mutex.unlock();
		if (nSize)
		{
			if(_enc_queue.isClient)
			{
				(*(unsigned int *)(&(_enc_queue.rd_buf()[offset-4]))) = _enc_queue.rd_size()-offset;
				(*(unsigned int *)(&(_enc_queue.rd_buf()[offset-4]))) |= 0x80000000; 
			}
		}
		if (_enc_queue.rd_ready())
		{
			int retcode = 0;
			if(_enc_queue.isClient)
				retcode = sendRawData_NoPoll(_enc_queue.rd_buf(), _enc_queue.rd_size());
			else
				retcode = sendRawData_NoPoll(_enc_queue.header(), _enc_queue.headerLen());
			if (retcode > 0)
			{
				_enc_queue.rd_flip(retcode);
			}
			else if (-1 == retcode)
			{
				return false;
			}
		}
	}
	return true;
}

void LSocket::force_sync()
{
	if(need_enc())
	{
		unsigned int nSize = 0, current_cmd = 0, offset = 0;
		mutex.lock();
		if (_snd_queue.rd_ready())
		{
			nSize = _snd_queue.rd_size();
			current_cmd = _current_cmd;
			offset = _enc_queue.rd_size();
			_enc_queue.put(_snd_queue.rd_buf(), nSize);
			_snd_queue.rd_flip(nSize);
			_current_cmd = 0;
		}
		mutex.unlock();
		if (nSize)
			packetPackEnc(_enc_queue, current_cmd, offset);
		if (_enc_queue.rd_ready())
		{
			sendRawDataIM(_enc_queue.header(), _enc_queue.headerLen());
			_enc_queue.reset();
		}
	}
	else
	{
		mutex.lock();
		if (_snd_queue.rd_ready())
		{
			_enc_queue.put(_snd_queue.rd_buf(), _snd_queue.rd_size());
			_snd_queue.rd_flip(_snd_queue.rd_size());
		}
		mutex.unlock();
		if (_enc_queue.rd_ready())
		{
			sendRawDataIM(_enc_queue.header(), _enc_queue.headerLen());
			_enc_queue.reset();
		}
	}
}

/**
 * \brief 检查套接口准备好读取操作
 * \return 是否成功
 * 			-1，表示操作失败
 * 			0，表示操作超时
 * 			1，表示等待成功，套接口已经有数据准备读取
 */
int LSocket::checkIOForRead()
{
	struct pollfd pfd;

	pfd.fd = sock;
	pfd.events = POLLIN | POLLERR | POLLPRI;
	pfd.revents = 0;

	int retcode = TEMP_FAILURE_RETRY(::poll(&pfd, 1, 0));
	if (retcode > 0 && 0 == (pfd.revents & POLLIN))
		retcode = -1;

	return retcode;
}

/**
 * \brief 等待套接口准备好写入操作
 * \return 是否成功
 * 			-1，表示操作失败
 * 			0，表示操作超时
 * 			1，表示等待成功，套接口已经可以写入数据
 */
int LSocket::checkIOForWrite()
{
	struct pollfd pfd;

	pfd.fd = sock;
	pfd.events = POLLOUT | POLLERR | POLLPRI;
	pfd.revents = 0;

	int retcode = TEMP_FAILURE_RETRY(::poll(&pfd, 1, 0));
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
	int s;
	struct ifreq ifr;
	static const char *none_ip = "0.0.0.0";

	if (NULL == ifName)
		return none_ip;
	s = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (-1 == s) 
		return none_ip;

	bzero(ifr.ifr_name, sizeof(ifr.ifr_name));
	strncpy(ifr.ifr_name, ifName, sizeof(ifr.ifr_name) - 1);
	if (-1 == ioctl(s, SIOCGIFADDR, &ifr)) 
	{
		TEMP_FAILURE_RETRY(::close(s));
		return none_ip;
	}

	TEMP_FAILURE_RETRY(::close(s));
	return inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr);
}

/*		unsigned int size = _rcv_queue.rd_size() - _rcv_raw_size - ((_rcv_queue.rd_size() - _rcv_raw_size) % 8); \
		if(size) \
		{ \
			enc.encdec(&_rcv_queue.rd_buf()[_rcv_raw_size], size, false); \
			_rcv_raw_size += size; \
		} \ */

#define success_recv() \
	do { \
		if ((unsigned int)retcode < _rcv_queue.wr_size()) \
		{ \
			set_flag(INCOMPLETE_READ); \
		} \
		_rcv_queue.wr_flip(retcode); \
		_rcv_raw_size += retcode; \
	} while(0)

/**
 * \brief 	接收数据到缓冲区，保证在调用这个函数之前套接口准备好了接收，也就是使用poll轮询过
 * 			如果是加密包需要先解密到解密缓冲区
 * \return 	实际接收字节数
 * 			返回-1，表示接收错误
 * 			返回0，表示接收超时
 * 			返回整数，不加密时表示实际接收的字节数,加密时返回解密到的字节数
 */
int LSocket::recvToBuf_NoPoll()
{
	int retcode =0; 
	if(need_enc())
	{
		_rcv_queue.wr_reserve(MAX_DATABUFFERSIZE);
		retcode = TEMP_FAILURE_RETRY(::recv(sock, _rcv_queue.wr_buf(), _rcv_queue.wr_size(), MSG_NOSIGNAL));
		if (retcode == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
			return 0;//should retry

		if (retcode > 0) 
			success_recv();
	}
	else
	{
		_rcv_queue.wr_reserve(MAX_DATABUFFERSIZE);
		retcode = TEMP_FAILURE_RETRY(::recv(sock, _rcv_queue.wr_buf(), _rcv_queue.wr_size(), MSG_NOSIGNAL));
		if (retcode == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
			return 0;//should retry

		if (retcode > 0)
			success_recv();
	}

	if (0 == retcode)
		return -1;//EOF 

	return retcode;
}

/**
 * \brief 接收指令到缓冲区，不从套接口接收指令，只是把接收缓冲的数据解包
 * \param pstrCmd 指令缓冲区
 * \param nCmdLen 指令缓冲区的大小
 * \return 实际接收的指令大小
 * 			返回-1，表示接收错误
 * 			返回0，表示接收超时
 * 			返回整数，表示实际接收的字节数
 */
int LSocket::recvToCmd_NoPoll(void *pstrCmd, const int nCmdLen)
{
	//够一个完整的纪录，不需要向套接口接收数据
	success_unpack(); 	
	return 0;
}

bool LSocket::connect()
{
	//retcode = TEMP_FAILURE_RETRY(::connect(sock, (struct sockaddr *) &addr, sizeof(addr)));
	int retcode = ::connect(sock, (struct sockaddr *) &addr, sizeof(addr));
	if (-1 == retcode)
	{   
		if(errno != EINPROGRESS)
		{   
			TEMP_FAILURE_RETRY(::close(sock));
			return false;
		}   
	}   
	return true;
}
int LSocket::recvToBuf_Http(char* buf)
{
	int retcode =0; 
	retcode = TEMP_FAILURE_RETRY(::recv(sock, buf, MAX_DATABUFFERSIZE, MSG_NOSIGNAL));
	if (retcode == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
		return 0;//should retry

	if (0 == retcode)
		return -1;//EOF 

	return retcode;
}

/**
 * \brief 向套接口发送原始数据，没有打包的数据，一般发送数据的时候需要加入额外的包头，保证在调用这个函数之前套接口准备好了接收，也就是使用poll轮询过
 * \param pBuffer 待发送的原始数据
 * \param nSize 待发送的原始数据大小
 * \return 实际发送的字节数
 * 			返回-1，表示发送错误
 * 			返回0，表示发送超时
 * 			返回整数，表示实际发送的字节数
 */
int LSocket::sendRawData_NoPoll(const void *pBuffer, const int nSize)
{
	int retcode = TEMP_FAILURE_RETRY(::send(sock, pBuffer, nSize, MSG_NOSIGNAL));
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
	int fd_flags;
	int nodelay = 1;

	if (::setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void *)&nodelay, sizeof(nodelay)))
		return false;

	fd_flags = ::fcntl(sock, F_GETFL, 0);

#if defined(O_NONBLOCK)
	fd_flags |= O_NONBLOCK;
#elif defined(O_NDELAY)
	fd_flags |= O_NDELAY;
#elif defined(FNDELAY)
	fd_flags |= O_FNDELAY;
#else
	/* XXXX: this breaks things, but an alternative isn't obvious...*/
	return false;
#endif

	if (::fcntl(sock, F_SETFL, fd_flags) == -1) 
		return false;

	return true;
}

/**
 * \brief 等待套接口准备好读取操作
 * \return 是否成功
 * 			-1，表示操作失败
 * 			0，表示操作超时
 * 			1，表示等待成功，套接口已经有数据准备读取
 */
int LSocket::waitForRead()
{
	struct pollfd pfd;

	pfd.fd = sock;
	pfd.events = POLLIN | POLLERR | POLLPRI;
	pfd.revents = 0;

	int retcode = TEMP_FAILURE_RETRY(::poll(&pfd, 1, rd_msec));
	if (retcode > 0 && 0 == (pfd.revents & POLLIN))
		retcode = -1;

	return retcode;
}

/**
 * \brief 等待套接口准备好写入操作
 * \return 是否成功
 * 			-1，表示操作失败
 * 			0，表示操作超时
 * 			1，表示等待成功，套接口已经可以写入数据
 */
int LSocket::waitForWrite()
{
	struct pollfd pfd;

	pfd.fd = sock;
	pfd.events = POLLOUT | POLLERR | POLLPRI;
	pfd.revents = 0;

	int retcode = TEMP_FAILURE_RETRY(::poll(&pfd, 1, wr_msec));
	if (retcode > 0 && 0 == (pfd.revents & POLLOUT))
		retcode = -1;

	return retcode;
}

/**
 * \brief 	接收数据到缓冲区
 * 			如果是加密包需要解密到解密缓冲区
 * \return 	实际接收字节数
 * 			返回-1，表示接收错误
 * 			返回0，表示接收超时
 * 			返回整数，不加密包表示实际接收的字节数,加密包返回解密后实际可用的字节数
 */
int LSocket::recvToBuf()
{
	int retcode = 0;

	if(need_enc())
	{
		if (isset_flag(INCOMPLETE_READ))
		{
			clear_flag(INCOMPLETE_READ);
			goto do_select_enc;
		}

		_rcv_queue.wr_reserve(MAX_DATABUFFERSIZE);
		retcode = TEMP_FAILURE_RETRY(::recv(sock, _rcv_queue.wr_buf(), _rcv_queue.wr_size(), MSG_NOSIGNAL));
		if (retcode == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
		{
do_select_enc:
			retcode = waitForRead();
			if (1 == retcode)
				retcode = TEMP_FAILURE_RETRY(::recv(sock, _rcv_queue.wr_buf(), _rcv_queue.wr_size(), MSG_NOSIGNAL));
			else
				return retcode;
		}

		if (retcode > 0) 
			success_recv();
	}
	else
	{
		if (isset_flag(INCOMPLETE_READ))
		{
			clear_flag(INCOMPLETE_READ);
			goto do_select;
		}
		_rcv_queue.wr_reserve(MAX_DATABUFFERSIZE);
		retcode = TEMP_FAILURE_RETRY(::recv(sock, _rcv_queue.wr_buf(), _rcv_queue.wr_size(), MSG_NOSIGNAL));
		if (retcode == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
		{
do_select:
			retcode = waitForRead();
			if (1 == retcode)
				retcode = TEMP_FAILURE_RETRY(::recv(sock, _rcv_queue.wr_buf(), _rcv_queue.wr_size(), MSG_NOSIGNAL));
			else
				return retcode;
		}

		if (retcode > 0)
			success_recv();
	}

	if (0 == retcode)
		return -1;//EOF 

	return retcode;
}

/**
 * \brief 				拆包
 * \param in 			输入数据
 * \param nPacketLen 	输入时：整个数据包长度,包括PH_LEN
 * \param out 			输出数据
 * \return 				打包以后的数据包长度
 */
unsigned int LSocket::packetUnpack(unsigned char *in, const unsigned int nPacketLen, unsigned char *out)
{
	unsigned int nRecvCmdLen = nPacketLen - PH_LEN;
	if (PACKET_DEC == ((*(unsigned int *)in) & PACKET_DEC)) 
	{
		if(nRecvCmdLen%8 == 0)
			enc.encdec(&(in[PH_LEN]), nRecvCmdLen, false); 
		else
		{}
	}
	//数据包压缩过
	if (PACKET_ZIP == ((*(unsigned int*)in) & PACKET_ZIP))
	{
		uLong nUnzipLen = MAX_DATASIZE;
//		in[PH_LEN] = 0x78;
//		in[PH_LEN+1] = 0x9c;

		int retcode = uncompress(out, &nUnzipLen, &(in[PH_LEN]), nRecvCmdLen);
		switch(retcode)
		{
			case Z_OK:
				break;
			case Z_MEM_ERROR:
				break;
			case Z_BUF_ERROR:
				break;
			case Z_DATA_ERROR:
				break;
		}
		//返回得到的字节数
		return nUnzipLen;
	}
	else
	{
		bcopy(&(in[PH_LEN]), out, nRecvCmdLen);
		return nRecvCmdLen;
	}
}

