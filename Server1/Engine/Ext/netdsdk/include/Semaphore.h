//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-05 18:36
 */
//========================================================================

#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_
#include "Pre.h"

#include <errno.h>
#include <semaphore.h>

#include "NDK.h"
#include "Thread.h"          // For Thread::yield ()
#include "TimeValue.h"

/**
 * @class Semaphore
 *
 * @brief 
 */
class Semaphore
{
public:
    Semaphore (int init_num = 0);
    ~Semaphore ();

    // block until wakeup by sem
    // return -1: error
    // return  0: wakeup by sem
    int wait (void);

    // block until timeout
    // return  0: wakeup by sem
    // return -1: error
    //          : errno = EWOULDBLOCK means timeout
    int wait (const TimeValue *timeout);

    // return  0: sem count > 0
    // return -1: sem count = 0
    int trywait ();
    
    // return -1: failed
    // return  0: succ
    int post ();

    // return -1: failed
    // other    : post count successfully  
    int post (int post_count);
private:
    sem_t sem_;
};

#include "Semaphore.inl"
#include "Post.h"
#endif
