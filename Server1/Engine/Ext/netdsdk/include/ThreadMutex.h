//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-05 18:35
 */
//========================================================================

#ifndef _THREADMUTEX_H_
#define _THREADMUTEX_H_
#include "Pre.h"

#include "GlobalMacros.h"

#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

/**
 * @class ThreadMutex
 *
 * @brief 
 */
class ThreadMutex
{
public:
    ThreadMutex ();
    ~ThreadMutex ();
    //
    int acquire (void);
    
    // acquire lock ownership in msec period 
    int acquire (int msec);

    // not block if someone already had the lock , errno is EBUSY
    int tryacquire (void);

    // release lock ownership 
    int release (void);

    // return mutex_t
    const pthread_mutex_t &lock () const;
private:
    pthread_mutex_t mutex_;
};

#include "ThreadMutex.inl"
#include "Post.h"
#endif

