/**
 * \file
 * \version  $Id: LTCPClientTask.h  $
 * \author  
 * \date 
 * \brief TCP�ͻ��˷�װ
 *
 * 
 */

#ifndef _zTCPClientTask_h_
#define _zTCPClientTask_h_

#include <pthread.h>
#include <string>

#include "LSocket.h"
#include "LCantCopy.h"
#include "LTime.h"

/**
 * \brief TCP�ͻ���
 *
 * ��װ��һЩTCP�ͻ��˵��߼������罨�����ӵȵ�
 *
 */
class LTCPClientTask : public LProcessor, private LCantCopy
{

	public:

		/**
		 * \brief ���ӶϿ�����
		 *
		 */
		enum TerminateMethod
		{
			TM_no,						/**< û�н������� */
			TM_sock_error,				/**< ��⵽�׽ӿڹرջ����׽ӿ��쳣 */
			TM_service_close			/**< �����������ر� */
		};

		/**
		 * \brief ��������״̬
		 *
		 */
		enum ConnState
		{
			close		=	0,							/**< ���ӹر�״̬ */
			sync		=	1,							/**< �ȴ�ͬ��״̬ */
			okay		=	2,							/**< ���Ӵ���׶� */
			recycle		=	3							/**< �����˳�״̬ */
		};

		/**
		 * \brief ���캯��������ʵ�����󣬳�ʼ�������Ա
		 * \param ip ��ַ
		 * \param port �˿�
		 * \param compress �ײ����ݴ����Ƿ�֧��ѹ��
		 */
		LTCPClientTask(
				const std::string &ip, 
				const unsigned short port,
				const bool compress = false) : pSocket(NULL), compress(compress), ip(ip), port(port), _ten_min(600)
		{
			state = close;
			terminate = TM_no;
			mainloop = false;
			fdsradd = false; 
		}

		/**
		 * \brief �������������ٶ���
		 */
		virtual ~LTCPClientTask() 
		{
			final();
		}

		/**
		 * \brief �������
		 *
		 */
		void final()
		{
			SAFE_DELETE(pSocket);
			terminate = TM_no;
			mainloop = false;
		}

		/**
		 * \brief �ж��Ƿ���Ҫ�ر�����
		 * \return true or false
		 */
		bool isTerminate() const
		{
			return TM_no != terminate;
		}

		/**
		 * \brief ��Ҫ�����Ͽ��ͻ��˵�����
		 * \param method ���ӶϿ���ʽ
		 */
		void Terminate(const TerminateMethod method)
		{
			terminate = method;
		}

		/**
		 * \brief ����ǵ�һ�ν�����ѭ��������Ҫ�ȴ������е�ָ��
		 * \return �Ƿ��ǵ�һ�ν���������ѭ��
		 */
		bool checkFirstMainLoop()
		{
			if (mainloop)
				return false;
			else
			{
				mainloop = true;
				return true;
			}
		}

		/**
		 * \brief ��ȡ��������ǰ״̬
		 * \return ״̬
		 */
		const ConnState getState() const
		{
			return state;
		}

		/**
		 * \brief ��������������һ��״̬
		 * \param state ��Ҫ���õ�״̬
		 */
		void setState(const ConnState state)
		{
			this->state = state;
		}

		/**
		 * \brief ���״̬���ַ�������
		 * \param state ״̬
		 * \return ����״̬���ַ�������
		 */
		const char *getStateString(const ConnState state)
		{
			const char *retval = NULL;

			switch(state)
			{
				case close:
					retval = "close";
					break;
				case sync:
					retval = "sync";
					break;
				case okay:
					retval = "okay";
					break;
				case recycle:
					retval = "recycle";
					break;
			}

			return retval;
		}

#ifdef _USE_EPOLL_
		/**
		 * \brief ��Ӽ���¼���epoll������
		 * \param kdpfd epoll������
		 * \param events ����ӵ��¼�
		 * \param ptr �������
		 */
		void addEpoll(int kdpfd, __uint32_t events, void *ptr)
		{
			if (pSocket)
				pSocket->addEpoll(kdpfd, events, ptr);
		}
		/**
		 * \brief ��epoll��������ɾ������¼�
		 * \param kdpfd epoll������
		 * \param events ����ӵ��¼�
		 */
		void delEpoll(int kdpfd, __uint32_t events)
		{
			if (pSocket)
				pSocket->delEpoll(kdpfd, events);
		}
#else
		/**
		 * \brief ���pollfd�ṹ
		 * \param pfd �����Ľṹ
		 * \param events �ȴ����¼�����
		 */
		void fillPollFD(struct pollfd &pfd, short events)
		{
			//if (pSocket)
			//	pSocket->fillPollFD(pfd, events);
		}
#endif

		/**
		 * \brief ���ĳ��״̬�Ƿ���֤��ʱ
		 * \param state ������״̬
		 * \param ct ��ǰϵͳʱ��
		 * \param timeout ��ʱʱ��
		 * \return ����Ƿ�ɹ�
		 */
		bool checkStateTimeout(const ConnState state, const LTime &ct, const time_t timeout) const
		{
			if (state == this->state)
				return (lifeTime.elapse(ct) >= timeout);
			else
				return false;
		}

		/**
		 * \brief ������֤����
		 *
		 * ������Ҫ�����������������֤һ��TCP���ӣ�ÿ��TCP���ӱ���ͨ����֤���ܽ�����һ������׶Σ�ȱʡʹ��һ���յ�ָ����Ϊ��ָ֤��
		 * <pre>
		 * int retcode = pSocket->recvToBuf_NoPoll();
		 * if (retcode > 0)
		 * {
		 * 		unsigned char pstrCmd[LSocket::MAX_DATASIZE];
		 * 		int nCmdLen = pSocket->recvToCmd_NoPoll(pstrCmd, sizeof(pstrCmd));
		 * 		if (nCmdLen <= 0)
		 * 			//����ֻ�Ǵӻ���ȡ���ݰ������Բ������û������ֱ�ӷ���
		 * 			return 0;
		 * 		else
		 * 		{
		 * 			LSocket::t_NullCmd *ptNullCmd = (LSocket::t_NullCmd *)pstrCmd;
		 * 			if (LSocket::null_opcode == ptNullCmd->opcode)
		 * 			{
		 * 				std::cout << "�ͻ�������ͨ����֤" << std::endl;
		 * 				return 1;
		 * 			}
		 * 			else
		 * 			{
		 * 				return -1;
		 * 			}
		 * 		}
		 * }
		 * else
		 * 		return retcode;
		 * </pre>
		 *
		 * \return ��֤�Ƿ�ɹ���1��ʾ�ɹ������Խ�����һ��������0����ʾ��Ҫ�����ȴ���֤��-1��ʾ�ȴ���֤ʧ�ܣ���Ҫ�Ͽ�����
		 */
		virtual int checkRebound()
		{
			return 1;
		}

		/**
		 * \brief ��Ҫɾ�����TCP���������Դ
		 */
		virtual void recycleConn() {};

		/**
		 * \brief һ������������֤�Ȳ�������Ժ���Ҫ��ӵ�ȫ��������
		 *
		 * ���ȫ���������ⲿ����
		 *
		 */
		virtual void addToContainer() {};

		/**
		 * \brief ���������˳���ʱ����Ҫ��ȫ��������ɾ��
		 *
		 * ���ȫ���������ⲿ����
		 *
		 */
		virtual void removeFromContainer() {};

		virtual bool connect();

		void checkConn();
		bool sendCmd(const void *pstrCmd, const int nCmdLen);
		bool ListeningRecv(bool);
		bool ListeningSend();

		void getNextState();
		void resetState();
		/**
		 * \brief ����Ƿ��Ѿ�������¼�
		 *
		 * \return �Ƿ����
		 */
		bool isFdsrAdd()
		{
			return fdsradd;
		}
		/**
		 * \brief ���ü�����¼���־
		 *
		 * \return �Ƿ����
		 */
		bool fdsrAdd(bool set=true)
		{
			 fdsradd=set;
			 return fdsradd;
		}
		void setIpPort(const std::string &ip, const unsigned short port)
		{
			this->ip=ip;
			this->port=port;
		}

	protected:

		LSocket *pSocket;								/**< �ײ��׽ӿ� */
		volatile ConnState state;						/**< ����״̬ */

	private:

		bool fdsradd;									/**< ���¼���ӱ�־ */
		const bool compress;							/**< �Ƿ�֧��ѹ�� */
		std::string ip;									/**< ��������ַ */
		unsigned short port;							/**< �������˿� */

		LTime lifeTime;									/**< �����ڣ���¼ÿ��״̬�ı��ʱ�� */
		TerminateMethod terminate;						/**< �Ƿ�������� */
		volatile bool mainloop;							/**< �Ƿ��Ѿ�����������ѭ�� */
		Timer _ten_min;

}; 


#endif
