#include "eventpool.h"
#include "connect.h"
#include "win32helper.h"
namespace mynet{
	class Event:public EventBase{
    public:
        Event(Target *target):EventBase(target)
        {
			reset();
		}
		OVERLAPPED overlapped;
        bool checkValid()
        {
            return target;
        }
           
		WSABUF         m_wsaBuf;                                 
		char           buffer[MAX_BUFFER_LEN];             
		           
		DWORD msgLen;
		
		HANDLE poolHandle; 
		virtual void reset()
		{
			memset(buffer,0,MAX_BUFFER_LEN);
			memset(&overlapped,0,sizeof(overlapped));  
			m_wsaBuf.buf = buffer;
			m_wsaBuf.len = MAX_BUFFER_LEN;
			eventType     = 0;
			msgLen = 0;
		}
		virtual void deal(){};
		virtual void redo(){deal();}
    };
	/**
	 *  
	 */
	template<typename TARGET>
	class InEvent:public Event{
	public:
		InEvent(TARGET *target):Event(target)
        {
			
		}
		void deal()
		{
			DWORD dwFlags = 0;
			DWORD dwBytes = 0;
			WSABUF *p_wbuf   = &m_wsaBuf;
			OVERLAPPED *p_ol = (OVERLAPPED*)&overlapped;
			reset();
			eventType = IN_EVT;
			int nBytesRecv = WSARecv((SOCKET)target->getHandle(), p_wbuf, 1, &msgLen, &dwFlags, p_ol, NULL );
			int result = WSAGetLastError();
			if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != result))
			{
				return;
			}
		}
	};
	/**
	 *  
	 */
	template<typename TARGET>
	class OutEvent:public Event{
	public:
		OutEvent(TARGET *target):Event(target)
		{
			
		}
		mynet::MyList<char*> buffers;
		void deal()
		{
			if (!dataLen) return;
			DWORD dwFlags = 0;
			msgLen = 0;
			WSABUF *p_wbuf   = &m_wsaBuf;
			m_wsaBuf.len = dataLen;
			OVERLAPPED *p_ol = (OVERLAPPED*)&overlapped;
			eventType = OUT_EVT;
			p_wbuf->buf = buffer;
			int nBytesRecv = WSASend((SOCKET)target->getHandle(), p_wbuf, 1, &msgLen, dwFlags, p_ol, NULL );
			int result = WSAGetLastError();
			if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != result))
			{
				printf("error happened int outevent deal\n");
				return;
			}
			return;
		}
	};
	/**
	 *   
	 **/
	template<typename TARGET>
	class AcceptEvent:public Event{
	public:
		AcceptEvent(TARGET *target):Event(target)
		{
			SOCKET socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);  
			if( INVALID_SOCKET ==  socket)  
			{  
				return;  
			} 
			handle = socket;	
		}
		void deal()
		{
			DWORD dwBytes = 0;  
			eventType = ACCEPT_EVT;  
			WSABUF *p_wbuf   = &m_wsaBuf;
			OVERLAPPED *p_ol = (OVERLAPPED*)&overlapped;
			// 
			
			AcceptHelper::getMe().doAccept((SOCKET)target->getHandle(),handle,p_wbuf,p_ol);
		}
		SOCKET handle;
		virtual unsigned int getPeerHandle() {return (unsigned int)handle;}
	};
   
	bool EventPool::init()
	{
		static bool initLoad = false;
		if (!initLoad)
		{
			WSADATA wsaData;
			int nResult;
			nResult = WSAStartup(MAKEWORD(2,2), &wsaData);
			initLoad = true;
		}
		if (!poolHandle)
			poolHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0 );
		return poolHandle != NULL;
	}
    void EventPool::bindEvent(Target *target,int eventType)
	{
		if (target->getHandle() == -1 ) return;
		 HANDLE tempHandle = CreateIoCompletionPort((HANDLE)target->getHandle(), poolHandle, (DWORD)target, 0);
		 if (eventType & ACCEPT_EVT)
		 {
			target->inEvt = new AcceptEvent<Target>(target);
			target->inEvt->deal();
		 }
		 if (eventType & IN_EVT)
		 {
			target->inEvt = new InEvent<Target>(target);
			target->inEvt->deal();
		 }
		 if (eventType & OUT_EVT)
		 {
			target->outEvt = new OutEvent<Target>(target);
			target->doSend(target->outEvt);
		 }
	}

	EventBase* EventPool::pullEvent()
	{
		//  
		OVERLAPPED *pOverlapped = NULL;
		Target*target = NULL;
		DWORD dataLen = 0;
		BOOL bReturn = GetQueuedCompletionStatus(
			poolHandle,
			&dataLen,
			(PULONG_PTR)&target,
			&pOverlapped,
			INFINITE
		);
		//  
		if ( 0==(DWORD)target)
		{
			return NULL;
		}
		//  
		if( !bReturn )  
		{
			if (!target->inEvt) return NULL;
			target->inEvt->eventType = ERR_EVT;
			return target->inEvt;  
		}  
		else  
		{
			//  
			Event* evt = CONTAINING_RECORD(pOverlapped, Event, overlapped); 
			evt->dataLen = dataLen;
			//  
			if (dataLen == 0 && (evt->isIn() || evt->isOut()))
			{
				evt->eventType = ERR_EVT; //  
			}
			return evt;
		}
		return NULL;
	}

	/**
	 *  pool  
	 **/
	 void Connection::doRead(EventBase *evt,stGetPackage *callback)
	{
		InEvent<Connection>* event = static_cast<InEvent<Connection>*>( evt );
		if (directDealCmd) // 
		{
			Record record(event->m_wsaBuf.buf,evt->dataLen);
			decoder.decode(&record,evt->target,callback);
		}
		else
		{
			Record *record = new Record(event->m_wsaBuf.buf,evt->dataLen);
			recvs.write(record);
		}
		evt->redo();
	}
	/**
	 * pool 
	 **/
	void Connection::doSend(EventBase *evt,bool over)
	{
		bool tag = false;
		OutEvent<Connection>* event = static_cast<OutEvent<Connection>*>( evt );
		if (event->msgLen < event->dataLen && !over)
		{
			return;
		}
		event->dataLen = 0;
		event->reset();
		int leftLen = EventBase::MAX_BUFFER_LEN;
		while (!sends.empty() && leftLen > 0)
		{
			tag = true;
			Record *record = NULL;
			if (sends.readOnly(record))
			{
				unsigned int realCopySize = record->recv(event->buffer,leftLen);
				evt->dataLen += realCopySize;
				
				if (leftLen == realCopySize)
				{
					leftLen = 0;
					if (record->empty())
					{
						delete record;
						sends.pop();
					}
					break;
				}
				else
				{
					leftLen -= realCopySize;
					if(!record->empty())
					{
						// TODO ERROR
					}
					sends.pop();
				}
			}else break;
		}
		if (tag)
			evt->redo();
	}
};