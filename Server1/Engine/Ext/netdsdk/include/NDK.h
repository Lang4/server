//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-02-08 18:35
 */
//========================================================================

#ifndef _NDK_H_
#define _NDK_H_
#include "Pre.h"

#include "TimeValue.h"
#include "GlobalMacros.h"
#include "DefaultConstants.h"

#include <fcntl.h>
#include <unistd.h>

// Previous declare
struct iovec;

namespace NDK
{
    extern int max_handles ();

    // 
    extern int set_handle_limit (int new_limit,
	    int increase_limit_only = 0);

    extern int handle_ready (NDK_HANDLE handle, 
	    int r_ready, 
	    int w_ready, 
	    int e_ready, 
	    const TimeValue *timeout = 0);

    extern int handle_read_ready  (NDK_HANDLE handle, 
	    const TimeValue *timeout = 0); 
    extern int handle_write_ready (NDK_HANDLE handle, 
	    const TimeValue *timeout = 0);
    extern int handle_exception_ready (NDK_HANDLE handle, 
	    const TimeValue *timeout = 0);

    extern int set_non_block_mode (NDK_HANDLE handle);
    extern int set_block_mode (NDK_HANDLE handle);
    extern int record_and_set_non_block_mode (NDK_HANDLE handle, 
	    int &val);
    extern int restore_non_block_mode (NDK_HANDLE handle, 
	    int val);

    extern int sleep (const TimeValue *tv);

    extern TimeValue gettimeofday ();

    // socket io
    extern int recv (NDK_HANDLE handle, 
	    void *buff, 
	    size_t len, 
	    int flags, 
	    const TimeValue *timeout);

    extern int recv (NDK_HANDLE handle, 
	    void *buff, 
	    size_t len, 
	    const TimeValue *timeout);

    extern int recv_n (NDK_HANDLE handle, 
	    void *buff, 
	    size_t len, 
	    int flags, 
	    const TimeValue *timeout);

    extern int recv_n (NDK_HANDLE handle, 
	    void *buff, 
	    size_t len, 
	    const TimeValue *timeout);
    extern int recvv (NDK_HANDLE handle, 
	    iovec iov[], 
	    size_t count, 
	    const TimeValue *timeout);

    extern int recvv_n (NDK_HANDLE handle, 
	    iovec iov[], 
	    size_t count, 
	    const TimeValue *timeout);

    extern int send (NDK_HANDLE handle, 
	    const void *buff, 
	    size_t len, 
	    int flags, 
	    const TimeValue *timeout);

    extern int send (NDK_HANDLE handle, 
	    const void *buff, 
	    size_t len, 
	    const TimeValue *timeout);

    extern int send_n (NDK_HANDLE handle, 
	    const void *buff,
	    size_t len, 
	    int flags, 
	    const TimeValue *timeout);

    extern int send_n (NDK_HANDLE handle, 
	    const void *buff, 
	    size_t len, 
	    const TimeValue *timeout);

    extern int sendv (NDK_HANDLE handle, 
	    const iovec iov[], 
	    size_t count, 
	    const TimeValue *timeout);

    extern int sendv_n (NDK_HANDLE handle, 
	    const iovec iov[], 
	    size_t count, 
	    const TimeValue *timeout);

    extern int read (NDK_HANDLE handle, 
	    void *buff, 
	    size_t len, 
	    const TimeValue *timeout);
    extern int read_n (NDK_HANDLE handle, 
	    void *buff, 
	    size_t len, 
	    const TimeValue *timeout);

    extern int write (NDK_HANDLE handle, 
	    const void *buff, 
	    size_t len, 
	    const TimeValue *timeout);

    extern int write_n (NDK_HANDLE handle, 
	    const void *buff, 
	    size_t len, 
	    const TimeValue *timeout);

    // Bind socket to an unused port.
    extern int bind_port (NDK_HANDLE handle,
	    size_t ip_addr, 
	    int addr_family);

    extern THREAD_FUNC_RETURN_T thr_run_reactor_event_loop (void *arg);
}

#include "Post.h"
#include "NDK.inl"
#endif

