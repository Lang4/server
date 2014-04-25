//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-05-08 22:16
 */
//========================================================================

#ifndef _THREADTIMERSVC_H_
#define _THREADTIMERSVC_H_
#include "Pre.h"

#include "TaskBase.h"
#include "Condition.h"
#include "ThreadMutex.h"
#include "RecursiveThreadMutex.h"

/**
 * @class ThreadTimerSvc
 *
 * @brief Implement an timer queue using a separate thread for dispatching.
 */
class TimerQueue;
class ThreadTimerSvc : public TaskBase
{
public:
    // Creates the timer queue.  Activation of the task is the user's
    // responsibility.
    ThreadTimerSvc (TimerQueue *tq = 0);

    // Destructor
    virtual ~ThreadTimerSvc ();

    // Return the timer_id if schedule success, return -1 on error.
    // Note : <future_time> is relative time.
    int schedule_timer (EventHandler *eh,
	    const void *arg,
	    const TimeValue &future_time,
	    const TimeValue &interval_time = TimeValue::zero);

    // Cancel the timer_id and pass back the  arg if an address is
    // passed in.
    int cancel_timer (int timer_id, const void **arg = 0);

    // Reset the interval of the <timer_id>
    int reset_timer_interval (int timer_id, const TimeValue &interval);

    // Inform the dispatching thread that it should terminate.
    virtual void deactivate (void);
protected:
    // Runs the dispatching thread.
    virtual int svc (void);
private:
    // Keeps track of whether we should delete the timer queue (if we
    // didn't create it, then we don't delete it).
    int delete_timer_queue_;

    // When deactivate is called this variable turns to false and the
    // dispatching thread is signalled, to terminate its main loop.
    volatile int active_;

    // Timer queue
    TimerQueue *timer_queue_;

    // Timer queue mutes
    typedef RecursiveThreadMutex MUTEX;
    //typedef ThreadMutex MUTEX;
    MUTEX mutex_; 

    /** 
     * The dispatching thread sleeps on this condition while waiting to
     * dispatch the next timer; it is used to wake it up if there is a
     * change on the timer queue.
     */
    Condition<MUTEX> condition_;
};

#include "Post.h"
#endif

