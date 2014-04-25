//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-09 12:20
 */
//========================================================================

#ifndef _SOCKACCEPTOR_H_
#define _SOCKACCEPTOR_H_
#include "Pre.h"

#include "config.h"
#include "Socket.h"
#include "InetAddr.h"
#include "SockStream.h"

/**
 * @class SockAcceptor
 *
 * @brief 
 */
class SockAcceptor : public Socket
{
public:
    SockAcceptor ();
    ~SockAcceptor ();

    /**
     * Initialize a passive-mode BSD-style acceptor socket
     * <local_addr> is the address that we're going to listen for
     * connections on.  If <reuse_addr> is 1 then we'll use the
     * <SO_REUSEADDR> to reuse this address.  Returns 0 on success and
     * -1 on failure.
     */
    int open (const InetAddr &local_addr, 
	    int reuse_addr = 1,
	    size_t revbuf_size = 0,
	    int protocol_family = AF_INET,
	    int backlog = NDK_DEFAULT_BACKLOG);

    /**
     * Accept a new SockStream connection.  timeout_msec is 0
     * means non-block, = -1 means block forever, > 0 means timeout
     */
    int accept (SockStream &new_stream,
	    InetAddr *remote_addr = 0,
	    const TimeValue *timeout = 0);
};

#include "Post.h"
#endif

