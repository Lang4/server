//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-04-19 23:13
 */
//========================================================================

#ifndef _MULTI_REACTORS_H_
#define _MULTI_REACTORS_H_
#include "Pre.h"

#include "ThreadMutex.h"
#include "ReactorImpl.h"
#include "GlobalMacros.h"

/**
 * @class MultiReactors
 *
 * @brief The responsibility of this class is disgind to dispatch large 
 * numbers of fds use multiple Reactor, as one reactor that includes large 
 * numbers of fds and which alse process I/O operation with only one thread 
 * would impact concurrence. e.g. EpollReactor
 */
enum 
{
    // The capacity of single reactor will increase along with 
    // the increasing concurrence service.
    REACTOR_NUM_FIXED = 0x13,

    // The number of the reactors will increase along with the increasing
    // concurrence service.
    SINGLE_REACTOR_CAPACITY_FIXED = 0x14
};
template<typename REACTOR>
class MultiReactors : public ReactorImpl
{
public:
    // Constructor
    MultiReactors ();

    ~MultiReactors ();

    /**
     * If <strategy> equals REACTOR_NUM_FIXED, then the argment <count>
     * is the number of reactors, else if <strategy> equals 
     * SINGLE_REACTOR_CAPACITY_FIXED , the argment <count> is the capacity of
     * single reactor serves. The <max_reactor_count> is the number of 
     * the handles that all Reactors can poll, if 0 , then the value is the number
     * of a process can allocate (e.g. result of `ulimit -n').
     */
    int init (int strategy = REACTOR_NUM_FIXED, 
	    int count = 4,
	    int max_handlers = 0);

    //
    virtual int close (void);

    // Not implement
    virtual int notify (EventHandler *eh = 0,
	    ReactorMask mask = EventHandler::EXCEPT_MASK,
	    const TimeValue *timeout = 0);

    // Not implement
    virtual int handle_events (const TimeValue *max_wait_time = 0);

    // Not implement
    virtual int deactivated (void);

    // Not implement
    virtual void deactivate (int do_stop);

    // ++ Register and remove Handlers.
    /**
     * Register <event_handler> to certain reactor with <mask>. 
     * The I/O handle will always come from <handle> on the <event_handler>.
     */
    virtual int register_handler (EventHandler *event_handler,
	    ReactorMask mask);

    /**
     * <handle> will be register to certain reactor.
     * Others is same to <Reactor>
     */
    virtual int register_handler (NDK_HANDLE handle, 
	    EventHandler *event_handler,
	    ReactorMask mask);

    /**
     * Removes <event_handler>.  Note that the I/O handle will be
     * obtained using <handle> method of <event_handler> .  If
     * <mask> == <ACE_Event_Handler::DONT_CALL> then the <handle_close>
     * method of the <event_handler> is not invoked.
     */
    virtual int remove_handler (EventHandler *event_handler,
	    ReactorMask mask);

    /**
     * Removes <handle>.  If <mask> == <ACE_Event_Handler::DONT_CALL>
     * then the <handle_close> method of the associated <event_handler>
     * is not invoked.
     */
    virtual int remove_handler (NDK_HANDLE handle,
	    ReactorMask mask);

    // ++ Timer management.

    /**
     * Schedule a timer event.
     * Schedule a timer event that will expire after an <delay> amount
     * of time.  The return value of this method, a timer_id value,
     * uniquely identifies the <event_handler> in the Reactor's internal
     * list of timers.  This timer_id value can be used to cancel the 
     * timer with the cancel_timer() call. 
     */
    virtual int schedule_timer (EventHandler *event_handler,
	    const void *arg,
	    const TimeValue &delay,
	    const TimeValue &interval = TimeValue::zero);

    /**
     * Reset recurring timer interval.
     *
     * Resets the interval of the timer represented by  timer_id to
     * interval, which is specified in relative time to the current
     * <gettimeofday>.  If <interval> is equal to TimeValue::zero,
     * the timer will become a non-rescheduling timer.  Returns 0 
     * if successful, -1 if not.
     * This change will not take effect until the next timeout.
     */
    virtual int reset_timer_interval (int timer_id,
	    const TimeValue &interval);

    /**
     * Cancel timer.
     *
     * Cancel timer associated with timer_id that was returned from
     * the schedule_timer() method.  If arg is non-NULL then it will be
     * set to point to the argument passed in when the handler was 
     * registered. This makes it possible to free up the memory and 
     * avoid memory leaks.  Returns 0 if cancellation succeeded and -1 
     * if the  timer_id wasn't found.
     *
     * On successful cancellation, EventHandler::handle_close () will 
     * be called when <dont_call_handle_close> is zero, handle_close() 
     * will pass the argument passed in when the handler was 
     * registered, This makes it possible to release the memory of 
     * the <arg> in handle_close().
     * avoid memory leaks, also pass the argument <EventHandler::TIMER_MASK>. 
     */
    virtual int cancel_timer (int timer_id,
	    const void **arg = 0,
	    int dont_call_handle_close = 1);

    /**
     * Cancel all timers associated with event handler.
     *
     * Shorthand for calling cancel_timer(long,const void **,int) 
     * multiple times for all timer associated with <event_handler>.
     *
     * EventHandler::handle_close () will be called when 
     * <dont_call_handle_close> is zero, but not pass the argument 
     * passed in when the handler was registered, because it could 
     * register multiple argument<arg>, I don't know which should be
     * passed.
     *
     * Returns number of handlers cancelled.
     */
    virtual int cancel_timer (EventHandler *event_handler,
	    int dont_call_handle_close = 1);

    /**
     * <count> is the number of reactors in MultiReactor, 
     * on -1 will stop all reactors.
     */
    void stop_reactors (int count = -1);

    /**
     * Returns the current size of the Reactor's internal descriptor
     * table.
     */
    virtual size_t size (void) const;

    // Not implement this method.
    virtual size_t curr_payload ();

    // + Only the owner thread can perform a <handle_events>.

    // Set the new owner of the thread and return the old owner.
    virtual void owner (thread_t thr_id);

    // Return the current owner of the thread.
    virtual thread_t owner (void);
protected:
    // Append a new REACTOR , return 0 if create success, other else
    // return -1.
    int append_reactor (void);

    // Inner method.
    virtual int register_handler_i (NDK_HANDLE handle, 
	    EventHandler *event_handler,
	    ReactorMask mask);

    // Inner method.
    virtual int remove_handler_i (NDK_HANDLE handle, 
	    ReactorMask mask);
protected:
    // 
    int	strategy_;

    //  
    int	reactor_count_;

    //
    int max_reactor_count_;

    //
    int max_handles_;

    // 
    int reactor_index_;

    //
    int single_reactor_capacity_;

    //
    typedef ThreadMutex MUTEX;
    MUTEX mutex_;

    REACTOR **reactors_;
};

#include "MultiReactors.cpp"
#include "Post.h"
#endif

