//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-27 14:35
 */
//========================================================================

#ifndef _RECURSIVETHREADMUTEX_H_
#define _RECURSIVETHREADMUTEX_H_
#include "Pre.h"

#include <sys/time.h>

#include "GlobalMacros.h"

/**
 * @class RecursiveThreadMutex
 *
 * @brief 
 */
class RecursiveThreadMutex
{
public:
    RecursiveThreadMutex ();
    ~RecursiveThreadMutex ();
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
    //
    int  nesting_level_;

    // 
    thread_t owner_thr_;

    pthread_mutex_t mutex_;
};

#include "RecursiveThreadMutex.inl"
#include "Post.h"
#endif

