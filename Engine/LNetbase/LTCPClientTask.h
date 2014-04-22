/**
 * \file
 * \version  $Id: LTCPClientTask.h  $
 * \author  
 * \date 
 * \brief TCP客户端封装
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
 * \brief TCP客户端
 *
 * 封装了一些TCP客户端的逻辑，比如建立连接等等
 *
 */
class LTCPClientTask : public LProcessor, private LCantCopy
{

	public:

		/**
		 * \brief 连接断开类型
		 *
		 */
		enum TerminateMethod
		{
			TM_no,						/**< 没有结束任务 */
			TM_sock_error,				/**< 检测到套接口关闭或者套接口异常 */
			TM_service_close			/**< 服务器即将关闭 */
		};

		/**
		 * \brief 连接任务状态
		 *
		 */
		enum ConnState
		{
			close		=	0,							/**< 连接关闭状态 */
			sync		=	1,							/**< 等待同步状态 */
			okay		=	2,							/**< 连接处理阶段 */
			recycle		=	3							/**< 连接退出状态 */
		};

		/**
		 * \brief 构造函数，创建实例对象，初始化对象成员
		 * \param ip 地址
		 * \param port 端口
		 * \param compress 底层数据传输是否支持压缩
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
		 * \brief 析构函数，销毁对象
		 */
		virtual ~LTCPClientTask() 
		{
			final();
		}

		/**
		 * \brief 清楚数据
		 *
		 */
		void final()
		{
			SAFE_DELETE(pSocket);
			terminate = TM_no;
			mainloop = false;
		}

		/**
		 * \brief 判断是否需要关闭连接
		 * \return true or false
		 */
		bool isTerminate() const
		{
			return TM_no != terminate;
		}

		/**
		 * \brief 需要主动断开客户端的连接
		 * \param method 连接断开方式
		 */
		void Terminate(const TerminateMethod method)
		{
			terminate = method;
		}

		/**
		 * \brief 如果是第一次进入主循环处理，需要先处理缓冲中的指令
		 * \return 是否是第一次进入主处理循环
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
		 * \brief 获取连接任务当前状态
		 * \return 状态
		 */
		const ConnState getState() const
		{
			return state;
		}

		/**
		 * \brief 设置连接任务下一个状态
		 * \param state 需要设置的状态
		 */
		void setState(const ConnState state)
		{
			this->state = state;
		}

		/**
		 * \brief 获得状态的字符串描述
		 * \param state 状态
		 * \return 返回状态的字符串描述
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
		 * \brief 添加检测事件到epoll描述符
		 * \param kdpfd epoll描述符
		 * \param events 待添加的事件
		 * \param ptr 额外参数
		 */
		void addEpoll(int kdpfd, __uint32_t events, void *ptr)
		{
			if (pSocket)
				pSocket->addEpoll(kdpfd, events, ptr);
		}
		/**
		 * \brief 从epoll描述符中删除检测事件
		 * \param kdpfd epoll描述符
		 * \param events 待添加的事件
		 */
		void delEpoll(int kdpfd, __uint32_t events)
		{
			if (pSocket)
				pSocket->delEpoll(kdpfd, events);
		}
#else
		/**
		 * \brief 填充pollfd结构
		 * \param pfd 待填充的结构
		 * \param events 等待的事件参数
		 */
		void fillPollFD(struct pollfd &pfd, short events)
		{
			//if (pSocket)
			//	pSocket->fillPollFD(pfd, events);
		}
#endif

		/**
		 * \brief 检测某种状态是否验证超时
		 * \param state 待检测的状态
		 * \param ct 当前系统时间
		 * \param timeout 超时时间
		 * \return 检测是否成功
		 */
		bool checkStateTimeout(const ConnState state, const LTime &ct, const time_t timeout) const
		{
			if (state == this->state)
				return (lifeTime.elapse(ct) >= timeout);
			else
				return false;
		}

		/**
		 * \brief 连接验证函数
		 *
		 * 子类需要重载这个函数用于验证一个TCP连接，每个TCP连接必须通过验证才能进入下一步处理阶段，缺省使用一条空的指令作为验证指令
		 * <pre>
		 * int retcode = pSocket->recvToBuf_NoPoll();
		 * if (retcode > 0)
		 * {
		 * 		unsigned char pstrCmd[LSocket::MAX_DATASIZE];
		 * 		int nCmdLen = pSocket->recvToCmd_NoPoll(pstrCmd, sizeof(pstrCmd));
		 * 		if (nCmdLen <= 0)
		 * 			//这里只是从缓冲取数据包，所以不会出错，没有数据直接返回
		 * 			return 0;
		 * 		else
		 * 		{
		 * 			LSocket::t_NullCmd *ptNullCmd = (LSocket::t_NullCmd *)pstrCmd;
		 * 			if (LSocket::null_opcode == ptNullCmd->opcode)
		 * 			{
		 * 				std::cout << "客户端连接通过验证" << std::endl;
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
		 * \return 验证是否成功，1表示成功，可以进入下一步操作，0，表示还要继续等待验证，-1表示等待验证失败，需要断开连接
		 */
		virtual int checkRebound()
		{
			return 1;
		}

		/**
		 * \brief 需要删除这个TCP连接相关资源
		 */
		virtual void recycleConn() {};

		/**
		 * \brief 一个连接任务验证等步骤完成以后，需要添加到全局容器中
		 *
		 * 这个全局容器是外部容器
		 *
		 */
		virtual void addToContainer() {};

		/**
		 * \brief 连接任务退出的时候，需要从全局容器中删除
		 *
		 * 这个全局容器是外部容器
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
		 * \brief 检查是否已经加入读事件
		 *
		 * \return 是否加入
		 */
		bool isFdsrAdd()
		{
			return fdsradd;
		}
		/**
		 * \brief 设置加入读事件标志
		 *
		 * \return 是否加入
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

		LSocket *pSocket;								/**< 底层套接口 */
		volatile ConnState state;						/**< 连接状态 */

	private:

		bool fdsradd;									/**< 读事件添加标志 */
		const bool compress;							/**< 是否支持压缩 */
		std::string ip;									/**< 服务器地址 */
		unsigned short port;							/**< 服务器端口 */

		LTime lifeTime;									/**< 生命期，记录每次状态改变的时间 */
		TerminateMethod terminate;						/**< 是否结束任务 */
		volatile bool mainloop;							/**< 是否已经进入主处理循环 */
		Timer _ten_min;

}; 


#endif
