//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2007-12-27 13:23
 * Revision
 * History  : 2007-12-27 - initial version
 */
//========================================================================

#ifndef _THREAD_H_
#define _THREAD_H_
#include "Pre.h"

#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <pthread.h>

#include "GlobalMacros.h"

/**
 * @class thread 
 *  
 * @brief consists of basic static functions.
 */
class ThreadManager;
class Thread  
{
    friend class ThreadManager;
public:
    enum
    {	
	THR_JOIN   = 0x00000001,
	THR_DETACH = 0x00000002
    };
    // return threadid 
    static thread_t self ();

    // 
    static int thr_equal (thread_t thr_id_1, thread_t thr_id_2);

    // send a signal to the thread.
    static int kill (thread_t, int signum);

    // yield the thread to another. drop the cpu time
    static void yield (void);

    /**
     * Cancel a thread.
     * This method is only portable on platforms, such as POSIX pthreads,
     * that support thread cancellation.
     */
    static int cancel (thread_t thr_id); 

    // test the cancel
    static void testcancel (void);
protected:
    // create a thread
    // return : 0, success , -1 failed
    static int spawn (THREAD_FUNC_T func, 
	    void *arg  = NULL,
	    thread_t *thr_id = 0,
	    int flags = THR_JOIN,
	    size_t stack_size = 0
	    );

    /** create n thread
     * return : >0 the number of create successfully
     *           0 create failed
     */
    static int spawn_n (size_t thr_num,
	    THREAD_FUNC_T func, 
	    void *arg  = NULL,
	    thread_t thr_id[] = 0,    // 
	    int flags = THR_JOIN,
	    size_t stack_size = 0   // shared
	    );

    /**
     * retval  0 for success
     * retval  -1 (with errno set) for failure.
     */
    static int join (thread_t thr_id);

    // 
    static int detach (thread_t thr_id);
    // 
private:
    Thread ();
};

#include "Thread.inl"
#include "Post.h"
#endif

