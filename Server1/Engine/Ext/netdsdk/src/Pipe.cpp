#include "Pipe.h"
#include "Debug.h"
#include "SockAcceptor.h"
#include "SockConnector.h"

#include <unistd.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

Pipe::Pipe ()
{
    handles_[0] = NDK_INVALID_HANDLE;
    handles_[1] = NDK_INVALID_HANDLE;
}
Pipe::~Pipe ()
{
    this->close ();
}
int Pipe::open ()
{
#if 0
    InetAddr local_any = InetAddr::addr_any;
    InetAddr local_addr;
    SockAcceptor acceptor;
    SockConnector connector;
    SockStream writer;
    SockStream reader;
    int result = 0;
    // Bind listener to any port and then find out what the port was.
    if (acceptor.open (local_any) < 0
	    || acceptor.get_local_addr (local_addr) != 0)
    {
	result = -1;
    }else
    {
	InetAddr svr_addr(local_addr.get_port_number (), "127.0.0.1");
	// Establish a connection within the same process.
	if (connector.connect (writer, svr_addr) != 0)
	    result = -1;
	else if (acceptor.accept (reader) != 0)
	{
	    writer.close ();
	    result = -1;
	}
    }
    // Close down the acceptor endpoint since we don't need it anymore.
    acceptor.close ();
    if (result == -1)
	return -1;
    this->handles_[0] = reader.handle ();
    this->handles_[1] = writer.handle ();
    int on = 1;
    if (::setsockopt (this->handles_[1], IPPROTO_TCP, TCP_NODELAY,
		&on,
		sizeof(on)) == -1)
    {
	this->close();
	return -1;
    }
    return 0;
#endif
#if 0
    if (::socketpair (AF_UNIX, SOCK_STREAM, 0, this->handles_) == -1)
    {
	NDK_DBG ("socketpair");
	return -1;
    }
    return 0;
#endif
    if (::pipe (this->handles_) == -1)
    {
	NDK_DBG ("pipe");
	return -1;
    }
    // Enable "msg no discard" mode, which ensures that record
    // boundaries are maintained when messages are sent and received.
#if 0
    int arg = RMSGN;
    if (::ioctl (this->handles_[0], 
		I_SRDOPT,
		(void*)arg) == -1
	    ||
	    ::ioctl (this->handles_[1],
		I_SRDOPT,
		(void*)arg) == -1
       )
    {
	NDK_DBG ("ioctl pipe handles");
	this->close ();
	return -1;
    }
#endif
    return 0;
}
int Pipe::close ()
{
    int result = 0;
    if (this->handles_[0] != NDK_INVALID_HANDLE)
	result = ::close (this->handles_[0]);
    handles_[0] = NDK_INVALID_HANDLE;

    if (this->handles_[1] != NDK_INVALID_HANDLE)
	result |= ::close (this->handles_[1]);
    handles_[1] = NDK_INVALID_HANDLE;
    return result;
}


