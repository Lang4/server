/**
 * \file
 * \version  $Id: LSocket.h  $
 * \author  
 * \date 
 * \brief ����zSocket�࣬���ڶ��׽ӿڵײ���з�װ
 */

#ifndef _zSocket_h_
#define _zSocket_h_

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
#include <pthread.h>
#include <assert.h>
#include <iostream>
#include <queue>
#include <ext/pool_allocator.h>
#include <ext/mt_allocator.h>

#include "LCantCopy.h"
#include "LMutex.h"
#include "LTime.h"

#include "EncDec.h"

#include <sys/poll.h>
#include <sys/epoll.h>

class LSocket;

const unsigned int trunkSize = 64 * 1024;
#define unzip_size(zip_size) ((zip_size) * 120 / 100 + 12)
const unsigned int PACKET_ZIP_BUFFER	=	unzip_size(trunkSize - 1) + sizeof(unsigned int) + 8;	/**< ѹ����Ҫ�Ļ��� */
const unsigned int startpos = 4;

/**
 * �ֽڻ��壬�����׽ӿڽ��պͷ������ݵĻ���
 * \param _type ��������������
 */
template <typename _type>
class ByteBuffer
{
	public:
		ByteBuffer();
		inline void put(const unsigned char *buf, const unsigned int size)
		{
			wr_reserve(size);
			bcopy(buf, &_buffer[_currPtr], size);
			_currPtr += size;
		}
		inline unsigned char *wr_buf()
		{
			return &_buffer[_currPtr];
		}
		inline unsigned char *rd_buf()
		{
			return &_buffer[_offPtr];
		}
		inline bool rd_ready() const
		{
			return _currPtr > _offPtr;
		}
		inline unsigned int rd_size() const
		{
			return _currPtr - _offPtr;
		}
		inline unsigned char *header()
		{
			if(isClient)
			{
				*((unsigned int*)(&(_buffer[_offPtr-startpos]))) = rd_size(); 
				*((unsigned int*)(&(_buffer[_offPtr-startpos]))) |= 0x80000000; 
				return &_buffer[_offPtr-startpos];
			}
			return rd_buf();
		}
		inline unsigned int headerLen() const
		{
			if(isClient)
				return rd_size()+startpos;
			return rd_size();
		}
		inline void rd_flip(unsigned int size)
		{
			_offPtr += size;
			if (_currPtr > _offPtr)
			{
				unsigned int tmp = _currPtr - _offPtr;
				if(_offPtr >= (tmp+startpos))
				{
					memmove(&_buffer[startpos], &_buffer[_offPtr], tmp);
					_offPtr = startpos;
					_currPtr = tmp+startpos;
				}
			}
			else
			{
				_offPtr = startpos;
				_currPtr = startpos;
			}
		}
		inline unsigned int wr_size() const
		{
			return _maxSize - _currPtr;
		}
		inline void wr_flip(const unsigned int size)
		{
			_currPtr += size;
		}
		inline void reset()
		{
			_offPtr = startpos;
			_currPtr = startpos;
		}
		inline unsigned int maxSize() const
		{
			return _maxSize;
		}
		inline void wr_reserve(const unsigned int size);
		volatile bool isClient;
	private:
		unsigned int _maxSize;
		unsigned int _offPtr;
		unsigned int _currPtr;
		_type _buffer;
};

/**
 * ��̬�ڴ�Ļ����������Զ�̬��չ��������С
 */
#ifdef _POOL_ALLOC_
typedef ByteBuffer<std::vector<unsigned char, __gnu_cxx::__pool_alloc<unsigned char> > > t_BufferCmdQueue;
#else
typedef ByteBuffer<std::vector<unsigned char> > t_BufferCmdQueue;
#endif

/**
 * ģ��ƫ�ػ�
 * �Ի�����ڴ�������������򻺳�д���ݣ���������С���㣬���µ��������С��
 * ��С����ԭ����trunkSize����������������
 * \param size �򻺳�д���˶�������
 */
template <>
inline void t_BufferCmdQueue::wr_reserve(const unsigned int size)
{
	if (wr_size() < size)
	{
#define trunkCount(size) (((size) + trunkSize - 1) / trunkSize)
		_maxSize += (trunkSize * trunkCount(size));
		_buffer.resize(_maxSize);
	}
}


/**
 * ��̬��С�Ļ���������ջ�ռ�����ķ�ʽ�������ڴ棬����һЩ��ʱ�����Ļ�ȡ
 */
typedef ByteBuffer<unsigned char [PACKET_ZIP_BUFFER+startpos]> t_StackCmdQueue;

/**
 * ģ��ƫ�ػ�
 * �Ի�����ڴ�������������򻺳�д���ݣ���������С���㣬���µ��������С��
 * ��С����ԭ����trunkSize����������������
 * \param size �򻺳�д���˶�������
 */
template <>
inline void t_StackCmdQueue::wr_reserve(const unsigned int size)
{
	/*
	if (wr_size() < size)
	{
		//���ܶ�̬��չ�ڴ�
		assert(false);
	}
	// */
}

/**
 * \brief �䳤ָ��ķ�װ���̶���С�Ļ���ռ�
 * ��ջ�ռ���仺���ڴ�
 * \param cmd_type ָ������
 * \param size �����С
 */
template <typename cmd_type, unsigned int size = 64 * 1024>
class CmdBuffer_wrapper
{

	public:

		typedef cmd_type type;
		unsigned int cmd_size;
		unsigned int max_size;
		type *cnt;

		CmdBuffer_wrapper() : cmd_size(sizeof(type)), max_size(size)// : cnt(NULL)
		{
			cnt = (type *)buffer;
			constructInPlace(cnt);
		}

	private:

		unsigned char buffer[size];

};

/**
 * \brief ��װ�׽ӿڵײ㺯�����ṩһ���Ƚ�ͨ�õĽӿ�
 */
class LSocket : private LCantCopy
{

	public:

		static const int T_RD_MSEC					=	2100;					/**< ��ȡ��ʱ�ĺ����� */
		static const int T_WR_MSEC					=	5100;					/**< ���ͳ�ʱ�ĺ����� */

		static const unsigned int PH_LEN 			=	sizeof(unsigned int);	/**< ���ݰ���ͷ��С */
		static const unsigned int PACKET_ZIP_MIN	=	32;						/**< ���ݰ�ѹ����С��С */

		static const unsigned int PACKET_ZIP		=	0x40000000;				/**< ���ݰ�ѹ����־ */
		static const unsigned int PACKET_DEC		=	0x80000000;				/**< ���ݰ����ܱ�־ */
		static const unsigned int INCOMPLETE_READ	=	0x00000001;				/**< �ϴζ��׽ӿڽ��ж�ȡ����û�ж�ȡ��ȫ�ı�־ */
		static const unsigned int INCOMPLETE_WRITE	=	0x00000002;				/**< �ϴζ��׽ӿڽ���д�����ú��д����ϵı�־ */

		static const unsigned int PACKET_MASK			=	trunkSize - 1;	/**< ������ݰ��������� */
		static const unsigned int MAX_DATABUFFERSIZE	=	PACKET_MASK;						/**< ���ݰ���󳤶ȣ�������ͷ4�ֽ� */
		static const unsigned int MAX_DATASIZE			=	(MAX_DATABUFFERSIZE - PH_LEN - 64);		/**< ���ݰ���󳤶� */
		static const unsigned int MAX_USERDATASIZE		=	(MAX_DATASIZE - 128);				/**< �û����ݰ���󳤶� */

		static const char *getIPByIfName(const char *ifName);

		LSocket(const int sock, const struct sockaddr_in *addr = NULL, const bool compress = false);
		~LSocket();

		int recvToCmd(void *pstrCmd, const int nCmdLen, const bool wait);
		bool sendCmd(const void *pstrCmd, const int nCmdLen, const bool buffer = false);
		bool sendCmdNoPack(const void *pstrCmd, const int nCmdLen, const bool buffer = false);
		bool sync();
		void force_sync();
		bool connect();
		int getSocket() { return sock; }
		void setClient();

		int checkIOForRead();
		int checkIOForWrite();
		int recvToBuf_NoPoll();
		int recvToBuf_Http(char* buf);
		int recvToCmd_NoPoll(void *pstrCmd, const int nCmdLen);

		/**
		 * \brief ��ȡ�׽ӿڶԷ��ĵ�ַ
		 * \return IP��ַ
		 */
		inline const char *getIP() const { return inet_ntoa(addr.sin_addr); }
		inline const DWORD getAddr() const { return addr.sin_addr.s_addr; }

		/**
		 * \brief ��ȡ�׽ӿڶԷ��˿�
		 * \return �˿�
		 */
		inline const unsigned short getPort() const { return ntohs(addr.sin_port); }

		/**
		 * \brief ��ȡ�׽ӿڱ��صĵ�ַ
		 * \return IP��ַ
		 */
		inline const char *getLocalIP() const { return inet_ntoa(local_addr.sin_addr); }

		/**
		 * \brief ��ȡ�׽ӿڱ��ض˿�
		 * \return �˿�
		 */
		inline const unsigned short getLocalPort() const { return ntohs(local_addr.sin_port); }

		/**
		 * \brief ���ö�ȡ��ʱ
		 * \param msec ��ʱ����λ���� 
		 * \return 
		 */
		inline void setReadTimeout(const int msec) { rd_msec = msec; }

		/**
		 * \brief ����д�볬ʱ
		 * \param msec ��ʱ����λ���� 
		 * \return 
		 */
		inline void setWriteTimeout(const int msec) { wr_msec = msec; }

		/**
		 * \brief ��Ӽ���¼���epoll������
		 * \param kdpfd epoll������
		 * \param events ����ӵ��¼�
		 * \param ptr �������
		 */
		inline void addEpoll(int kdpfd, __uint32_t events, void *ptr)
		{
			struct epoll_event ev;
			ev.events = events;
			ev.data.ptr = ptr;
			if (-1 == epoll_ctl(kdpfd, EPOLL_CTL_ADD, sock, &ev))
			{
				char _buf[100];
				bzero(_buf, sizeof(_buf));
				strerror_r(errno, _buf, sizeof(_buf));
			}
		}
		/**
		 * \brief ��epoll��������ɾ������¼�
		 * \param kdpfd epoll������
		 * \param events ����ӵ��¼�
		 */
		inline void delEpoll(int kdpfd, __uint32_t events)
		{
			struct epoll_event ev;
			ev.events = events;
			ev.data.ptr = NULL;
			if (-1 == epoll_ctl(kdpfd, EPOLL_CTL_DEL, sock, &ev))
			{
				char _buf[100];
				bzero(_buf, sizeof(_buf));
				strerror_r(errno, _buf, sizeof(_buf));
			}
		}

		inline void setEncMethod(CEncrypt::encMethod m) { enc.setEncMethod(m); }
		inline void set_key_rc5(const unsigned char *data, int nLen, int rounds = RC5_8_ROUNDS) { enc.set_key_rc5(data, nLen, rounds); }
		inline void set_key_des(const_DES_cblock *key) { enc.set_key_des(key); }
		inline unsigned int snd_queue_size() const { return _snd_queue.rd_size() + _enc_queue.rd_size(); }

		inline unsigned int getBufferSize() const {return _rcv_queue.maxSize() + _snd_queue.maxSize();}
	private:
		volatile bool isClient;
		int sock;									/**< �׽ӿ� */
		struct sockaddr_in addr;					/**< �׽ӿڵ�ַ */
		struct sockaddr_in local_addr;				/**< �׽ӿڵ�ַ */
		int rd_msec;								/**< ��ȡ��ʱ������ */
		int wr_msec;								/**< д�볬ʱ������ */
		int syn_msec;								/**< ͬ����ʱ, ���� */

		t_BufferCmdQueue _rcv_queue;				/**< ���ջ���ָ����� */
		unsigned int _rcv_raw_size;					/**< ���ջ���������ݴ�С */
		t_BufferCmdQueue _snd_queue;				/**< ���ܻ���ָ����� */
		t_BufferCmdQueue _enc_queue;				/**< ���ܻ���ָ����� */
		unsigned int _current_cmd;
		LMutex mutex;								/**< �� */

		LTime last_check_time;						/**< ���һ�μ��ʱ�� */
		unsigned int check_time;					/**< ���ʱ�� */		

		unsigned int bitmask;						/**< ��־���� */
		CEncrypt enc;								/**< ���ܷ�ʽ */

		inline void set_flag(unsigned int _f) { bitmask |= _f; }
		inline bool isset_flag(unsigned int _f) const { return bitmask & _f; }
		inline void clear_flag(unsigned int _f) { bitmask &= ~_f; }
		inline bool need_enc() const { return CEncrypt::ENCDEC_NONE!=enc.getEncMethod(); }
		/**
		 * \brief �������ݰ���ͷ��С����
		 * \return ��С����
		 */
		inline unsigned int packetMinSize() const { return PH_LEN; }

		/**
		 * \brief �����������ݰ��ĳ���
		 * \param in ���ݰ�
		 * \return �����������ݰ��ĳ���
		 */
		inline unsigned int packetSize(const unsigned char *in) const 
		{ 
			unsigned int len = *((unsigned int *)in);
			return PH_LEN + len & PACKET_MASK; 
		}

		inline int sendRawData(const void *pBuffer, const int nSize);
		inline bool sendRawDataIM(const void *pBuffer, const int nSize);
		inline int sendRawData_NoPoll(const void *pBuffer, const int nSize);
		inline bool setNonblock();
		inline int waitForRead();
		inline int waitForWrite();
		inline int recvToBuf();

		inline unsigned int packetUnpack(unsigned char *in, const unsigned int nPacketLen, unsigned char *out);
		template<typename buffer_type>
		inline unsigned int packetAppend(const void *pData, const unsigned int nLen, buffer_type &cmd_queue);
		template<typename buffer_type>
		inline unsigned int packetPackEnc(buffer_type &cmd_queue, const unsigned int current_cmd, const unsigned int offset = 0);
	public:
		template<typename buffer_type>
		static inline unsigned int packetPackZip(const void *pData, const unsigned int nLen, buffer_type &cmd_queue, const bool _compress=true);
};
/**
 * \brief �����ݽ�����֯,��Ҫʱѹ���ͼ���
 * \param pData ����֯�����ݣ�����
 * \param nLen ����������ݳ��ȣ�����
 * \param cmd_queue ������������
 * \return �����Ĵ�С
 */
template<typename buffer_type>
inline unsigned int LSocket::packetAppend(const void *pData, const unsigned int nLen, buffer_type &cmd_queue)
{
	unsigned int nSize = packetPackZip(pData, nLen, cmd_queue, PACKET_ZIP == (bitmask & PACKET_ZIP));
//	if (need_enc())
//		nSize = packetPackEnc(cmd_queue, cmd_queue.rd_size());
	return nSize;
}

/**
 * \brief 				�����ݽ��м���
 * \param cmd_queue		�����ܵ����ݣ��������
 * \param current_cmd	���һ��ָ���
 * \param offset		���������ݵ�ƫ��
 * \return 				���ؼ����Ժ���ʵ���ݵĴ�С
 */
template<typename buffer_type>
inline unsigned int LSocket::packetPackEnc(buffer_type &cmd_queue, const unsigned int current_cmd, const unsigned int offset)
{
	unsigned int mod = (cmd_queue.rd_size() - offset) % 8;
	if (0!=mod)
	{
		mod = 8 - mod;
		(*(unsigned int *)(&(cmd_queue.rd_buf()[cmd_queue.rd_size() - current_cmd]))) += mod;
		cmd_queue.wr_flip(mod);
	}
	//���ܶ���
	enc.encdec(&cmd_queue.rd_buf()[offset], cmd_queue.rd_size() - offset, true);

	return cmd_queue.rd_size();
}

const uint32_t uMaxDataSize = LSocket::MAX_DATASIZE;

/**
 * \brief 			�����ݽ���ѹ��,���ϲ��ж��Ƿ���Ҫ����,����ֻ������ܲ����ж�
 * \param pData 	��ѹ�������ݣ�����
 * \param nLen 		��ѹ�������ݳ��ȣ�����
 * \param pBuffer 	��������ѹ���Ժ������
 * \param _compress	�����ݰ�����ʱ���Ƿ�ѹ��
 * \return 			���ؼ����Ժ���ʵ���ݵĴ�С
 */
template<typename buffer_type>
inline unsigned int LSocket::packetPackZip(const void *pData, const unsigned int nLen, buffer_type &cmd_queue, const bool _compress)
{
	/*if (nLen > MAX_DATASIZE)
	{
		Cmd::t_NullCmd *cmd = (Cmd::t_NullCmd *)pData;
		Zebra::logger->warn("%s: ���͵����ݰ�����(cmd = %u, para = %u", __FUNCTION__, cmd->cmd, cmd->para);
	}*/
	unsigned int nSize = nLen > uMaxDataSize? uMaxDataSize: nLen;//nLen & PACKET_MASK;
	unsigned int nMask = 0;//nLen & (~PACKET_MASK);
	if (nSize > PACKET_ZIP_MIN /*���ݰ�����*/ 
			&& _compress /*��ѹ����ǣ����ݰ���Ҫѹ��*/
			/*&& !(nMask & PACKET_ZIP)*/ /*���ݰ���������Ѿ���ѹ������*/ )
	{
		uLong nZipLen = unzip_size(nSize);
		cmd_queue.wr_reserve(nZipLen + PH_LEN);
		int retcode = compress(&(cmd_queue.wr_buf()[PH_LEN]), &nZipLen, (const Bytef *)pData, nSize);
		switch(retcode)
		{
			case Z_OK:
				break;
			case Z_MEM_ERROR:
				break;
			case Z_BUF_ERROR:
				break;
		}
//		cmd_queue.wr_buf()[PH_LEN] = nLen%9;
//		cmd_queue.wr_buf()[PH_LEN+1] = nLen%8;
		nSize = nZipLen;
		nMask |= PACKET_ZIP;
	}
	else
	{
		cmd_queue.wr_reserve(nSize + PH_LEN);
		bcopy(pData, &(cmd_queue.wr_buf()[PH_LEN]), nSize);
	}

	(*(unsigned int *)cmd_queue.wr_buf()) = (nSize | nMask);
	cmd_queue.wr_flip(nSize + PH_LEN);
	//Zebra::logger->debug("%s��չ�����ݰ����ȣ�%u, %u, %u", __FUNCTION__, nSize+PH_LEN, *(unsigned int *)(cmd_queue.wr_buf()) & PACKET_MASK, nLen);

	return nSize + PH_LEN;
}

/**
 * \brief ��������Ϣ����ӿڣ����н��յ���TCP����ָ����Ҫͨ������ӿ�������
 */
class LProcessor
{
	public:
		virtual bool msgParse(const void *, const unsigned int) = 0;
		virtual ~LProcessor() {}
};
/**
 * \brief ָ����������
 */
struct CmdAnalysis
{
	CmdAnalysis(const char *disc,DWORD time_secs):_log_timer(time_secs)
	{
		bzero(_disc,sizeof(disc));
		strncpy(_disc,disc,sizeof(_disc)-1);
		bzero(_data,sizeof(_data));
		_switch=false;
	}
	struct
	{
		DWORD num;
		DWORD size;
	}_data[256][256] ;
	LMutex _mutex;
	Timer _log_timer;
	char _disc[256];
	bool _switch;//����
	void add(const BYTE &cmd, const BYTE &para , const DWORD &size)
	{
		if(!_switch)
		{
			return ;
		}
		_mutex.lock(); 
		_data[cmd][para].num++;
		_data[cmd][para].size +=size;
		LRTime ct;
		if(_log_timer(ct))
		{
			for(int i = 0 ; i < 256 ; i ++)
			{
				for(int j = 0 ; j < 256 ; j ++)
				{
					if(_data[i][j].num)
					{}
				}
			}
			bzero(_data,sizeof(_data));
		}
		_mutex.unlock(); 
	}
};

#endif
