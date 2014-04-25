#include "SockConnector.h"
#include "Debug.h"
#include "NDK.h"

#include <errno.h>

int SockConnector::connect (SockStream &new_stream,
	const InetAddr &remote_addr,
	const TimeValue *timeout/* = 0*/,
	const InetAddr &local_addr/* = InetAddr::addr_any*/,
	int protocol_family/* = AF_INET*/,
	size_t sendbuf_size/* = 0*/)
{
    TRACE ("SockConnector");
    if (this->shared_open (new_stream, protocol_family) == -1)
	return -1;
    if (local_addr != InetAddr::addr_any)
    {
	sockaddr *laddr = reinterpret_cast<sockaddr *> (local_addr.get_addr ());
	int size = local_addr.get_addr_size ();
	if (::bind (new_stream.handle (), laddr, size) == -1)
	{
	    new_stream.close ();
	    return -1;
	}
    }
    if (sendbuf_size != 0)
    {
	::setsockopt (new_stream.handle (), 
		SOL_SOCKET, SO_SNDBUF, 
		(const void*)&sendbuf_size, sizeof (sendbuf_size));
    }
    if (timeout == 0)
	return ::connect (new_stream.handle (), 
		reinterpret_cast<sockaddr *>(remote_addr.get_addr ()),
		static_cast<socklen_t> (remote_addr.get_addr_size ()));
    // timeout use nonblock connect
    return nb_connect (new_stream, remote_addr, timeout);
}
int SockConnector::nb_connect (SockStream &new_stream,
	const InetAddr &remote_addr,
	const TimeValue *timeout)
{
    TRACE ("SockConnector");
    int ret_val = 0;
    int err_num = 0;
    NDK::set_non_block_mode (new_stream.handle ());	
    ret_val = ::connect (new_stream.handle (),
	    reinterpret_cast<sockaddr *>(remote_addr.get_addr ()),
	    static_cast<socklen_t> (remote_addr.get_addr_size ()));
    err_num = errno;
    if (ret_val == -1)
    {
	if (err_num == EINPROGRESS || err_num == EWOULDBLOCK)
	{
	    ret_val = NDK::handle_ready (new_stream.handle (), 
		    1, 
		    1, 
		    0, 
		    timeout);
	    if (ret_val == 0) // timeout
	    {
		ret_val = -1; // error
	    }else if (ret_val > 0)
	    {
		socklen_t len = sizeof (ret_val);
		if (::getsockopt (new_stream.handle (), 
			    SOL_SOCKET, 
			    SO_ERROR, 
			    &ret_val, 
			    &len) < 0
		    || ret_val != 0)
		    ret_val = -1;
	    }// failed
	}
    }
    NDK::set_block_mode (new_stream.handle ()); // restore
    if (ret_val == -1) new_stream.close ();
    return ret_val;
}

