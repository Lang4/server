#include "Reactor.h"
#include "Guard_T.h"
#include "EpollReactor.h"

Reactor *Reactor::reactor_ = 0;
ThreadMutex Reactor::instance_lock_;

Reactor::Reactor (ReactorImpl* implementation/* = 0*/,
	int delete_implementation/* = 0*/)
: implementation_ (implementation)
, delete_implementation_ (delete_implementation)
{
    TRACE ("Reactor");
    if (this->implementation_ == 0)
    {
	/**
	 * if Reactor construct failed, Reactor would't destruct, and 
	 * EpollReactor that constructed here would't be destruct, so
	 * we use auto_ptr to avoid that
	 */
	std::auto_ptr<EpollReactor> impl(new EpollReactor);  
	// must use this->, because class construct not completely
	implementation_ = impl.release ();
	delete_implementation_ = 1;
    } 
}
Reactor::~Reactor ()
{
    TRACE ("Reactor");
    this->implementation ()->close ();
    if (this->delete_implementation_ && this->implementation_)
	delete this->implementation_;
}
Reactor *Reactor::instance ()
{
    if (Reactor::reactor_ == 0)
    {
	Guard_T<ThreadMutex> g (Reactor::instance_lock_);
	if (Reactor::reactor_ == 0)
	{
	    Reactor::reactor_ = new Reactor();
	}
    }
    return Reactor::reactor_;
}
Reactor *Reactor::instance (Reactor *r) 
{
    Guard_T<ThreadMutex> g (Reactor::instance_lock_);
    Reactor *t = Reactor::reactor_;
    Reactor::reactor_ = r;
    return t;
}
void Reactor::close_singleton (int delete_reactor/* = 1*/)
{
    if (delete_reactor)
    {
	delete Reactor::reactor_;
	Reactor::reactor_ = 0;
    }
}
void Reactor::end_reactor_event_loop ()
{
    TRACE ("Reactor");
    this->implementation_->deactivate (1);
}
void Reactor::reset_reactor_event_loop ()
{
    TRACE ("Reactor");
    this->implementation_->deactivate (0);
}
int Reactor::reactor_event_loop_done ()
{
    TRACE ("Reactor");
    return this->implementation_->deactivated ();
}
int Reactor::handle_events (const TimeValue *max_wait_time/* = 0*/)
{
    return this->implementation ()->handle_events (max_wait_time);
}
int Reactor::run_reactor_event_loop ()
{
    TRACE ("Reactor");
    this->implementation_->owner (Thread::self ());
    if (this->reactor_event_loop_done ())
	return 0;

    while (1)
    {
	int result = this->implementation_->handle_events ();
	if (result == -1 && this->implementation_->deactivated ())
	    return 0;
	else if (result == -1)
	    return -1;
    }
    return 0;
}
int Reactor::register_handler (NDK_HANDLE handle,
	EventHandler *event_handler, 
	ReactorMask mask)
{
    // Remember the old reactor.
    Reactor *old_reactor = event_handler->reactor ();
    // Assign *this* <Reactor> to the <EventHandler>.
    event_handler->reactor (this);
    int result = this->implementation ()->register_handler (handle, 
	    event_handler, mask);

    if (result == -1)
	event_handler->reactor (old_reactor);
    return result;
}
int Reactor::register_handler (EventHandler *event_handler, 
	ReactorMask mask)
{
    // Remember the old reactor.
    Reactor *old_reactor = event_handler->reactor ();
    // Assign *this* <Reactor> to the <EventHandler>.
    event_handler->reactor (this);
    int result = this->implementation ()->register_handler (
	    event_handler, 
	    mask);

    if (result == -1)
	event_handler->reactor (old_reactor);
    return result;
}
int Reactor::remove_handler (NDK_HANDLE handle,
	ReactorMask mask)
{
    return this->implementation ()->remove_handler (handle, mask);
}
int Reactor::remove_handler (EventHandler *event_handler,
	ReactorMask mask)
{
    return this->implementation ()->remove_handler (event_handler, mask);
}
int Reactor::schedule_timer (EventHandler *event_handler,
	    const void *arg,
	    const TimeValue &delay,
	    const TimeValue &interval/* = TimeValue::zero*/)
{
    return this->implementation ()->schedule_timer (event_handler,
	    arg,
	    delay,
	    interval);
}
int Reactor::reset_timer_interval (int timer_id,
	    const TimeValue &interval)
{
    return this->implementation ()->reset_timer_interval (timer_id,
	    interval);
}
int Reactor::cancel_timer (int timer_id,
	    const void **arg/* = 0*/,
	    int dont_call_handle_close/* = 1*/)
{
    return this->implementation ()->cancel_timer (timer_id,
	    arg,
	    dont_call_handle_close);
}
int Reactor::cancel_timer (EventHandler *event_handler,
	    int dont_call_handle_close/* = 1*/)
{
    return this->implementation ()->cancel_timer (event_handler,
	    dont_call_handle_close);
}
size_t Reactor::size ()
{
    return this->implementation ()->size ();
}
