//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-24 22:32
 */
//========================================================================

#ifndef _TASKBASE_H_
#define _TASKBASE_H_
#include "Pre.h"

#include "EventHandler.h"
#include "ThreadManager.h"

/**
 * @class TaskBase
 *
 * @brief 
 */
class TaskBase : public EventHandler
{
public:
    TaskBase (ThreadManager *thr_mgr = 0);
    virtual ~TaskBase ();

    //
    virtual int open (void *args = 0);

    /**
     * Hook called when during thread exit normally,
     * In general, this method shouldn't be called directly 
     * by an application, particularly if the <Task> is 
     * running as an Active Object.Instead, a special message 
     * should be passed into the <Task> via the <put> method 
     * defined below, and the <svc> method should interpret 
     * this as a flag to shutdown the <Task>.
     */
    virtual int close ();
    
    // Run by a daemon thread to handle deferred processing.
    virtual int svc () = 0;

    /**
     * return 0  success
     * return -1 failed
     */
    virtual int activate (int flags = Thread::THR_JOIN,
	    size_t n_threads = 1,
	    thread_t thread_ids[] = 0,
	    int append_thr = 0,
	    size_t stack_size = 0,
	    int grp_id = -1);

    // Get the thread manager associated with this Task.
    ThreadManager *thr_mgr (void) const;

    // 
    void thr_mgr (ThreadManager *thr_mgr);

    // Returns the number of threads currently running within a task.
    size_t thr_count (void) const;

    int get_grp_id (void) const;

    // retval 0  Success.
    // retval -1 Failure
    virtual int wait ();
private:
    // Routine that runs the service routine as a daemon thread.
    static THREAD_FUNC_RETURN_T svc_run (void *);

    // clean up
    void cleanup (void);

    // Disallow these operations.
    TaskBase &operator= (const TaskBase &);
    TaskBase (const TaskBase &);
protected:
    // Count of the number of threads running within the task. 
    size_t thr_count_;
    int    grp_id_;

    // Multi-threading manager.
    ThreadManager *thr_mgr_;

    typedef ThreadMutex LOCK;
    LOCK   lock_;
};

#include "Post.h"
#endif

