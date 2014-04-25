#include "EpollTPReactor.h"
#include "TimerQueue.h"
#include "Guard_T.h"
#include "Reactor.h"
#include "Common.h"
#include "Debug.h"
#include "Trace.h"
#include "NDK.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>

int EpollTPReactor::handle_events (const TimeValue *max_wait_time/* = 0*/) 
{
    TRACE ("EpollTPReactor");
    TokenGuard guard (this->token_);
    int result = guard.acquire_quietly (max_wait_time);
    if (!guard.is_owner ())
	return result;

    if (this->deactivated_)
	return -1;

    return this->handle_events_i (max_wait_time, guard);
}
int EpollTPReactor::handle_events_i (const TimeValue *max_wait_time, 
	TokenGuard &guard) 
{
    TRACE ("EpollTPReactor");
    int result  = 0;
    int err_num = 0;
#ifdef MODIFY_EPOLL_TIMEOUT
    TimeValue max_wait_time_buff;
    if (max_wait_time)
	max_wait_time_buff = *max_wait_time;
#endif
    
    /**
     * Wait pending events use epoll_wait, but not check errno
     */
    do
    {
#ifdef MODIFY_EPOLL_TIMEOUT 
	TimeValue tp_before, tp_after;
	if (max_wait_time != 0 
		&& *max_wait_time > TimeValue::zero)
	    tp_before->gettimeofday ();
	result = this->work_pending_i (*max_wait_time_buff);
#else
	result = this->work_pending_i (max_wait_time);
#endif

#ifdef MODIFY_EPOLL_TIMEOUT
	err_num = errno;
	// This code block used to save epoll_wait escape time
	if (result == -1 && err_num == EINTR)
	{
	    if (max_wait_time != 0
		    && *max_wait_time > TimeValue::zero)
	    {
		tp_after.gettimeofday ();
		max_wait_time_buff -= tp_after - tp_before;
		if (max_wait_time_buff < TimeValue::zero)
		{
		    result = 0;  // timeout
		    break;
		}
	    }
	}
#endif
    }while (result == -1 && err_num == EINTR);

    // timeout
    if (result == 0)
	return 0;
    else if (result == -1 && err_num != EINTR)
	return result;
    // Dispatch an event.
    return this->dispatch (guard);
}
int EpollTPReactor::dispatch (TokenGuard &guard)
{
    TRACE ("EpollTPReactor");
    int result = 0;
    // Handle timers early since they may have higher latency
    // constraints than I/O handlers.  Ideally, the order of
    // dispatching should be a strategy...

    if ((result = this->dispatch_timer_handler (guard)) != 0)
	return result;

    // Check to see if there are no more I/O handles left to
    // dispatch AFTER we've handled the timers.
    //
    result = this->dispatch_io_event (guard);
    return result;
}
int EpollTPReactor::dispatch_timer_handler (TokenGuard &guard)
{
    if (this->timer_queue_->is_empty ())
	return 0;

    TimerNode dispatched_node;
    TimeValue current_time = NDK::gettimeofday ();
    if (this->timer_queue_->dispatch_timer (current_time,
		dispatched_node))
    {
	// Release the token before expiration upcall.
	guard.release_token ();

	if (this->timer_queue_->upcall (dispatched_node.handler (),
		&EventHandler::handle_timeout,
		dispatched_node.arg (),
		current_time) < 0)
	{
	    // This call will be locked by EpollToken

	    // We must ensure the timer that expired one time, notify to user
	    // when it call done
	    this->cancel_timer (dispatched_node.timer_id (),
		    0,  // passed out arg
		    0   // call handle close
		    );
	}
	return 1;
    }
    return 0;
}
int EpollTPReactor::dispatch_io_event (TokenGuard &guard)
{
    TRACE ("EpollTPReactor");
    struct epoll_event *pfd = this->start_pevents_;
    // Increment the pointer to the next element 
    ++this->start_pevents_;
    if (pfd < this->end_pevents_)
    {
	const NDK_HANDLE handle   = pfd->data.fd;

	if (pfd->events == 0)
	{
	    return 0;
	}
	bool disp_out  = false;
	bool disp_exc  = false;
	bool disp_in   = false;
	if (NDK_BIT_ENABLED (pfd->events, EPOLLOUT))
	{
	    disp_out = true;
	    NDK_CLR_BITS (pfd->events, EPOLLOUT);
	}else if (NDK_BIT_ENABLED (pfd->events, EPOLLPRI))
	{
	    disp_exc = true;
	    NDK_CLR_BITS (pfd->events, EPOLLPRI);
	}else if (NDK_BIT_ENABLED (pfd->events, EPOLLIN))
	{
	    disp_in = true;
	    NDK_CLR_BITS (pfd->events, EPOLLIN);
	}else if (NDK_BIT_ENABLED (pfd->events, EPOLLHUP | EPOLLERR))
	{
	    this->remove_handler_i (handle, EventHandler::ALL_EVENTS_MASK);
	    return 0;
	}
	else
	{
	    NDK_INF ("dispatch_io [handle = %d] trigger unknown events 0x%x\n",
		    handle, pfd->events);
	}
	// 
	EventHandler *eh = this->handler_rep_.find (handle);
	if (eh)
	{
	    // Note that if there's an error (such as the handle was closed
	    // without being removed from the event set) the EPOLLHUP and/or
	    // EPOLLERR bits will be set in pfd->events.
	    //int status  = 0;
	    guard.release_token ();
	    if (disp_out)
	    {
		if (this->upcall (eh, &EventHandler::handle_output, handle) < 0)
		    this->remove_handler (handle, EventHandler::WRITE_MASK);
		return 1;
	    }
	    else if (disp_exc)
	    {
		if (this->upcall (eh, &EventHandler::handle_exception, handle) < 0)
		    this->remove_handler (handle, EventHandler::EXCEPT_MASK);
		return 1;
	    }
	    else if (disp_in)
	    {
		if (this->upcall (eh, &EventHandler::handle_input, handle) < 0)
		    this->remove_handler (handle, EventHandler::READ_MASK);
		return 1;
	    }
	}else
	{
	    NDK_INF ("dispatch_io [handle = %d] not match event handler\n", handle);
	}
    }
    return 0;
}
size_t EpollTPReactor::reactor_mask_to_epoll_event (ReactorMask mask)
{
    TRACE ("EpollTPReactor");
    if (mask == EventHandler::NULL_MASK)
	return EPOLL_CTL_DEL;
    size_t events = 0;
    // READ, ACCEPT, and CONNECT flag will place the handle in the
    // read set.
    if (NDK_BIT_ENABLED (mask, EventHandler::READ_MASK))
	NDK_SET_BITS (events, EPOLLIN | EPOLLET);

    if (NDK_BIT_ENABLED (mask, EventHandler::ACCEPT_MASK)
	    || NDK_BIT_ENABLED (mask, EventHandler::CONNECT_MASK))
	NDK_SET_BITS (events, EPOLLIN);

    // WRITE and CONNECT flag will place the handle in the write set.
    if (NDK_BIT_ENABLED (mask, EventHandler::WRITE_MASK))
	NDK_SET_BITS (events, EPOLLOUT | EPOLLET);
    if (NDK_BIT_ENABLED (mask, EventHandler::CONNECT_MASK))
	NDK_SET_BITS (events, EPOLLOUT);
    // EXCEPT flag will place the handle in the except set.
    if (NDK_BIT_ENABLED (mask, EventHandler::EXCEPT_MASK))
	NDK_SET_BITS (events, EPOLLPRI);
    return events;
}
int EpollTPReactor::register_handler_i (NDK_HANDLE handle,
	EventHandler *event_handler,
	ReactorMask mask)
{
    NDK::set_non_block_mode (handle);
    return EpollTPReactor::register_handler_i (handle, 
	    event_handler, 
	    mask);
}


