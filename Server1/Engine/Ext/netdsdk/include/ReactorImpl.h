//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-02-01 15:26
 */
//========================================================================

#ifndef _REACTOR_IMPL_H_
#define _REACTOR_IMPL_H_
#include "Pre.h"

#include "TimeValue.h"
#include "EventHandler.h"

class TimerQueue;
class ReactorImpl;

/** 
 * @class NotificationBuffer
 *
 * @brief 
 */
class NotificationBuffer
{
public:
    NotificationBuffer ();
    NotificationBuffer (EventHandler *eh,
	    ReactorMask mask);

    ~NotificationBuffer ();

    // Pointer to the Event_Handler that will be dispatched
    // by the main event loop.
    EventHandler *eh_;

    // Mask that indicates which method to call.
    ReactorMask mask_;
};
/** 
 * @class ReactorNotify
 *
 * @brief Abstract class for unblocking an ReactorImpl from its event loop.
 */
class ReactorNotify : public EventHandler
{
public:
    // = Initialization and termination methods.
    virtual int open (ReactorImpl *) = 0;

    virtual int close (void) = 0;

    /**
     * Called by a thread when it wants to unblock the <ReactorImpl>.
     * This wakeups the <ReactorImpl> if currently blocked.  Pass over
     * both the <EventHandler> *and* the <mask> to allow the caller to
     * dictate which <EventHandler> method the <ReactorImpl> will invoke.
     * The <timeout> indicates how long to blocking trying to notify the 
     * <ReactorImpl>.  If timeout == 0, the caller will block until action 
     * is possible, else will wait until the relative time specified in 
     * timeout elapses).
     */
    virtual int notify (EventHandler *eh = 0,
	    ReactorMask mask = EventHandler::EXCEPT_MASK,
	    const TimeValue *timeout = 0) = 0;

    // Returns the NDK_HANDLE of the notify pipe on which the reactor
    // is listening for notifications so that other threads can unblock
    // the <ReactorImpl>.
    virtual NDK_HANDLE notify_handle (void) = 0;

    // Read one of the notify call on the <handle> into the
    // <buffer>. This could be because of a thread trying to unblock
    // the <ReactorImpl>
    virtual int read_notify_pipe (NDK_HANDLE handle,
	    NotificationBuffer &buffer) = 0;

    // Purge any notifications pending in this reactor for the specified
    // EventHandler object. Returns the number of notifications purged. 
    // Returns -1 on error.
    virtual int purge_pending_notifications (EventHandler * = 0,
	    ReactorMask = EventHandler::ALL_EVENTS_MASK) = 0;
};

/**
 * @class ReactorImpl
 *
 * @brief An abstract class for implementing the Reactor Pattern.
 */
class ReactorImpl
{
public:
    ReactorImpl ();
    virtual ~ReactorImpl ();

    //
    virtual int close (void) = 0;

    /**
     * Called by a thread when it wants to unblock the <ReactorImpl>.
     * This wakeups the <ReactorImpl> if currently blocked.  Pass over
     * both the <EventHandler> *and* the <mask> to allow the caller to
     * dictate which <EventHandler> method the <ReactorImpl> will invoke.
     * The <timeout> indicates how long to blocking trying to notify the 
     * <ReactorImpl>.  If timeout == 0, the caller will block until action 
     * is possible, else will wait until the relative time specified in 
     * timeout elapses).
     */
    virtual int notify (EventHandler *eh = 0,
	    ReactorMask mask = EventHandler::EXCEPT_MASK,
	    const TimeValue *timeout = 0) = 0;

    /**
     * This event loop driver blocks for up to <max_wait_time> before
     * returning.  It will return earlier if events occur.  Note that
     * <max_wait_time> can be 0, in which case this method blocks
     * indefinitely until events occur.
     *
     * Returns the total number of EventHandlers that were
     * dispatched, 0 if the <max_wait_time> elapsed without dispatching
     * any handlers, or -1 if an error occurs.
     */
    virtual int handle_events (const TimeValue *max_wait_time = 0) = 0;

    // ++ Event handling control.
    /**
     * Return the status of Reactor. If this function returns 0, the 
     * reactor is actively handling events. If it returns non-zero, 
     * <handling_events> return -1 immediately.
     */
    virtual int deactivated (void) = 0;

    /**
     * Control whether the Reactor will handle any more incoming events or not.
     * If <do_stop> == 1, the Reactor will be disabled.  By default, a reactor
     * is in active state and can be deactivated/reactived as wish.
     */
    virtual void deactivate (int do_stop) = 0;

    // ++ Register and remove Handlers.
    /**
     * Register <event_handler> with <mask>.  The I/O handle will always
     * come from <handle> on the <event_handler>.
     */
    virtual int register_handler (EventHandler *event_handler,
	    ReactorMask mask) = 0;

    /**
     * Register <event_handler> with <mask>.
     */
    virtual int register_handler (NDK_HANDLE io_handle,
	    EventHandler *event_handler,
	    ReactorMask mask) = 0;
    
    /**
     * Removes <event_handler>.  Note that the I/O handle will be
     * obtained using <handle> method of <event_handler> .  If
     * <mask> == <ACE_Event_Handler::DONT_CALL> then the <handle_close>
     * method of the <event_handler> is not invoked.
     */
    virtual int remove_handler (EventHandler *event_handler,
	    ReactorMask mask) = 0;

    /**
     * Removes <handle>.  If <mask> == <ACE_Event_Handler::DONT_CALL>
     * then the <handle_close> method of the associated <event_handler>
     * is not invoked.
     */
    virtual int remove_handler (NDK_HANDLE handle,
	    ReactorMask mask) = 0;

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
	    const TimeValue &interval = TimeValue::zero) = 0;

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
	    const TimeValue &interval) = 0;

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
	    int dont_call_handle_close = 1) = 0;

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
	    int dont_call_handle_close = 1) = 0;
    /**
     * Returns the size of the Reactor's internal descriptor
     * table.
     */
    virtual size_t size (void) const = 0;

    /**
     * Returns the actually number of the handles registered in Reactor.
     */
    virtual size_t curr_payload() = 0;

    // + Only the owner thread can perform a <handle_events>.

    // Set the new owner of the thread and return the old owner.
    virtual void owner (thread_t thr_id) = 0;

    // Return the current owner of the thread.
    virtual thread_t owner (void) = 0;
};

#include "Post.h"
#endif

