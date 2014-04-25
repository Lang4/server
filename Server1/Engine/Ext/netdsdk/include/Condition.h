//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-05 18:36
 */
//========================================================================

#ifndef _CONDITION_H_
#define _CONDITION_H_
#include "Pre.h"

#include "NDK.h"
#include "Thread.h"          // For Thread::yield ()
#include "TimeValue.h"

#include <errno.h>

/**
 * @class Condition
 *
 * @brief 
 */
template<typename MUTEX>
class Condition
{
public:
    // Constructor
    Condition (MUTEX &m);

    // Destory cond
    ~Condition ();

    /**
     * block until wakeup by signal
     * return  0: wakeup by signal
     * return -1: error
     */
    int wait (void);

    /**
     * block until timeout
     * return  0: wakeup by signal
     * return -1: error or timeout
     *          : errno == ETIMEDOUT means timeout
     */
    int wait (const TimeValue *timeout);

    /**
     * signal one waiting thread.
     * <man pthread_cond_signal: The pthread_cond_signal() function shall  
     * unblock at least one of the threads that are blocked on the specified 
     * condition variable cond>
     * return  0: signal succ
     * return -1: failed
     */
    int signal (void);

    /**
     * signal *all* waiting threads.
     * return  0: succ
     * return -1: failed
     */
    int broadcast (void);

    // Return the reference of mutex
    MUTEX &mutex (void);
private:
    // Mutex
    MUTEX &mutex_;

    // Condition
    pthread_cond_t cond_;
};

#include "Condition.inl"
#include "Post.h"
#endif

