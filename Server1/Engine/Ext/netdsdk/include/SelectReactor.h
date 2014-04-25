//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-05-25 00:02
 */
//========================================================================

#ifndef _SELECTREACTOR_H_
#define _SELECTREACTOR_H_
#include "Pre.h"

#include "Pipe.h"
#include "ReactorImpl.h"
#include "ThreadMutex.h"
#include "GlobalMacros.h"
#include "ReactorToken.h"

#include <signal.h>        // for sig_atomic_t

class  SelectReactor;
/**
 * class SelectEventTuple 
 * @brief Class that associates specific event mask with a given event handler
 *
 * This class merely provides a means to associate an event mask with an event 
 * handler.  Such an association is needed since it is not possible to retrieve 
 * the event mask from the "interest set" stored in the fd_set.  
 * Without this external association, it would not be possible keep track of the
 * event mask for a given event handler when suspending it or resuming it.
 */
class SelectEventTuple
{
public:
    SelectEventTuple ();

    // The event handler
    EventHandler *event_handler;

    // The event mask for the above event handler.
    ReactorMask  mask;
};
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/**
 * class SelectReactorHandlerRepository
 * @brief
 * Used to map NDK_HANDLEs onto the appropriate EventHandler *.
 *
 * This class is simply a container that maps a handle to its
 * corresponding event handler.  It is not meant for use outside of.
 * the SelectReactor
 */
class SelectReactorHandlerRepository
{
public:
    SelectReactorHandlerRepository ();

    // Initialize a repository of the appropriate size.
    int open (size_t size);

    // Close down the repository.
    int close (void);
    
    /**
     * Return the EventHandler associated with NDK_HANDLE.  If
     * index_p is non-zero, then return the index location of the handle, if found.
     */
    EventHandler *find (NDK_HANDLE handle, size_t *index_p = 0);

    // Bind the EventHandler to handle with the appropriate ReactorMask setttings
    int bind (NDK_HANDLE handle, 
	    EventHandler *handler,
	    ReactorMask mask);

    // Remove the binding for NDK_HANDLE
    int unbind (NDK_HANDLE handle);
    int unbind_all (void);

    // Check the handle to make sure it's a valid NDK_HANDLE that is within the range 
    // of legal handles (i.e., greater than or equal to zero and less than max_size_).
    int is_invalid_handle (NDK_HANDLE handle) const;

    // Check the handle to make sure it's a valid NDK_HANDLE that is within the range of 
    // currently registered handles (i.e., greater than or equal to zero and less than 
    // max_handlep1_).
    int handle_in_range (NDK_HANDLE handle) const;

    // Returns the current table size.
    size_t size (void) const;

    // Set the event mask for event handler associated with the given handle
    void mask (NDK_HANDLE handle, ReactorMask mask); 

    // Retrieve the event mask for the event handler associated with the given handle.
    ReactorMask mask (NDK_HANDLE handle);
private:
    // Maximum number of handles.
    int max_size_ ;

    // The underlying array of event handlers.
    // The array of event handlers is directly indexed directly using an NDK_HANDLE valie
    // This is Unix-specific.
    SelectEventTuple *handlers_;
};
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/**
 * @class SelectReactorNotify
 *
 * @brief Event handler used for unblocking the SelectReactor from its event loop.
 *
 * This event handler is used internally by the SelectReactor as a means to allow 
 * a thread other then the one running the event loop to unblock the event loop.
 */
class SelectReactorNotify : public ReactorNotify
{
public:
    SelectReactorNotify ();
    // = Initialization and termination methods.
    virtual int open (ReactorImpl *);

    virtual int close ();

    /**
     * Called by a thread when it wants to unblock the <ReactorImpl>.
     * This wakeups the <ReactorImpl> if currently blocked.  Pass over
     * both the <EventHandler> *and* the <mask> to allow the caller to
     * dictate which <EventHandler> method the <ReactorImpl> will invoke.
     * The <timeout> indicates how long to blocking trying to notify the 
     * <ReactorImpl>.  If timeout == -1, the caller will block until action 
     * is possible, else will wait until the relative time specified in 
     * timeout elapses).
     */
    virtual int notify (EventHandler *eh = 0,
	    ReactorMask mask = EventHandler::EXCEPT_MASK,
	    const TimeValue *timeout = 0);

    // Returns the NDK_HANDLE of the notify pipe on which the reactor
    // is listening for notifications so that other threads can unblock
    // the <ReactorImpl>.
    virtual NDK_HANDLE notify_handle (void);

    // Read one of the notify call on the <handle> into the
    // <buffer>. This could be because of a thread trying to unblock
    // the <ReactorImpl>
    virtual int read_notify_pipe (NDK_HANDLE handle,
	    NotificationBuffer &buffer);

    // Purge any notifications pending in this reactor for the specified
    // EventHandler object. Returns the number of notifications purged. 
    // Returns -1 on error.
    virtual int purge_pending_notifications (EventHandler * = 0,
	    ReactorMask = EventHandler::ALL_EVENTS_MASK);

    // Handle readable event
    virtual int handle_input (NDK_HANDLE handle);
protected:
    // Keep a back pointer to the SelectReactor. 
    SelectReactor *ep_reactor_;

    /**
     * Contains the ACE_HANDLE the SelectReactor is listening on, as well as
     * the NDK_HANDLE that threads wanting the attention of the SelectReactor 
     * will write to.
     */
    Pipe notification_pipe_;
}; 
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/**
 * @class SelectReactor
 *
 * @brief A `select' based Reactor implemenatation.
 *
 * The SelectReactor uses the 'select' to demultiplex events on a given set of 
 * file descriptors.  But select() have hard-coded limit on the number of file 
 * descriptors(generally is 1024). 
 */

typedef ReactorToken SelectReactorToken;

class SelectReactor : public ReactorImpl
{
public:
    // Initialize SelectReactor
    SelectReactor (int auto_open = 1, size_t max_size = 0, TimerQueue * = 0);
    
    // Close down and release all resources.
    virtual ~SelectReactor ();

    // Initialization
    /**
     * Initialize SelectReactor with size.
     * Note On Unix platforms, the <size> parameter should be 
     * as large as the maximum number of file descriptors allowed 
     * in fd_set.  This is necessary since a file descriptor
     * is used to directly index the array of event handlers
     * maintained by the Reactor's handler repository.  Direct
     * indexing is used for efficiency reasons.  If the size
     * parameter is less than the process maximum, the process
     * maximum will be decreased in order to prevent potential
     * access violations.
     * max_size_ whill be set to FD_SETSIZE if <size> equel to zero
     */
    virtual int open (size_t size, TimerQueue *tq);

    // Close down and release all resources.
    virtual int close (void);

    // ++ Event loop drivers
    /**
     * Returns non-zero if there are I/O events "ready" for dispatching,
     * but does not actually dispatch the event handlers. By default,
     * don't block while checking this.
     */
    //virtual int work_pending (const int max_wait_time = -1);

    /**
     * This event loop driver blocks for up to @a max_wait_time before
     * returning.  It will return earlier if events occur.  Note that
     * @a max_wait_time can be -1, in which case this method blocks
     * indefinitely until events occur.
     *
     * return  The total number of EventHandlers that were dispatched,
     * 0 if the @a max_wait_time elapsed without dispatching any handlers, 
     * or -1 if an error occurs.
     */
    virtual int handle_events (const TimeValue *max_wait_time = 0);

    // ++ Event handling control.
    /**
     * return The status of Reactor.  If this function returns 0, the
     * reactor is actively handling events.  If it returns non-zero, 
     * handle_events() return -1 immediately.
     */
    virtual int deactivated (void);

    /**
     * Control whether the Reactor will handle any more incoming events
     * or not.  If do_stop == 1, the Reactor will be disabled.  By
     * default, a reactor is in active state and can be deactivated/reactived 
     * as desired.
     */
    virtual void deactivate (int do_stop);

    // ++ Register and remove Handlers. These method maybe called in user thread 
    // context, so must use mutex
    /**
     * Register event_handler with mask.  The I/O handle will always come 
     * from handle on the event_handler.
     */
    virtual int register_handler (EventHandler *event_handler,
	    ReactorMask mask);

    /**
     * Register event_handler with mask.  The I/O handle is provided through 
     * the io_handle parameter.
     */
    virtual int register_handler (NDK_HANDLE io_handle, 
	    EventHandler *event_handler,
	    ReactorMask mask);

    /**
     * The I/O handle will be obtained using handle() method of event_handler.
     * If mask == EventHandler::DONT_CALL then the handle_close() 
     * method of the event_handler is not invoked.
     */
    virtual int remove_handler (EventHandler *event_handler,
	    ReactorMask mask);

    /**
     * Removes <handle>.  If <mask> == <EventHandler::DONT_CALL then the 
     * handle_close() method of the associated <event_handler> is not invoked.
     */
    virtual int remove_handler (NDK_HANDLE io_handle,
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

    // 
    virtual int notify (EventHandler *eh = 0,
	    ReactorMask mask = EventHandler::EXCEPT_MASK,
	    const TimeValue *timeout = 0);

    // Returns the current size of the Reactor's internal descriptor table
    virtual size_t size (void) const;
protected:
    /**
     * Non-locking version of wait_pending().
     * Returns non-zero if there are I/O events "ready" for dispatching,
     * but does not actually dispatch the event handlers.  By default,
     * don't block while checking this. Return zero if timeout.
     */
    int work_pending_i (const TimeValue *max_wait_time);

    /**
     * Poll for events and return the number of event handlers that
     * were dispatched.This is a helper method called by all handle_events() 
     * methods.
     */
    int handle_events_i (const TimeValue *max_wait_time, TokenGuard &guard);

    /**
     * Perform the upcall with the given event handler method.
     */
    int upcall (EventHandler *event_handler,
	    int (EventHandler::*callback)(NDK_HANDLE),
	    NDK_HANDLE handle);

    /**
     * Dispatch EventHandlers for time events, I/O events, Returns the 
     * total number of EventHandlers that were dispatched or -1 if something 
     * goes wrong, 0 if no events ready.
     */
    int dispatch (TokenGuard &guard);

    /**
     * Dispatch a single timer, if ready.
     * Returns: 0 if no timers ready (token still held),
     *		1 if a timer was expired (token released)
     */
    int dispatch_timer_handler (TokenGuard &guard);
    /**
     * Dispatch an IO event to the corresponding event handler. Returns
     * Returns: 0 if no events ready ,
     *	       -1 on error
     */
    int dispatch_io_event (TokenGuard &guard);

    /**
     * Register the given event handler with the reactor.
     */
    int register_handler_i (NDK_HANDLE handle,
	    EventHandler *event_handler,
	    ReactorMask mask);

    // Remove the event handler which relate the handle
    int remove_handler_i (NDK_HANDLE handle,
	    ReactorMask mask);

    // Convert a reactor mask to its corresponding epoll() event mask.
    size_t reactor_mask_to_epoll_event (ReactorMask mask);

    // mask operate 
    int mask_opt_i (NDK_HANDLE handle, 
	    ReactorMask mask, 
	    int opt);

protected:
    //
    bool initialized_;

    // The maximum number of file descriptors over which demultiplexing will occur.
    size_t size_;

    // This flag is used to keep track of whether we are actively handling
    // events or not.
    //AtomicOpt_T<int, ThreadMutex> deactivated_;
    sig_atomic_t deactivated_;

    // Keeps track of whether we should delete the timer queue (if we
    // didn't create it, then we don't delete it).
    int delete_timer_queue_;

    // Timer queue
    TimerQueue *timer_queue_;

    // Callback object that unblocks the <SelectReactor> if it's sleeping.
    ReactorNotify *notify_handler_;

    // Current thread calls <handle_events>
    thread_t owner_;

    // The repository that contains all registered event handlers.
    SelectReactorHandlerRepository handler_rep_;

    // The mutex of the <handler_rep_>
    typedef ThreadMutex HR_MUTEX;
    HR_MUTEX handler_rep_lock_;

    // L/F Pattern mutex
    SelectReactorToken   token_;
};

#include "SelectReactor.inl"
#include "Post.h"
#endif

