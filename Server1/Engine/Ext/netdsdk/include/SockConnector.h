//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-09 12:20
 */
//========================================================================

#ifndef _SOCKCONNECTOR_H_
#define _SOCKCONNECTOR_H_
#include "Pre.h"

#include "Trace.h"
#include "InetAddr.h"
#include "SockStream.h"

/**
 * @class SockConnector
 *
 * @brief 
 */
class SockConnector
{
public:
    SockConnector ();
    ~SockConnector ();

    /**
     * actively connect to a peer, producing a connected SockStream 
     * object if the connection succeeds.
     *  
     * return  0: success
     * return -1: timeout or error
     */
    int connect (SockStream &new_stream, 
	    const InetAddr &remote_addr, 
	    const TimeValue *timeout = 0,
	    const InetAddr &local_addr = InetAddr::addr_any,
	    int protocol_family = AF_INET,
	    size_t sendbuf_size = 0);
protected:
    /** 
     * perform operations that ensure the socket is opened
     */
    int shared_open (SockStream &new_stream, int protocol_family);

    /**
     * non-block connect 
     */
    int nb_connect (SockStream &new_stream,
	    const InetAddr &remote_addr,
	    const TimeValue *timeout);
private:
};

#include "SockConnector.inl"
#include "Post.h"
#endif

