#include "SockAcceptor.h"
#include "Debug.h"

#include <errno.h>

SockAcceptor::SockAcceptor ()
{
}
SockAcceptor::~SockAcceptor ()
{
}
int SockAcceptor::open (const InetAddr &local_addr,
	int reuse_addr/* = 1*/,
	size_t recvbuf_size/* = 0*/,
	int protocol_family/* = AF_INET*/,
	int backlog/* = NDK_DEFAULT_BACKLOG*/)
{
    TRACE ("SockAcceptor");
    if (Socket::open (SOCK_STREAM, protocol_family) == -1)
	return -1;

    int error = 0;
    if (recvbuf_size != 0)
	::setsockopt (this->handle (), 
		SOL_SOCKET, 
		SO_RCVBUF, 
		(const void*)&recvbuf_size, 
		sizeof (recvbuf_size));
    if (reuse_addr)
	::setsockopt (this->handle (), 
		SOL_SOCKET, 
		SO_REUSEADDR, 
		(const void*)&reuse_addr, 
		sizeof(reuse_addr));
    // create ok
    if (protocol_family == AF_INET)
    {
	sockaddr_in local_inet_addr;
	::memset (reinterpret_cast<void *> (&local_inet_addr),
		0,
		sizeof (local_inet_addr));
	if (local_addr == InetAddr::addr_any)
	{
	    local_inet_addr.sin_port  = 0;
	}
	else
	{
	    local_inet_addr = *reinterpret_cast<sockaddr_in *>(
		    local_addr.get_addr ());
	}
	if (local_inet_addr.sin_port == 0)
	{
	    if (NDK::bind_port (this->handle (), 
			ntohl(local_inet_addr.sin_addr.s_addr), 
			protocol_family) == -1)
	    {
		NDK_DBG ("bind_port failed");
		error = 1;
	    }
	}else
	    if (::bind (this->handle (),
			reinterpret_cast<sockaddr *> (&local_inet_addr),
			sizeof (local_inet_addr)) == -1)
	    {
		NDK_DBG ("bind failed");
		error = 1;
	    }
    }
    else
	error = 1;   // no support
    if (error != 0 || ::listen (this->handle (), backlog) == -1)
    {
	NDK_DBG ("listen failed");
	this->close ();
	return -1;
    }
    return 0;
}   
int SockAcceptor::accept (SockStream &new_stream,
	InetAddr *remote_addr/* = 0*/,
	const TimeValue *timeout/* = 0*/)
{
    TRACE ("SockAcceptor");
    int block_mode   = 0;
    int *len_ptr     = 0;
    sockaddr *addr   = 0;
    int len          = 0;
    if (remote_addr != 0)
    {
	len     = remote_addr->get_addr_size ();
	len_ptr = &len;
	addr    = (sockaddr *) remote_addr->get_addr ();
    }
    if (timeout != 0)
    {
	NDK::record_and_set_non_block_mode (this->handle (), 
		block_mode);  // 1. set
	if (*timeout > TimeValue::zero)
	{
	    if (NDK::handle_read_ready (this->handle (), 
			timeout) == -1)  // timeout or error
	    {
		return -1;
	    }
	}
	new_stream.handle (::accept (this->handle (), 
		    addr, 
		    reinterpret_cast<socklen_t*>(len_ptr)));
	NDK::restore_non_block_mode (this->handle (), 
		block_mode);  // 2. restore
    }else
    {
	do
	{
	    new_stream.handle (::accept (this->handle (), 
			addr, 
			reinterpret_cast<socklen_t*>(len_ptr)));
	}while (new_stream.handle () == NDK_INVALID_HANDLE
		&& errno == EINTR
		&& timeout == 0);
    }
    if (new_stream.handle () != NDK_INVALID_HANDLE
	    && remote_addr != 0)
    {
	if (addr)
	{
	    remote_addr->set_type (addr->sa_family);
	}
    }
    return new_stream.handle () != NDK_INVALID_HANDLE ? 0 : -1;
}


