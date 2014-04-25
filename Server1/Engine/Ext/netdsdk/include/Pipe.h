//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-02-14 13:24
 */
//========================================================================

#ifndef _PIPE_H_
#define _PIPE_H_
#include "Pre.h"

#include "GlobalMacros.h"

/**
 * @class Pipe
 *
 * @brief 
 */
class Pipe
{
public:
    Pipe ();
    ~Pipe ();

    //
    int open ();

    //
    int close ();
    //
    NDK_HANDLE read_handle ();
    
    // 
    NDK_HANDLE write_handle ();

private:
    NDK_HANDLE handles_[2];
};

#include "Pipe.inl"
#include "Post.h"
#endif

