//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-04 18:15
 */
//========================================================================

#ifndef _SOCKET_H_
#define _SOCKET_H_
#include "Pre.h"

#include "NDK.h"
#include "InetAddr.h"
#include "GlobalMacros.h"

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * @class Socket
 *
 * @brief 
 */
class Socket
{
public:
    ~Socket ();
    //
    int open (int sock_type, int protocol_family);

    // 
    int close ();

    //
    NDK_HANDLE handle () const;

    //
    void handle (NDK_HANDLE handle);
    
    // 
    int get_local_addr (InetAddr &);
protected:
    Socket ();

    // socket handle
    NDK_HANDLE handle_;
private:
};

#include "Socket.inl"
#include "Post.h"
#endif

