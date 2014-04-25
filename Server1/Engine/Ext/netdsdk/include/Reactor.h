//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-27 14:47
 */
//========================================================================

#ifndef _REACTOR_H_
#define _REACTOR_H_
#include "Pre.h"

#include "Trace.h"
#include "TimeValue.h"
#include "ThreadMutex.h"
#include "EventHandler.h"

#include <memory>          // for auto_ptr

class ReactorImpl;
/**
 * @class Reactor
 *
 * @brief The responsibility of this class is to forward all 
 * methods to its delegation/implementation class, e.g.,
 * SelectReactor EpollReactor
 */
class Reactor
{
public:
    enum
    {
	ADD_MASK  = 1,
	CLR_MASK  = 2,
	SET_MASK  = 3
    };
    /**
     * Create the Reactor using @a implementation.  The flag
     * @a delete_implementation tells the Reactor whether or not to
     * delete the @a implementation on destruction.
     */
    Reactor (ReactorImpl* implementation = 0, 
	    int delete_implementation = 0);
    /**
     * Close down and release all resources.
     * Any notifications that remain queued on this reactor instance
     * are lost 
     */
    virtual ~Reactor ();

    // Get pointer to a process-wide Reactor.
    static Reactor *instance (void);

    /**
     * Set pointer to a process-wide Reactor and return existing
     * pointer.  If <delete_reactor> != 0 then we'll delete the 
     * Reactor at destruction time.
     */
    static Reactor *instance (Reactor *r);

    // Delete the dynamically allocated Singleton
    static void close_singleton (int delete_reactor = 0);

    // ++ Reactor event loop management methods.
    // These methods work with an instance of a reactor.
    /**
     * Run the event loop until the 
     * <Reactor::handle_events/Reactor::alertable_handle_events>
     * method returns -1 or the <end_reactor_event_loop> method is invoked.
     */
    virtual int run_reactor_event_loop ();

    /**
     * Instruct the Reactor to terminate its event loop and notifies the
     * Reactor so that it can wake up and deactivate
     * itself. Deactivating the Reactor would allow the Reactor to be
     * shutdown gracefully. Internally the Reactor calls deactivate ()
     * on the underlying implementation.
     */ 
    virtual void end_reactor_event_loop (void);

    // Indicate if the Reactor's event loop has been ended.
    virtual int reactor_event_loop_done (void);

    /** 
     * Restart <run_reactor_event_loop> 
     * Internally the Reactor calls deactivate(0) on the underlying 
     * implementation.
     */
    virtual void reset_reactor_event_loop (void);
    /**
     * This event loop driver blocks for up to <max_wait_time> before
     * returning.  It will return earlier if events occur.  Note that
     * <max_wait_time> can be 0, in which case this method blocks
     * indefinitely until events occur.
     */
    virtual int handle_events (const TimeValue *max_wait_time = 0);

    // ++ Register and remove handlers.
    /**
     * Register handler for I/O events.
     * The handle will come from EventHandler::handle().
     *
     * If this handler/handle pair has already been registered, any 
     * new masks specified will be added.
     *
     * The reactor of the <event_handler> will be assigned to *this*
     * automatic, restore the original reactor if register failed
     */
    virtual int register_handler (EventHandler *event_handler,
	    ReactorMask mask);

    /**
     * Register handler for I/O events.
     * Same as register_handler(EventHandler*, ReactorMask),
     * except handle is explicitly specified.
     */
    virtual int register_handler (NDK_HANDLE io_handle,
	    EventHandler *event_handler,
	    ReactorMask mask);

    /**
     * Remove <masks> from <handle> registration.
     * For I/O handles, <masks> are removed from the Reactor.  Unless
     * <masks> includes <EventHandler::DONT_CALL>, 
     * EventHandler::handle_close() will be called with the <masks>
     * that have been removed. 
     */
    virtual int remove_handler (NDK_HANDLE handle, 
	    ReactorMask masks);

    /**
     * Remove <masks> from <event_handler> registration.
     * Same as remove_handler(NDK_HANDLE, ReactorMask), except
     * <handle> comes from EventHandler::handle().
     */
    virtual int remove_handler (EventHandler *event_handler,
	    ReactorMask masks);

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

    // Get the implementation class
    ReactorImpl* implementation ();

    // Returns the current size of the Reactor's internal descriptor
    // table
    virtual size_t size (void);

protected:
    // Set the implementation class.
    virtual void implementation (ReactorImpl *implementation);

    // Delegation/implementation class that all methods will be
    // forwarded to.
    ReactorImpl *implementation_;

    // Flag used to indicate whether we are responsible for cleaning up
    // the implementation instance
    int delete_implementation_;

    // Pointer to a process-wide ACE_Reactor singleton.
    static Reactor *reactor_;
    static ThreadMutex instance_lock_;

private:
    // Deny access since member-wise won't work...
    Reactor (const Reactor &);
    Reactor &operator = (const Reactor &);
};

#include "Reactor.inl"
#include "Post.h"
#endif

