#include "../include/Debug.h"
#include "../include/Acceptor.h"
#include "../include/InetAddr.h"
#include "../include/SvcHandler.h"
#include "../include/TaskWithMQ.h"
#include "../include/EpollReactor.h"
#include "../include/ThreadManager.h"
#include "../include/MultiReactors.h"

#include <assert.h>
#include <signal.h>

// log server
typedef Singleton_T<LogSvr> Log;
THREAD_FUNC_RETURN_T event_loop (void *arg)
{
    Reactor *r = reinterpret_cast<Reactor*> (arg);
    r->run_reactor_event_loop ();
    Log::instance ()->put (LOG_RINFO, "reactor event loop done");
    return 0;
}
// --------------------------------------------------------
enum
{
    TRANSFER_FILE  = 200,
    ECHO_MSG	   = 202,
    EXIT_CMD	   = 204
};
class MsgHeader
{
public:
    int msgid;
    int msglen;  // not include header
    MsgHeader ()
    : msgid (-1)
    , msglen (0)
    {}
};
class EventItem
{
public:
    MsgHeader *msg_head;
    char *msg_body;
    EventItem ()
    : msg_head (0)
    , msg_body (0)
    {}
    ~EventItem ()
    {
	if (msg_head)
	    delete msg_head;
	if (msg_body)
	    delete [] msg_body;
    }
};
#define MAX_BUFF_LEN           1440
class MsgHandleTask : public TaskWithMQ
{
public:
    MsgHandleTask ()
    : TaskWithMQ (ThreadManager::instance ())
    , handle_ (NDK_INVALID_HANDLE)
    , stop_ (0)
    , total_flux_ (0)
    {
    }
    
    ~MsgHandleTask (){}
    virtual int open (void * = 0)
    {
	//this->handle_ = ::open ("recv.file", O_CREAT | O_WRONLY, 0644);
	return this->activate (Thread::THR_JOIN, 3);
    }
    virtual int svc ()
    {
	MessageBlock *mb = 0;
	TimeValue tv (3, 0);
	int count = 0;
	while (1)
	{
	    count += this->getq_n (mb, 10, &tv);
	    if (mb == 0) continue;

	    MessageBlock *tmb = mb;
	    for (; mb != 0; mb = mb->next ())
	    {
		EventItem *pitem = reinterpret_cast<EventItem*>(mb->base ());
		//NDK::write (this->handle_, pitem->msg_body, pitem->msg_head->msglen, 0);
		//this->total_flux_ += pitem->msg_head->msglen;
		delete pitem;
	    }
	    tmb->release ();
	    if (this->stop_)
	    {
		Log::instance ()->put (LOG_RINFO, "recv total %ld Bytes\n", this->total_flux_);
		this->stop_ = 0;
	    }
	    //Log::instance ()->put (LOG_RINFO, "msgqueue get %d mb\n", count);
	}
	return 0;
    }
    int stop ()
    {
	this->stop_ = 1;
    }
protected:
    int handle_;
    volatile int stop_;
    long long total_flux_;
};
typedef Singleton_T<MsgHandleTask> msg_handle_task;
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class Server : public SvcHandler
{
public:
    Server () 
    {
	Log::instance ()->put (LOG_RINFO, "server constructor %d\n", count_++);
	msg_header_recv_pos_ = 0;
	msg_body_recv_pos_ = 0;
	msg_header_ = 0;
	msg_body_   = 0;
    }
    ~Server ()
    { 
	Log::instance ()->put (LOG_RINFO, "server destructor\n");
    }
    virtual int open (void *arg)
    {
	Log::instance ()->put (LOG_RINFO, "server open\n");
	unused_arg (arg);
	assert (SvcHandler::open (0) == 0);
    }
    virtual int handle_input (NDK_HANDLE handle)
    {
	TimeValue tv(3, 0);
	this->msg_header_ = new MsgHeader;
	this->msg_body_   = new char[MAX_BUFF_LEN];
	int result = 0;
	result = NDK::recv_n (handle, 
		this->msg_header_, 
		sizeof(MsgHeader), 
		0);//&tv);
	if (result == 0)
	{
	    Log::instance ()->put (LOG_RINFO, "peer close when recv header\n");
	    return -1;
	}else if (result < 0)
	{
	    Log::instance ()->put (LOG_RINFO, "peer recv header failed\n");
	    return -1;
	}
	// recv body
	result = NDK::recv_n (handle, 
		this->msg_body_, 
		this->msg_header_->msglen, 
		0);//&tv);
	if (result <= 0)
	{
	    Log::instance ()->put (LOG_RINFO, "peer close when recv body\n");
	    return -1;
	}else if (result < 0)
	{
	    Log::instance ()->put (LOG_RINFO, "peer recv body failed\n");
	    return -1;
	}
	EventItem *item = new EventItem;
	item->msg_head  = this->msg_header_;
	item->msg_body  = this->msg_body_;
	this->msg_header_ = 0;
	this->msg_body_   = 0;
	msg_handle_task::instance ()->putq (new MessageBlock ((const char *)item, 
		    sizeof (EventItem)));	
	//Log::instance ()->put (LOG_RINFO, "peer recv a msg\n");
	return 0;
    }
    virtual int handle_close (NDK_HANDLE handle, ReactorMask mask)
    {
	unused_arg (handle);
	unused_arg (mask);
	this->peer ().close ();
	if (this->msg_header_)
	    delete this->msg_header_;
	this->msg_header_ = 0;
	if (this->msg_body_)
	    delete this->msg_body_;
	this->msg_body_ = 0;
	delete this;
	return 0;
    }
protected:
    volatile int msg_header_recv_pos_;
    volatile int msg_body_recv_pos_;
    MsgHeader *msg_header_;
    char *msg_body_;
    static int count_;
};
void signal_hanlder (int )
{
    msg_handle_task::instance ()->stop ();
}
int Server::count_ = 0;
int main (int argc, char *argv[])
{
    signal (SIGHUP, signal_hanlder);
    int port = 8881;
#if 1
    EpollReactor *er = new EpollReactor;
    Reactor *r = new Reactor (er);
    Reactor::instance (r);
    ThreadManager::instance ()->spawn_n (1, event_loop, Reactor::instance ());
#else
    MultiReactors<EpollReactor> *mr = new MultiReactors<EpollReactor>;
    Reactor *r = new Reactor (mr);
    Reactor::instance (r);
    if (mr->init (REACTOR_NUM_FIXED, 4) != 0) return -1;
#endif
    Log::instance ()->open ("log");
    msg_handle_task::instance ()->open ();
    Acceptor<Server> acceptor (Reactor::instance ());
    InetAddr local_addr (port);
    acceptor.open (local_addr,
	    Reactor::instance (),
	    1,
	    1024 * 1024 * 2);
    ThreadManager::instance ()->wait ();
    return 0;
}

