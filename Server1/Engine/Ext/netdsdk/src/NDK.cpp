#include "NDK.h"
#include "Trace.h"
#include "InetAddr.h"
#include "ReactorImpl.h"

#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/resource.h>

int NDK::max_handles ()
{
    TRACE ("NDK");
    struct rlimit rl;
    ::memset ((void *)&rl, 0, sizeof (rl));
    int r = ::getrlimit (RLIMIT_NOFILE, &rl);
#if !defined (RLIM_INFINITY)
    if (r == 0)
	return rl.rlim_cur;
#else
    if (r == 0 && rl.rlim_cur != RLIM_INFINITY)
	return rl.rlim_cur;
#endif
    return -1;
}
int NDK::set_handle_limit (int new_limit, 
	int increase_limit_only/* = 0*/)
{
    TRACE ("NDK");
    int cur_limit = NDK::max_handles ();
    int max_limit = cur_limit;
    if (cur_limit == -1)
	return -1;
    struct rlimit rl;
    ::memset ((void *)&rl, 0, sizeof (rl));
    int r = ::getrlimit (RLIMIT_NOFILE, &rl);
    if (r == 0)
	max_limit = rl.rlim_max;
    if (new_limit < 0)
	return -1;

    if (new_limit > cur_limit)
    {
	rl.rlim_cur = new_limit;
	return ::setrlimit (RLIMIT_NOFILE, &rl);
    }
    else if (increase_limit_only == 0)
    {
	rl.rlim_cur = new_limit;
	return ::setrlimit (RLIMIT_NOFILE, &rl);
    }
    return 0;
}
//************************************I/O Operation*****************************
int NDK::handle_ready (NDK_HANDLE handle, 
	int r_ready, 
	int w_ready, 
	int e_ready, 
	const TimeValue* timeout) 
{
    int width = handle + 1; //
    fd_set rset, wset, eset;

    // must define here and not define in while loop
    struct timeval tv;
    if (timeout != 0)
    {
	tv = *timeout;
    }
    int ret_val = 0;
    while (1)
    {
	FD_ZERO (&rset);
	FD_SET (handle, &rset);
	ret_val = ::select (width, r_ready ? &rset : 0, 
		w_ready ? &(wset = rset) : 0, 
		e_ready ? &(eset = rset) : 0,
		timeout == 0 ? 0 : &tv);
	// not clear the fd_set return -1, POSIX 1.0v: timeval would be modify
	// clear all the fd_set return 0 (timeout)
	if ((ret_val == -1) && (errno == EINTR))// interupt by signal
	    continue;
	break;  // timeout or ready
    }
    return ret_val;
}
int NDK::recv (NDK_HANDLE handle, 
	void *buff, 
	size_t len, 
	int flags, 
	const TimeValue *timeout) 
{
    if (handle == NDK_INVALID_HANDLE)
	return -1;
    // complete block
    if (timeout == 0)  // block mode, handle by tcp stack
	return ::recv (handle, buff, len, flags);
   
    // set nonblock mode
    int flag = 0;
    NDK::record_and_set_non_block_mode (handle, flag);

    // timeout
    if (*timeout > TimeValue::zero)
	if (NDK::handle_read_ready (handle, timeout) <= 0) 
	    // timeout not do recv
	{
	    // reset to block mode
	    NDK::restore_non_block_mode (handle, flag);
	    return -1;
	}
    int ret = ::recv (handle, buff, len, flags);
    // reset to block mode
    NDK::restore_non_block_mode (handle, flag);
    return ret; // return -1 if no data in tcp stack buff under non-block mode
}
int NDK::recv_n (NDK_HANDLE handle, 
	void *buff, 
	size_t len, 
	int flags, 
	const TimeValue *timeout) 
{
    if (handle == NDK_INVALID_HANDLE)
	return -1;
    ssize_t n = 0;
    size_t recv_bytes = 0;
    for (; recv_bytes < len; recv_bytes += n)
    {
	n = ::recv (handle, buff, len - recv_bytes, flags);
	if (n == 0)  // peer closed
	    return 0;
	if (n == -1)  // check for errors
	{
	    // check for possible blocking.
	    if (errno == EWOULDBLOCK || errno == EAGAIN)
	    {
		if (timeout != 0 
			&& *timeout > TimeValue::zero)
		{
		    // note msec not be modifyed. so not exact
		    int ret = NDK::handle_read_ready (handle, timeout);  
		    if (ret <= 0) // time out or not ready handle
			return -1;
		}
		n = 0;  // !!
		continue;
	    }else if (errno == EINTR) 
	    {
	       	n = 0;  // !!
		continue;  
	    }
	    // other error info
	    return -1;
	}
    }
    return recv_bytes;
}
int NDK::recvv (NDK_HANDLE handle, 
	iovec iov[], 
	size_t count, 
	const TimeValue *timeout) 
{
    if (handle == NDK_INVALID_HANDLE)
	return -1;
    if (timeout == 0)
	return ::readv (handle, iov, count);

    // set nonblock mode
    int flag = 0;
    NDK::record_and_set_non_block_mode (handle, flag);

    // timeout
    if (*timeout > TimeValue::zero)
	if (NDK::handle_read_ready (handle, timeout) <= 0) 
	    // timeout not do recv
	{
	    // reset to block mode
	    NDK::restore_non_block_mode (handle, flag);
	    return -1;
	}
    int ret = ::readv (handle, iov, count);
    // reset to block mode
    NDK::restore_non_block_mode (handle, flag);
    return ret;
}
int NDK::recvv_n (NDK_HANDLE handle, 
	iovec iv[], 
	size_t count, 
	const TimeValue *timeout) 
{
    if (handle == NDK_INVALID_HANDLE)
	return -1;
    ssize_t n = 0;
    size_t recv_bytes = 0;
#if IOVEC_SECURITY_IO
    // for bak
    struct iovec *iov = iv;
    struct iovec *iov_bak = 
	(struct iovec*)::malloc (sizeof(struct iovec) * count);
    for (size_t i = 0; i < count; ++i)
	iov_bak[i] = iov[i]; 
    bool bfull = true;
#else
    struct iovec *iov = iv;
#endif
    for (size_t s = 0; s < count;)
    {
	n = ::readv (handle, iov + s, count - s);
	if (n == 0)  // peer closed
	    return 0;
	if (n == -1)  // check for errors
	{
	    // check for possible blocking.
	    if (errno == EAGAIN || errno == EWOULDBLOCK)
	    {
		if (timeout != 0 
			&& *timeout > TimeValue::zero)
		{
		    // note msec not be modifyed. so not exact
		    int ret = NDK::handle_read_ready (handle, timeout);  
		    if (ret <= 0) // time out or not ready handle
			return -1;
		}
		n = 0;  // !!
		continue;
	    }else if (errno == EINTR) 
	    {
	       	n = 0;  // !!
		continue;  
	    }
	    // other error info
	    return -1;
	}
	for (recv_bytes += n; 
		s < count && n >= static_cast<int>(iov[s].iov_len); 
		++s)
	    n -= iov[s].iov_len;
#if IOVEC_SECURITY_IO
	if (n != 0)
	{
	    bfull = false;
	    char *base = static_cast<char *> (iov[s].iov_base);
	    iov[s].iov_base = base + n;
	    iov[s].iov_len  = iov[s].iov_len - n;
	}
#endif
    }
#if IOVEC_SECURITY_IO
    if (!bfull)
    {
	for (size_t i = 0; i < count; ++i)
	    iov[i] = iov_bak[i]; 
    }
    ::free (iov_bak);
#endif
    return recv_bytes;
}
int NDK::send (NDK_HANDLE handle, 
	const void *buff, 
	size_t len, 
	int flags, 
	const TimeValue *timeout) 
{
    if (handle == NDK_INVALID_HANDLE)
	return -1;
    if (timeout == 0)
	return ::send (handle, buff, len, flags);
     
    // set nonblock mode
    int flag = 0;
    NDK::record_and_set_non_block_mode (handle, flag);

    // try it first
    int ret = ::send (handle, buff, len, flags);
    if (ret == -1 
	    && (errno == EAGAIN 
		|| errno == EWOULDBLOCK 
		|| errno == ENOBUFS))
    {
	// timeout
	if (*timeout > TimeValue::zero)
	{
	    if (NDK::handle_read_ready (handle, timeout) > 0) 
		// <= 0 timeout not do send 
		// > 0 try it again
		ret = ::send (handle, buff, len, flags);
	}
    }
    // reset to block mode
    NDK::restore_non_block_mode (handle, flag);
    return ret;
}
int NDK::send_n (NDK_HANDLE handle, 
	const void *buff, 
	size_t len, 
	int flags, 
	const TimeValue* timeout) 
{
    if (handle == NDK_INVALID_HANDLE)
	return -1;
    ssize_t n = 0;
    size_t send_bytes = 0;
    // set nonblock mode
    int flag = 0;
    NDK::record_and_set_non_block_mode (handle, flag);
    for (; send_bytes < len; send_bytes += n)
    {
	n = ::send (handle, buff, len - send_bytes, flags);
	if (n == 0)  // peer closed
	{
	    // reset to block mode
	    NDK::restore_non_block_mode (handle, flag);
	    return 0;
	}
	if (n == -1)  // check for errors
	{
	    // check for possible blocking.
	    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS)
	    {
		if (timeout != 0
			&& *timeout > TimeValue::zero)
		{
		    // note msec not be modifyed. so not exact
		    int ret = NDK::handle_write_ready (handle, timeout);  
		    if (ret <= 0) // time out or not ready handle
		    {
			// reset to block mode
			NDK::restore_non_block_mode (handle, flag);
			return -1;
		    }
		}
		n = 0;  // !!
		continue;
	    }else if (errno == EINTR) 
	    {
	       	n = 0;  // !!
		continue;  
	    }

	    // other error info
	    // reset to block mode
	    NDK::restore_non_block_mode (handle, flag);
	    return -1;
	}
    }
    // reset to block mode
    NDK::restore_non_block_mode (handle, flag);
    return send_bytes;
}
int NDK::sendv (NDK_HANDLE handle, 
	const iovec iov[], 
	size_t count, 
	const TimeValue *timeout) 
{
    if (handle == NDK_INVALID_HANDLE)
	return -1;
    if (timeout == 0)
	return ::writev (handle, iov, count);

    // set nonblock mode
    int flag = 0;
    NDK::record_and_set_non_block_mode (handle, flag);

    // try it first
    int ret = ::writev (handle, iov, count);
    if (ret == -1 && 
	    (errno == EAGAIN 
	     || errno == EWOULDBLOCK 
	     || errno == ENOBUFS))
    {
	// timeout
	if (*timeout > TimeValue::zero)
	{
	    if (NDK::handle_write_ready (handle, timeout) > 0) // <= 0 timeout not do writev 
		// try it again
		ret = ::writev (handle, iov, count);
	}
    }
    // reset to block mode
    NDK::restore_non_block_mode (handle, flag);
    return ret;
}
int NDK::sendv_n (NDK_HANDLE handle, 
	const iovec iov_[], 
	size_t count, 
	const TimeValue *timeout) 
{
    if (handle == NDK_INVALID_HANDLE)
	return -1;
    ssize_t n = 0;
    size_t send_bytes = 0;
#if IOVEC_SECURITY_IO
    // for bak
    struct iovec *iov = const_cast<iovec*>(iov_);
    struct iovec *iov_bak = (struct iovec*)::malloc (sizeof(struct iovec) * count);
    for (size_t i = 0; i < count; ++i)
	iov_bak[i] = iov[i]; 
    bool bfull = true;
#else
    struct iovec *iov = const_cast<iovec*>(iov_);
#endif
    for (size_t s = 0; s < count;)
    {
	n = ::writev (handle, iov + s, count - s);
	if (n == 0)  // peer closed
	    return 0;
	if (n == -1)  // check for errors
	{
	    // check for possible blocking.
	    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS)
	    {
		if (*timeout > TimeValue::zero)
		{
		    // note msec not be modifyed. so not exact
		    int ret = NDK::handle_write_ready (handle, timeout);  
		    if (ret <= 0) // time out or not ready handle
			return -1;
		}
		n = 0;  // !!
		continue;
	    }else if (errno == EINTR) 
	    {
	       	n = 0;  // !!
		continue;  
	    }
	    // other error info
	    return -1;
	}
	for (send_bytes += n; 
		s < count && n >= static_cast<int>(iov[s].iov_len); 
		++s)
	    n -= iov[s].iov_len;
#if IOVEC_SECURITY_IO
	if (n != 0)
	{
	    bfull = false;
	    char *base = static_cast<char *> (iov[s].iov_base);
	    iov[s].iov_base = base + n;
	    iov[s].iov_len  = iov[s].iov_len - n;
	}
#endif
    }
#if IOVEC_SECURITY_IO
    if (!bfull)
    {
	for (size_t i = 0; i < count; ++i)
	    iov[i] = iov_bak[i]; 
    }
    ::free (iov_bak);
#endif
    return send_bytes;
}
int NDK::read (NDK_HANDLE handle, 
	void *buff, 
	size_t len, 
	const TimeValue *timeout)
{
    if (handle == NDK_INVALID_HANDLE)
	return -1;
    // complete block
    if (timeout == 0)  // block mode, handle by tcp stack
	return ::read (handle, buff, len);
   
    // set nonblock mode
    int flag = 0;
    NDK::record_and_set_non_block_mode (handle, flag);

    // timeout
    if (*timeout > TimeValue::zero)
	if (NDK::handle_read_ready (handle, timeout) <= 0) 
	    // timeout not do recv
	{
	    NDK::restore_non_block_mode (handle, flag);
	    return -1;
	}
    int ret = ::read (handle, buff, len);
    // reset to block mode
    NDK::restore_non_block_mode (handle, flag);
    return ret; // return -1 if no data in tcp stack buff under non-block mode
}
int NDK::read_n (NDK_HANDLE handle, 
	void *buff, 
	size_t len, 
	const TimeValue *timeout) 
{
    if (handle == NDK_INVALID_HANDLE)
	return -1;
    ssize_t n = 0;
    size_t recv_bytes = 0;
    for (; recv_bytes < len; recv_bytes += n)
    {
	n = ::read (handle, buff, len - recv_bytes);
	if (n == 0)  // peer closed
	    return 0;
	if (n == -1)  // check for errors
	{
	    // check for possible blocking.
	    if (errno == EWOULDBLOCK || errno == EAGAIN)
	    {
		if (*timeout > TimeValue::zero)
		{
		    // note msec not be modifyed. so not exact
		    int ret = NDK::handle_read_ready (handle, timeout);  
		    if (ret <= 0) // time out or not ready handle
			return -1;
		}
		n = 0;  // !!
		continue;
	    }else if (errno == EINTR) 
	    {
	       	n = 0;  // !!
		continue;  
	    }
	    // other error info
	    return -1;
	}
    }
    return recv_bytes;
}
int NDK::write (NDK_HANDLE handle, 
	const void *buff, 
	size_t len, 
	const TimeValue *timeout) 
{
    if (handle == NDK_INVALID_HANDLE)
	return -1;
    if (timeout == 0)
	return ::write (handle, buff, len);
     
    // set nonblock mode
    int flag = 0;
    NDK::record_and_set_non_block_mode (handle, flag);

    // try it first
    int ret = ::write (handle, buff, len);
    if (ret == -1 && 
	    (errno == EAGAIN 
	     || errno == EWOULDBLOCK 
	     || errno == ENOBUFS))
    {
	// timeout
	if (*timeout > TimeValue::zero)
	{
	    if (NDK::handle_read_ready (handle, timeout) > 0) 
		// <= 0 timeout not do recv
		// = 0 try it again
		ret = ::write (handle, buff, len);
	}
    }
    // reset to block mode
    NDK::restore_non_block_mode (handle, flag);
    return ret;
}
int NDK::write_n (NDK_HANDLE handle, 
	const void *buff, 
	size_t len, 
	const TimeValue *timeout) 
{
    if (handle == NDK_INVALID_HANDLE)
	return -1;
    ssize_t n = 0;
    size_t send_bytes = 0;
    // set nonblock mode
    int flag = 0;
    NDK::record_and_set_non_block_mode (handle, flag);
    for (; send_bytes < len; send_bytes += n)
    {
	n = ::write (handle, buff, len - send_bytes);
	if (n == 0)  // peer closed
	    return 0;
	if (n == -1)  // check for errors
	{
	    // check for possible blocking.
	    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS)
	    {
		if (*timeout > TimeValue::zero)
		{
		    // note msec not be modifyed. so not exact
		    int ret = NDK::handle_write_ready (handle, timeout);  
		    if (ret <= 0) // time out or not ready handle
			return -1;
		}
		n = 0;  // !!
		continue;
	    }else if (errno == EINTR) 
	    {
		n = 0;  // !!
		continue;  
	    }
	    // other error info
	    return -1;
	}
    }
    // reset to block mode
    NDK::restore_non_block_mode (handle, flag);
    return send_bytes;
}
int NDK::bind_port (NDK_HANDLE handle, 
	size_t ip_addr, 
	int addr_family)
{
    InetAddr addr;
    unused_arg (addr_family);
#if 1
    return ::bind (handle, (sockaddr *)addr.get_addr (),
	    addr.get_addr_size ());
#endif
    unsigned short upper_limit = MAX_DEFAULT_PORT;
    // We have to select the port explicitly.
    for (;;)
    {
	addr.set (upper_limit, ip_addr);
	if (::bind (handle, (sockaddr *)addr.get_addr (),
		    addr.get_addr_size ()) >= 0)
	    return 0;
	else if (errno != EADDRINUSE)
	    return -1;
	else
	{
	    --upper_limit;
	}
    }
}
THREAD_FUNC_RETURN_T NDK::thr_run_reactor_event_loop (void *arg)
{
    ReactorImpl *r = reinterpret_cast<ReactorImpl *>(arg);
    if (r == 0)
	return 0;
    r->owner (Thread::self ());

    while (1)
    {
	int result = r->handle_events ();
	if (result == -1 && r->deactivated ())
	    return 0;
	else if (result == -1)
	    return 0;
    }
    return 0;
}


