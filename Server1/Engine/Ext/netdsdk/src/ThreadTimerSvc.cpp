#include "ThreadTimerSvc.h"

#include "NDK.h"
#include "Guard_T.h"
#include "TimerQueue.h"

ThreadTimerSvc::ThreadTimerSvc (TimerQueue *tq/* = 0*/)
: TaskBase (0)
, delete_timer_queue_ (0)
, active_ (1)
, timer_queue_ (tq)
, mutex_ ()
, condition_ (mutex_)
{
    if (timer_queue_ == 0)
    {
	delete_timer_queue_ = 1;
	timer_queue_ = new TimerQueue();
    }
    if (activate() != 0)
	active_ = 0;
}
ThreadTimerSvc::~ThreadTimerSvc ()
{
    if (this->delete_timer_queue_)
    {
	delete this->timer_queue_;
	this->timer_queue_ = 0;
	this->delete_timer_queue_ = 0;
    }
}
int ThreadTimerSvc::schedule_timer (EventHandler *eh,
	const void *arg,
	const TimeValue &future_time,
	const TimeValue &interval_time/* = TimerValue::zero*/)
{
    Guard_T<MUTEX> g (this->mutex_);
    int result = this->timer_queue_->schedule(eh,
	    arg,
	    future_time,
	    interval_time);
    this->condition_.signal ();
    return result;
}
int ThreadTimerSvc::cancel_timer (int timer_id, const void **arg/* = 0*/)
{
    Guard_T<MUTEX> g (this->mutex_);
    int result = this->timer_queue_->cancel (timer_id, arg);
    this->condition_.signal ();
    return result;
}
int ThreadTimerSvc::reset_timer_interval (int timer_id, 
	const TimeValue &interval)
{
    Guard_T<MUTEX> g (this->mutex_);
    int result = this->timer_queue_->reset_interval (timer_id,
	    interval);
    this->condition_.signal ();
    return result;
}
void ThreadTimerSvc::deactivate ()
{
    Guard_T<MUTEX> g (this->mutex_);
    this->active_ = 0;
    this->condition_.signal ();
}
int ThreadTimerSvc::svc ()
{
    Guard_T<MUTEX> g (this->mutex_);
    while (this->active_)
    {
	// If the queue is empty, sleep until there is a change on it.
	if (this->timer_queue_->is_empty ())
	{
	    this->condition_.wait ();
	    continue;
	}
	else
	{
	    // Compute the remaining time, being careful not to sleep
	    // for "negative" amounts of time.
	    TimeValue wait_time(0);
	    TimeValue *this_time = 0;
	    this_time = this->timer_queue_->calculate_timeout (0, &wait_time);
	    TimeValue *tv =
	       (this_time == 0 ? 0 /* Infinity */
		: this_time/* Point to wait_time */);	
	    this->condition_.wait (tv);
	}
	// Expire timers anyway, at worst this is a no-op.
	this->timer_queue_->expire (NDK::gettimeofday ());
    }
    return 0;
}

