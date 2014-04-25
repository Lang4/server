//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-04 18:38
 */
//========================================================================

#ifndef _SOCK_STREAM_H_
#define _SOCK_STREAM_H_
#include "Pre.h"

#include "Socket.h"

/**
 * @class SockStream
 *
 * @brief 
 */
class TimeValue;
class SockStream : public Socket
{
public:
    SockStream ();
    SockStream (NDK_HANDLE handle);
    ~SockStream ();
public:
    /**
     * block indefinable if msec = -1 
     * block timeout if msec > 0
     * not block if msec = -1
     */

    /**
     * peer closed connection return 0
     * time out or error return -1, buff is NULL 
     * actual recv bytes return > 0
     */
    int recv (void *buff, 
	    size_t len, 
	    int flags, 
	    const TimeValue *timeout = 0);
    /**
     * peer closed connection return 0
     * time out or error return -1 (may be buff has data but not completely)
     * actual recv bytes return > 0
     */
    int recv_n (void *buff, 
	    size_t len, 
	    int flags, 
	    const TimeValue *timeout = 0);

    //
    int recv (void *buff, 
	    size_t len, 
	    const TimeValue *timeout = 0);
    int recv_n (void *buff, 
	    size_t len, 
	    const TimeValue *timeout = 0);

    // 
    int recvv (iovec iov[], 
	    size_t count, 
	    const TimeValue *timeout = 0);
    int recvv_n (iovec iov[], 
	    size_t count, 
	    const TimeValue *timeout = 0);

    //
    int send (const void *buff, 
	    size_t len, 
	    int flags, 
	    const TimeValue *timeout = 0);
    int send_n (const void *buff, 
	    size_t len, 
	    int flags, 
	    const TimeValue *timeout = 0);

    //
    int send (const void *buff, 
	    size_t len, 
	    const TimeValue *timeout = 0);
    int send_n (const void *buff, 
	    size_t len, 
	    const TimeValue *timeout = 0);

    //
    int sendv (const iovec iov[], 
	    size_t count, 
	    const TimeValue *timeout = 0);
    int sendv_n (const iovec iov[], 
	    size_t count, 
	    const TimeValue *timeout = 0);
};

#include "SockStream.inl"
#include "Post.h"
#endif

