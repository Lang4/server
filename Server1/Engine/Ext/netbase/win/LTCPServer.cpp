/**
 * \brief ʵ����zTCPServer
 *
 * 
 */
#include "LTCPServer.h"
#include <stdio.h>
/**
 * \brief ���캯��,���ڹ���һ��������zTCPServer����
 * \param name ����������
 */
LTCPServer::LTCPServer(const std::string &name)
: name(name),
  sock(INVALID_SOCKET)
{
	printf("LTCPServer::LTCPServer\n");
}

/**
 * \brief ��������,��������һ��zTCPServer����
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
 * \brief �󶨼�������ĳһ���˿�
 * \param name �󶨶˿�����
 * \param port ����󶨵Ķ˿�
 * \return ���Ƿ�ɹ�
 */
bool LTCPServer::bind(const std::string &name,const WORD port)
{
  printf("LTCPServer::bind");
  struct sockaddr_in addr;

  if (INVALID_SOCKET != sock) 
  {
    printf("�����������Ѿ���ʼ��");;
    return false;
  }

  sock = ::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  if (INVALID_SOCKET == sock) 
  {
    printf("�����׽ӿ�ʧ��");
    return false;
  }

  //�����׽ӿ�Ϊ������״̬
  int reuse = 1;
  if (-1 == ::setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char*)&reuse,sizeof(reuse))) 
  {
    printf("���������׽ӿ�Ϊ������״̬");
    ::closesocket(sock);
    sock = INVALID_SOCKET;
    return false;
  }

  //�����׽ӿڷ��ͽ��ջ���,���ҷ������ı�����accept֮ǰ����
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
	sprintf(tmpc, "�����ʼ��ʧ�ܣ��˿� %u �Ѿ���ռ�ã�\n",port);
	MessageBox( NULL, tmpc, "����", MB_ICONERROR );
    ::closesocket(sock);
        sock = INVALID_SOCKET;
    return false;
  }

  retcode = ::listen(sock,MAX_WAITQUEUE);
  if (-1 == retcode) 
  {
	  char tmpc[MAX_PATH];
	  sprintf(tmpc, "�����׽ӿ�ʧ�ܣ��˿� %u �Ѿ���ռ�ã�",port);
	  MessageBox( NULL, tmpc, "����", MB_ICONERROR );
    ::closesocket(sock);
    sock = INVALID_SOCKET;
    return false;
  }

  printf("��ʼ�� %s:%u �ɹ�\n",name.c_str(),port);

  return true;
}

/**
 * \brief ���ܿͻ��˵�����
 *
 *
 * \param addr ���صĵ�ַ
 * \return ���صĿͻ����׽ӿ�
 */
int LTCPServer::accept(struct sockaddr_in *addr)
{
//  Zebra::logger->info("LTCPServer::accept");
  int len = sizeof(struct sockaddr_in);
  bzero(addr,sizeof(struct sockaddr_in));

    //return ::WSAAccept(sock,(struct sockaddr *)addr,&len,NULL, NULL );
  return ::accept(sock,(struct sockaddr *)addr,&len);
}

