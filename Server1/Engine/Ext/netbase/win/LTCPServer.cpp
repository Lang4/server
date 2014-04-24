/**
 * \brief 实现类zTCPServer
 *
 * 
 */
#include "LTCPServer.h"
#include <stdio.h>
/**
 * \brief 构造函数,用于构造一个服务器zTCPServer对象
 * \param name 服务器名称
 */
LTCPServer::LTCPServer(const std::string &name)
: name(name),
  sock(INVALID_SOCKET)
{
	printf("LTCPServer::LTCPServer\n");
}

/**
 * \brief 析构函数,用于销毁一个zTCPServer对象
 *
 *
 */
LTCPServer::~LTCPServer() 
{
  printf("LTCPServer::~LTCPServer");

  if (INVALID_SOCKET != sock) 
  {
    ::shutdown(sock,0x02);
    ::closesocket(sock);
    sock = INVALID_SOCKET;
  }
}

/**
 * \brief 绑定监听服务到某一个端口
 * \param name 绑定端口名称
 * \param port 具体绑定的端口
 * \return 绑定是否成功
 */
bool LTCPServer::bind(const std::string &name,const WORD port)
{
  printf("LTCPServer::bind");
  struct sockaddr_in addr;

  if (INVALID_SOCKET != sock) 
  {
    printf("服务器可能已经初始化");;
    return false;
  }

  sock = ::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  if (INVALID_SOCKET == sock) 
  {
    printf("创建套接口失败");
    return false;
  }

  //设置套接口为可重用状态
  int reuse = 1;
  if (-1 == ::setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char*)&reuse,sizeof(reuse))) 
  {
    printf("不能设置套接口为可重用状态");
    ::closesocket(sock);
    sock = INVALID_SOCKET;
    return false;
  }

  //设置套接口发送接收缓冲,并且服务器的必须在accept之前设置
  int window_size = 128 * 1024;
  if (-1 == ::setsockopt(sock,SOL_SOCKET,SO_RCVBUF,(char*)&window_size,sizeof(window_size)))
  {
    ::closesocket(sock);
    return false;
  }
  if (-1 == ::setsockopt(sock,SOL_SOCKET,SO_SNDBUF,(char*)&window_size,sizeof(window_size)))
  {
    ::closesocket(sock);
        sock = INVALID_SOCKET;
    return false;
  }

  bzero(&addr,sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  int retcode = ::bind(sock,(struct sockaddr *) &addr,sizeof(addr));
  if (-1 == retcode) 
  {
    char tmpc[MAX_PATH];
	sprintf(tmpc, "网络初始化失败，端口 %u 已经被占用！\n",port);
	MessageBox( NULL, tmpc, "错误", MB_ICONERROR );
    ::closesocket(sock);
        sock = INVALID_SOCKET;
    return false;
  }

  retcode = ::listen(sock,MAX_WAITQUEUE);
  if (-1 == retcode) 
  {
	  char tmpc[MAX_PATH];
	  sprintf(tmpc, "监听套接口失败，端口 %u 已经被占用！",port);
	  MessageBox( NULL, tmpc, "错误", MB_ICONERROR );
    ::closesocket(sock);
    sock = INVALID_SOCKET;
    return false;
  }

  printf("初始化 %s:%u 成功\n",name.c_str(),port);

  return true;
}

/**
 * \brief 接受客户端的连接
 *
 *
 * \param addr 返回的地址
 * \return 返回的客户端套接口
 */
int LTCPServer::accept(struct sockaddr_in *addr)
{
//  Zebra::logger->info("LTCPServer::accept");
  int len = sizeof(struct sockaddr_in);
  bzero(addr,sizeof(struct sockaddr_in));

    //return ::WSAAccept(sock,(struct sockaddr *)addr,&len,NULL, NULL );
  return ::accept(sock,(struct sockaddr *)addr,&len);
}

