//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-04-19 23:13
 */
//========================================================================

#ifndef _EPOLL_TP_REACTOR_H_
#define _EPOLL_TP_REACTOR_H_
#include "Pre.h"

#include "EpollReactor.h"

class EpollTPReactor : public EpollReactor
{
public:
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
protected:
    /**
     * Poll for events and return the number of event handlers that
     * were dispatched.This is a helper method called by all handle_events() 
     * methods.
     */
    int handle_events_i (const TimeValue *max_wait_time, TokenGuard &guard);

    /**
     * Dispatch EventHandlers for time events, I/O events, Returns the 
     * total number of EventHandlers that were dispatched or -1 if something 
     * goes wrong, 0 if no events ready.
     */
    int dispatch (TokenGuard &guard);

    /**
     * Dispatch one timer, if ready.
     * Returns: 0 if no timers ready (token still held),
     *		1 if a timer was expired (token released)
     */
    int dispatch_timer_handler (TokenGuard &guard);
    /**
     * Dispatch an IO event to the corresponding event handler.
     * Returns: 0 if no events ready ,
     *	       -1 on error
     */
    int dispatch_io_event (TokenGuard &guard);
    /**
     * Register the given event handler with the reactor.
     */
    virtual int register_handler_i (NDK_HANDLE handle,
	    EventHandler *event_handler,
	    ReactorMask mask);

    // Convert a reactor mask to its corresponding epoll() event mask.
    virtual size_t reactor_mask_to_epoll_event (ReactorMask mask);
};

#include "Post.h"
#endif

