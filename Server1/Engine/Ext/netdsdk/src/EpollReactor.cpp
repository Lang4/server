#include "EpollReactor.h"
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

int EpollReactorHandlerRepository::open (int size)
{
    if (size <= 0) return -1;
    this->handlers_   = new EpollEventTuple[size];
    if (this->handlers_ == 0)
	return -1;
    this->max_size_ = size;
    // Try to set the number of handles to the current limit. 
    // Deny user to spawn new handle that overtop of the size, it'll overflow
    return 0;//NDK::set_handle_limit (this->max_size_);
}
int EpollReactorHandlerRepository::close ()
{
    if (this->handlers_ != 0)
    {
	this->unbind_all ();
	delete [] this->handlers_;
	this->handlers_  = 0; 
	this->max_size_ = 0;
    }
    return 0;    
}
int EpollReactorHandlerRepository::unbind (NDK_HANDLE handle)
{
    TRACE ("EpollReactorHandlerRepository");
    if (this->find (handle) == 0)
	return -1;
    this->handlers_[handle].event_handler = 0;
    this->handlers_[handle].mask = EventHandler::NULL_MASK;
    return 0;
}
int EpollReactorHandlerRepository::unbind_all ()
{
    for (int handle = 0; handle < this->max_size_; ++handle)
	this->unbind (handle);
    return 0;
}
EventHandler *EpollReactorHandlerRepository::find (NDK_HANDLE handle,
	size_t *index_p/* = 0*/)
{
    TRACE ("EpollReactorHandlerRepository");
    EventHandler *eh = 0;
    if (this->handle_in_range (handle))
    {
	eh = this->handlers_[handle].event_handler;
	if (eh != 0)
	{
	    if (index_p != 0)
		*index_p = handle;
	}
    }
    return eh;
}
int EpollReactorHandlerRepository::bind (NDK_HANDLE handle,
	EventHandler *event_handler,
	ReactorMask mask)
{
    TRACE ("EpollReactorHandlerRepository");
    if (event_handler == 0 || this->handlers_ == 0)
	return -1;
#if 0
    if (handle == NDK_INVALID_HANDLE)
	handle = event_handler->handle ();
#endif
    if (this->is_invalid_handle (handle))
	return -1;
    this->handlers_[handle].event_handler = event_handler;
    this->handlers_[handle].mask = mask;
    return 0;
}
//---------------------------------------------------------------------------
EpollReactorNotify::EpollReactorNotify ()
: ep_reactor_ (0)
, notification_pipe_ ()
{
    TRACE ("EpollReactorNotify");
}
int EpollReactorNotify::open (ReactorImpl *impl)
{
    TRACE ("EpollReactorNotify");
    this->ep_reactor_ = dynamic_cast<EpollReactor *> (impl);
    if (this->ep_reactor_ == 0)
	return -1;
    if (this->notification_pipe_.open () == -1)
	return -1;
    // 
    // close-on-exec
    ::fcntl (this->notification_pipe_.read_handle (), F_SETFD, FD_CLOEXEC);
    ::fcntl (this->notification_pipe_.write_handle (), F_SETFD, FD_CLOEXEC);

    // Set the read handle into non-blocking mode since we need to
    // perform a "speculative" read when determining if there are
    // notifications to dispatch.
    NDK::set_non_block_mode (this->notification_pipe_.read_handle ());
    return 0;
}
int EpollReactorNotify::close ()
{
    TRACE ("EpollReactorNotify");
    return this->notification_pipe_.close ();
}
int EpollReactorNotify::notify (EventHandler *eh/* = 0*/,
	ReactorMask mask/* = EventHandler::EXCEPT_MASK*/,
	const TimeValue *timeout/* = 0*/)
{
    TRACE ("EpollReactorNotify");
    unused_arg (timeout);
    if (this->ep_reactor_ == 0)
	return 0;

    // 
    NotificationBuffer buff (eh, mask);
    return NDK::write (this->notification_pipe_.write_handle (), 
	    (void *)&buff, 
	    sizeof (buff),
	    &TimeValue::zero) > 0 ? 0 : -1;
}
int EpollReactorNotify::read_notify_pipe (NDK_HANDLE handle,
	NotificationBuffer &buffer)
{
    TRACE ("EpollReactorNotify");
    /*
    int ret = NDK::read (this->notification_pipe_.read_handle (),
	    (char *)&buffer,
	    sizeof (buffer),
	    0);
    if (ret > 0)
    {
	if (ret < sizeof (buffer))
	{
	    int n = sizeof (buffer) - ret;
	    if (NDK::read (this->notification_pipe_.read_handle (),
			((char *)&buffer)[ret],
			n) <= 0)
		return -1;

	}
	return 0;
    }
    if (ret <= 0 && (errno != EWOULDBLOCK && errno != EAGAIN))
	return -1;
    return 0;
    */
    unused_arg (handle);
    return NDK::read (this->notification_pipe_.read_handle (),
	    (char *)&buffer,
	    sizeof (buffer),
	    &TimeValue::zero);
}
int EpollReactorNotify::handle_input (NDK_HANDLE handle)
{
    TRACE ("EpollReactorNotify");
    NotificationBuffer buffer;
    this->read_notify_pipe (handle, buffer);
    return 0;
}
int EpollReactorNotify::purge_pending_notifications (
	EventHandler *event_handler/* = 0*/,
	ReactorMask/* = EventHandler::ALL_EVENTS_MASK*/)
{
    TRACE ("EpollReactorNotify");
    unused_arg (event_handler);
    return -1;
}
NDK_HANDLE EpollReactorNotify::notify_handle ()
{
    TRACE ("EpollReactorNotify");
    return this->notification_pipe_.read_handle ();
}
//---------------------------------------------------------------------------
void polite_sleep_hook (void *) { }
int EpollReactor::TokenGuard::acquire_quietly (const TimeValue *timeout/* = 0*/)
{
    TRACE ("EpollReactor::TokenGuard");
    int result = 0;
    if (timeout == 0)
    {
	result = this->token_.acquire_read (&polite_sleep_hook);
    }else
    {
	result = this->token_.acquire_read (&polite_sleep_hook,
		0,
		timeout);
    }
    if (result == 0)
	this->owner_ = 1;
    return result;
}
int EpollReactor::TokenGuard::acquire (const TimeValue *timeout/* = 0*/)
{
    TRACE ("EpollReactor::TokenGuard");
    int result = 0;
    if (timeout == 0)
    {
	result = this->token_.acquire ();
    }else
    {
	result = this->token_.acquire (0, 0, timeout);
    }
    if (result == 0)
	this->owner_ = 1;
    return result;
}
//---------------------------------------------------------------------------
EpollReactor::EpollReactor (int auto_open/* = 1*/,
	int epoll_size/* = 0*/,
	TimerQueue *tq/* = 0*/)
: initialized_ (false) 
, epoll_fd_ (NDK_INVALID_HANDLE)
, size_ (0)
, curr_payload_ (0)	   
, deactivated_ (0)
, delete_timer_queue_ (0)
, events_ (0)
, start_pevents_ (0)
, end_pevents_ (0)
, timer_queue_ (tq)
, notify_handler_ (0)
, owner_ (NULL_thread)
, token_ (this)
{
    TRACE ("EpollReactor");
    if (auto_open)
    {
	if (epoll_size == 0)
	   epoll_size = NDK::max_handles ();
	this->open (epoll_size, tq);
    }
}
EpollReactor::~EpollReactor ()
{
    TRACE ("EpollReactor");
    this->close ();
}
int EpollReactor::open (int size, TimerQueue *tq, 
	bool reset_fd_limit/* = true*/)
{
    TRACE ("EpollReactor");
    Guard_T<EpollReactorToken> g (this->token_);
    if (this->initialized_)
	return -1;

    this->timer_queue_ = tq;
    this->size_        = size;
    if (this->size_ == 0)
	this->size_ = NDK::max_handles ();
    else
    {
	if (reset_fd_limit)
	{
	    NDK::set_handle_limit (this->size_);
	}
    }
    if (this->size_ <= 0)
	return -1;

    int result = 0;
    this->notify_handler_ = new EpollReactorNotify();
    if (this->notify_handler_ == 0)
	result = -1;
    this->events_ = new epoll_event[this->size_];
    if (this->events_ == 0)
	result = -1;

    NDK_INF ("reactor max handles = %d\n", this->size_);
    if (result != -1)
	this->epoll_fd_ = ::epoll_create (this->size_);
    if (this->epoll_fd_ == NDK_INVALID_HANDLE)
    {
	NDK_DBG ("epoll create");
	result = -1;
    }
    ::fcntl (this->epoll_fd_, F_SETFD, FD_CLOEXEC);
    if (result != -1 
	    && this->handler_rep_.open (NDK::max_handles ()))
	result = -1;
    if (result != -1 && this->timer_queue_ == 0)
    {
	this->timer_queue_ = new TimerQueue();
	if (this->timer_queue_ == 0)
	    result = -1;
	else
	    this->delete_timer_queue_ = 1;
    }

    if (result != -1)
    {
	if (this->notify_handler_->open (this) == -1
		|| 
		this->register_handler_i (this->notify_handler_->notify_handle (),
		this->notify_handler_,
		EventHandler::READ_MASK) == -1)
	    result = -1;
    }

    if (result != -1)
	this->initialized_ = true;
    else
	this->close ();

    return result;
}
int EpollReactor::close (void) 
{
    TRACE ("EpollReactor");
    Guard_T<EpollReactorToken> g (this->token_);
    if (this->epoll_fd_ != NDK_INVALID_HANDLE)
	::close (this->epoll_fd_);
    this->epoll_fd_ = NDK_INVALID_HANDLE;
    if (this->events_)
	delete [] this->events_;
    this->events_ = 0;
    if (this->timer_queue_)
	delete this->timer_queue_;
    this->timer_queue_ = 0;
    this->events_ = 0;
    this->handler_rep_.close ();
    this->epoll_fd_      = NDK_INVALID_HANDLE;
    this->curr_payload_ = 0;
    this->start_pevents_ = 0;
    this->end_pevents_   = 0;
    this->initialized_   = false;
    this->deactivated_   = 1;
    return 0;
}
#if 0
int EpollReactor::work_pending (int max_wait_time/*msec*//* = -1*/)
{
    TRACE ("EpollReactor");
    //
    return this->work_pending_i (max_wait_time);
}
#endif
int EpollReactor::work_pending_i (const TimeValue *max_wait_time)
{
    TRACE ("EpollReactor");
    if (this->deactivated_)
	return 0;
    if (this->start_pevents_ != this->end_pevents_)
	return 1;  // We still have work_pending (). Do not poll for
		   // additional events.
    
    TimeValue wait_time(0);
    TimeValue *this_timeout = 0;
    this_timeout = this->timer_queue_->calculate_timeout (
	    const_cast<TimeValue *>(max_wait_time), 
	    &wait_time);

    // Check if we have timers to fire.
    const int timers_pending =
	((this_timeout != 0 && max_wait_time == 0)  // one timer
	 || (this_timeout != 0 && max_wait_time != 0
	     && *this_timeout != *max_wait_time) ? 1 : 0); 

    const long timeout =
	(this_timeout == 0 ? -1 /* Infinity */
	 : static_cast<long>(this_timeout->msec ())/* Point to wait_time */);	

    // Wait for events
    int nfds = 0;
    nfds = ::epoll_wait (this->epoll_fd_,
	    this->events_,
	    this->size_,
	    static_cast<int>(timeout));
    if (nfds > 0)
    {
	this->start_pevents_ = this->events_;
	this->end_pevents_   = this->start_pevents_ + nfds;
    }else
    {
	NDK_DBG ("epoll timeout or errr");
    }
    //NDK_INF ("epoll return %d active fd\n", nfds);
    // If timers are pending, override any timeout from the epoll.
    return ((nfds == 0 && timers_pending != 0) ? 1 : nfds);
}
int EpollReactor::handle_events (const TimeValue *max_wait_time/* = 0*/) 
{
    TRACE ("EpollReactor");
    Guard_T<EpollReactorToken> g (this->token_);

    if (Thread::thr_equal (Thread::self (), this->owner_) == 0
	    || this->deactivated_)
	return -1;

    return this->handle_events_i (max_wait_time);
}
int EpollReactor::handle_events_i (const TimeValue *max_wait_time) 
{
    TRACE ("EpollReactor");
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
    return this->dispatch ();
}
int EpollReactor::dispatch ()
{
    TRACE ("EpollReactor");
    int result = 0;
    // Handle timers early since they may have higher latency
    // constraints than I/O handlers.  Ideally, the order of
    // dispatching should be a strategy...

    if ((result = this->dispatch_timer_handler ()) != 0)
	return result;

    // Check to see if there are no more I/O handles left to
    // dispatch AFTER we've handled the timers.
    //
    result = this->dispatch_io_event ();
    return result;
}
int EpollReactor::dispatch_timer_handler ()
{
    TRACE ("EpollReactor");
    if (this->timer_queue_->is_empty ())
	return 0;

    this->timer_queue_->expire (NDK::gettimeofday ());
    return 0;
}
int EpollReactor::dispatch_io_event ()
{
    TRACE ("EpollReactor");
    struct epoll_event *& pfd = this->start_pevents_;
    int result = 0;
    while (pfd < this->end_pevents_)
    {
	// If more than one event comes in between epoll_wait(2) calls,
	// they will be combined reported.
	if (pfd->events == 0)
	{
	    ++pfd;
	    continue;
	}
	// 
	EventHandler *eh = this->handler_rep_.find (pfd->data.fd);
	if (eh)
	{
	    // Note that if there's an error (such as the handle was closed
	    // without being removed from the event set) the EPOLLHUP and/or
	    // EPOLLERR bits will be set in pfd->events.
	    if (NDK_BIT_ENABLED (pfd->events, EPOLLOUT))
	    {
		NDK_CLR_BITS (pfd->events, EPOLLOUT);
		if (this->upcall (eh, 
			    &EventHandler::handle_output, 
			    pfd->data.fd) < 0)
		    this->remove_handler (pfd->data.fd, 
			    EventHandler::WRITE_MASK);
		++result;
	    }
	    else if (NDK_BIT_ENABLED (pfd->events, EPOLLPRI))
	    {
		NDK_CLR_BITS (pfd->events, EPOLLPRI);
		if (this->upcall (eh, 
			    &EventHandler::handle_exception, 
			    pfd->data.fd) < 0)
		    this->remove_handler (pfd->data.fd, 
			    EventHandler::EXCEPT_MASK);
		++result;
	    }
	    else if (NDK_BIT_ENABLED (pfd->events, EPOLLIN))
	    {
		NDK_CLR_BITS (pfd->events, EPOLLIN);
		if (this->upcall (eh, 
			    &EventHandler::handle_input, 
			    pfd->data.fd) < 0)
		    this->remove_handler (pfd->data.fd, 
			    EventHandler::READ_MASK);
		++result;
	    }
	    else if (NDK_BIT_ENABLED (pfd->events, EPOLLHUP | EPOLLERR))
	    {
		this->remove_handler (pfd->data.fd, 
			EventHandler::ALL_EVENTS_MASK);
		++pfd;
		++result;
	    }
	    else
	    {
		NDK_INF ("dispatch_io [handle = %d] trigger unknown events 0x%x\n",
			pfd->data.fd, pfd->events);
		++pfd;
	    }
	}else
	{
	    NDK_INF ("dispatch_io [handle = %d] not match event handler\n", 
		    pfd->data.fd);
	    ++pfd;
	}
    }
    return result;
}
int EpollReactor::register_handler_i (NDK_HANDLE handle,
	EventHandler *event_handler,
	ReactorMask mask)
{
    TRACE ("EpollReactor");
    if (handle == NDK_INVALID_HANDLE
	    || event_handler == 0
	    || mask == EventHandler::NULL_MASK)
	return -1;
    if (this->handler_rep_.find (handle) == 0)
    {
	// Handler not present in the repository.  Bind it.
	if (this->handler_rep_.bind (handle, event_handler, mask) != 0)
	    return -1;
	struct epoll_event epev;
	::memset (&epev, 0, sizeof (epev));
	epev.events  = this->reactor_mask_to_epoll_event (mask);
	epev.data.fd = handle;
	if (::epoll_ctl (this->epoll_fd_, EPOLL_CTL_ADD, handle, &epev) == -1)
	{
	    this->handler_rep_.unbind (handle);
	    NDK_DBG ("epoll_ctl");
	    return -1;
	}
	++this->curr_payload_;
    }else
    {
	// Handler is already present in the repository, so register it
	// again, possibly for different event.  Add new mask to the
	// current one
	return this->mask_opt_i (handle, mask, Reactor::ADD_MASK);
    }
    return 0;
}
size_t EpollReactor::reactor_mask_to_epoll_event (ReactorMask mask)
{
    TRACE ("EpollReactor");
    if (mask == EventHandler::NULL_MASK)
	return EPOLL_CTL_DEL;
    size_t events = 0;
    // READ, ACCEPT, and CONNECT flag will place the handle in the
    // read set.
    if (NDK_BIT_ENABLED (mask, EventHandler::READ_MASK)
	    || NDK_BIT_ENABLED (mask, EventHandler::ACCEPT_MASK)
	    || NDK_BIT_ENABLED (mask, EventHandler::CONNECT_MASK))
	NDK_SET_BITS (events, EPOLLIN);
    // WRITE and CONNECT flag will place the handle in the write set.
    if (NDK_BIT_ENABLED (mask, EventHandler::WRITE_MASK)
	    || NDK_BIT_ENABLED (mask, EventHandler::CONNECT_MASK))
	NDK_SET_BITS (events, EPOLLOUT);
    // EXCEPT flag will place the handle in the except set.
    if (NDK_BIT_ENABLED (mask, EventHandler::EXCEPT_MASK))
	NDK_SET_BITS (events, EPOLLPRI);
    return events;
}
int EpollReactor::mask_opt_i (NDK_HANDLE handle,
	ReactorMask mask,
	int opt)
{
    TRACE ("EpollReactor");
    const ReactorMask old_mask = this->handler_rep_.mask (handle);
    ReactorMask new_mask = old_mask;
    switch (opt)
    {
    case Reactor::ADD_MASK:
	NDK_SET_BITS (new_mask, mask);
	break;
    case Reactor::SET_MASK:
	new_mask = mask;
	break;
    case Reactor::CLR_MASK:
	NDK_CLR_BITS (new_mask, mask);
	break;
    default:
	return -1;
    }
    this->handler_rep_.mask (handle, new_mask);

    struct epoll_event epev;
    ::memset (&epev, 0, sizeof (epev));
    epev.data.fd = handle;

    if (new_mask == EventHandler::NULL_MASK)
    {
	epev.events = 0;
	return ::epoll_ctl (this->epoll_fd_, EPOLL_CTL_DEL, handle, &epev);	
    }else
    {
	epev.events = this->reactor_mask_to_epoll_event (new_mask);
	if (::epoll_ctl (this->epoll_fd_, EPOLL_CTL_MOD, handle, &epev) == -1)
	{
	    // If a handle is closed, epoll removes it from the poll set
	    // automatically - we may not know about it yet. If that's the
	    // case, a mod operation will fail with ENOENT. Retry it as
	    // an add.
	    if (errno == ENOENT)
		return ::epoll_ctl (this->epoll_fd_, 
			EPOLL_CTL_ADD, 
			handle, &epev);
	}
    }
    return 0;
}
int EpollReactor::deactivated (void) 
{
    TRACE ("EpollReactor");
    return this->deactivated_;
}
void EpollReactor::deactivate (int do_stop) 
{
    TRACE ("EpollReactor");
    this->deactivated_ = do_stop;

}
int EpollReactor::register_handler (EventHandler *event_handler, 
	ReactorMask mask) 
{
    TRACE ("EpollReactor");
    Guard_T<EpollReactorToken> g (this->token_);
    return this->register_handler_i (event_handler->handle (),
	    event_handler,
	    mask);
}
int EpollReactor::register_handler (NDK_HANDLE io_handle, 
	EventHandler *event_handler, 
	ReactorMask mask) 
{
    TRACE ("EpollReactor");
    Guard_T<EpollReactorToken> g (this->token_);
    return this->register_handler_i (io_handle,
	    event_handler,
	    mask);
}
int EpollReactor::remove_handler_i (NDK_HANDLE handle,
	ReactorMask mask)
{
    TRACE ("EpollReactor");
    EventHandler *eh = this->handler_rep_.find (handle);
    if (eh == 0 ||
	    this->mask_opt_i (handle, mask, Reactor::CLR_MASK) == -1)
	return -1;
    if (NDK_BIT_DISABLED (mask, EventHandler::DONT_CALL))
	eh->handle_close (handle, mask);
    // If there are no longer any outstanding events on the given handle
    // then remove it from the handler repository.
    if (this->handler_rep_.mask (handle) == EventHandler::NULL_MASK)
    {
	this->handler_rep_.unbind (handle);
	--this->curr_payload_;
    }
    return 0;
}
int EpollReactor::remove_handler (EventHandler *event_handler, 
	ReactorMask mask) 
{
    TRACE ("EpollReactor");
    //Guard_T<HR_MUTEX> g (this->handler_rep_lock_);
    Guard_T<EpollReactorToken> g (this->token_);
    return this->remove_handler_i (event_handler->handle (),
	    mask);
}
int EpollReactor::remove_handler (NDK_HANDLE io_handle, 
	ReactorMask mask) 
{
    TRACE ("EpollReactor");
    Guard_T<EpollReactorToken> g (this->token_);
    return this->remove_handler_i (io_handle,
	    mask);
}
int EpollReactor::schedule_timer (EventHandler *event_handler,
	const void *arg,
	const TimeValue &delay,
	const TimeValue &interval/* = TimeValue::zero*/)
{
    TRACE ("EpollReactor");
    if (0 == this->timer_queue_) return -1;

    Guard_T<EpollReactorToken> g (this->token_);
    return this->timer_queue_->schedule (event_handler,
	    arg,
	    delay,
	    interval);
}
int EpollReactor::reset_timer_interval (int timer_id,
	const TimeValue &interval)
{
    TRACE ("EpollReactor"); 
    if (0 == this->timer_queue_) return -1;

    Guard_T<EpollReactorToken> g (this->token_);
    return this->timer_queue_->reset_interval (timer_id, interval);
}
int EpollReactor::cancel_timer (int timer_id,
	const void **arg/* = 0*/,
	int dont_call_handle_close/* = 1*/)
{
    TRACE ("EpollReactor"); 
    if (0 == this->timer_queue_) return -1;

    Guard_T<EpollReactorToken> g (this->token_);
    return this->timer_queue_->cancel (timer_id,
	    arg,
	    dont_call_handle_close);
}
int EpollReactor::cancel_timer (EventHandler *event_handler,
	int dont_call_handle_close/* = 1*/)
{
    TRACE ("EpollReactor");
    if (0 == this->timer_queue_) return 0;

    Guard_T<EpollReactorToken> g (this->token_);
    return this->timer_queue_->cancel (event_handler,
	    dont_call_handle_close);
}
size_t EpollReactor::size (void) const
{
    TRACE ("EpollReactor");
    return this->size_;
}
size_t EpollReactor::curr_payload ()
{
    TRACE ("EpollReactor");
    return this->curr_payload_.value ();
}
thread_t EpollReactor::owner ()
{
    Guard_T<EpollReactorToken> g (this->token_);
    return this->owner_;
}
void EpollReactor::owner (thread_t thr_id)
{
    Guard_T<EpollReactorToken> g (this->token_);
    this->owner_ = thr_id;
}


