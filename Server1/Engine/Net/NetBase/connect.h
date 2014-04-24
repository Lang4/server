#pragma once
#include "mylist.h"
#include "decoder.h"
#include "eventpool.h"
#include "mylist.h"
#include "record.h"
#include "target.h"
#include <vector>
#include <string>
#include <list>
/**
 * windows network epoll 
 */
namespace mynet{
    /*
     *******************Connection***********************
     */
    class Connection:public Target{
    public:
         virtual void destroy();
		 Connection();
		 unsigned long getHandle();
		 void setHandle(unsigned long socket);
		 
		 /**
		  *
		  */
		 void sendCmd(void *cmd,unsigned int len);
		 void recvCmdCallback(void *cmd,unsigned int len);
		 Decoder decoder;
		 bool directDealCmd;
		 /** 
		  *  
		  */
		 unsigned int getCmd(void *cmd,unsigned int len);
		 /**
		  * 
		  **/
		 unsigned int recv(void *cmd,unsigned int size);
		 int send(void *cmd,unsigned int len);
		/**
		 * 
		 **/
		 void doRead(EventBase *evt,stGetPackage *callback = NULL);
		/**
		 *  
		 **/
		void doSend(EventBase *evt,bool over = true);
		 //  
		unsigned long socket;
		MyList<Record*> recvs;
		MyList<Record*> sends;
        template<class CmdObject>
        void sendObject(CmdObject *object)
        {
            cmd::BinaryStream ss ;
            object->write(&ss);
            if (ss.size())
            {
                Decoder decoder;
                decoder.encode(ss.content(),ss.size());
                Record * record = decoder.getRecord();
                printf("发送逻辑层数据:%d 大小:%u\n",object->__msg__id__,record->contentSize);

                sends.write(record);
                if (outEvt)
                    doSend(outEvt,false);
            }
        }
		void flushSend()
		{
			if (outEvt)
				outEvt->redo();
		}
		bool checkValid(){return socket != -1;}
		virtual ~Connection(){}
    };
    /**
     * *************************Client ********************************
     */
    class Client:public Connection{
    public:
        Client(const char *ip,unsigned short port)
        {
			init(ip,port);
            this->peerIp = ip;
            this->port = port;
        }
        void reconnect()
        {
            socket = init(peerIp.c_str(), port);
        }
        std::string peerIp;
        unsigned short port;
        static int init(const char *ip,unsigned short port);
        void close();
    };
    /**
     **************************Server**********************************
     */
    class Server:public Target{
    public:
		Server(const char *ip,unsigned short port);
		void init(const char *ip,unsigned short port);
		unsigned long getHandle(){return (unsigned long)socket;}
		unsigned long socket;
	};
}