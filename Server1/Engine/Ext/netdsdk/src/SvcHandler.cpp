#include "SvcHandler.h"
#include "Trace.h"
#include "Debug.h"

SvcHandler::SvcHandler (Reactor *r/* = Reactor::instance ()*/)
: closed_ (0)
{
    this->reactor (r);
}
SvcHandler::~SvcHandler ()
{
    if (this->closed_ == 0)
    {
	this->closed_ = 1;
	this->shutdown ();
    }
}
// example code
int SvcHandler::open (void *arg/* = 0*/)
{
    unused_arg(arg);
    if (this->reactor () 
	    && this->reactor ()->register_handler
	    (this, 
	     EventHandler::READ_MASK) == -1)
    {
	NDK_INF ("register handler %d failed\n", this->handle ());
	return -1;
    }
    return 0;
}
int SvcHandler::close ()
{
    return this->handle_close ();
}
int SvcHandler::handle_close (NDK_HANDLE, ReactorMask)
{
    this->destroy ();
    return 0;
}
SockStream &SvcHandler::peer () const
{
    return (SockStream &)this->peer_;
}
NDK_HANDLE SvcHandler::handle () const
{
    return this->peer_.handle ();
}
void SvcHandler::handle (NDK_HANDLE handle)
{
    this->peer_.handle (handle);
}
void SvcHandler::destroy ()
{
    if (this->closed_ == 0)
    {
	// Will call the destructor, which automatically calls 
	// <shutdown>. 
	// <SvcHandler> is allocate by Acceptor or Connector
	delete this;
    }
}
void SvcHandler::shutdown (void)
{
    if (this->reactor ())
    {
	ReactorMask mask = EventHandler::ALL_EVENTS_MASK
	    | EventHandler::DONT_CALL;
	
	if (this->peer ().handle () != NDK_INVALID_HANDLE)
	    this->reactor ()->remove_handler (this, mask);
    }
    this->peer ().close ();
}


