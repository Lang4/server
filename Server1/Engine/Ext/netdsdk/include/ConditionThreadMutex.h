//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-02-20 20:48
 */
//========================================================================

#ifndef _CONDITIONTHREADMUTEX_H_
#define _CONDITIONTHREADMUTEX_H_
#include "Pre.h"

#include <errno.h>

#include "NDK.h"
#include "Thread.h"
#include "TimeValue.h"
#include "ThreadMutex.h"

/**
 * @class ConditionThreadMutex
 *
 * @brief 
 */
class ConditionThreadMutex
{
public:
    // 
    ConditionThreadMutex (ThreadMutex &mutex);

    //
    ~ConditionThreadMutex ();

    /**
     * block until wakeup by signal
     * return  0: wakeup by signal
     * return -1: error
     */
    int wait ();

    /**
     * block until timeout
     * return  0: wakeup by signal
     * return -1: error or timeout
     *          : errno == EWOULDBLOCK means timeout
     */
    int wait (const TimeValue *timeout);
    //
    int signal ();

    //
    int broadcast ();

    // Return the reference of mutex
    ThreadMutex &mutex (void);
private:
    ThreadMutex     &mutex_;
    pthread_cond_t   cond_;
};

#include "ConditionThreadMutex.inl"
#include "Post.h"
#endif

