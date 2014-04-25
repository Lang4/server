#ifndef _MUTLTI_REACTORS_CPP_
#define _MUTLTI_REACTORS_CPP_

#include "NDK.h"
#include "Trace.h"
#include "Debug.h"
#include "Guard_T.h"
#include "MultiReactors.h"
#include "ThreadManager.h"

template<typename REACTOR>
MultiReactors<REACTOR>::MultiReactors ()
: strategy_ (REACTOR_NUM_FIXED)
, reactor_count_ (4)
, max_reactor_count_ (64)		    
, max_handles_ (0)					    
, reactor_index_ (0)
, single_reactor_capacity_ (0)
, reactors_ (0){
}
template<typename REACTOR>
MultiReactors<REACTOR>::~MultiReactors ()
{
    TRACE ("MultiReactors");
    this->close ();
}
template<typename REACTOR>
int MultiReactors<REACTOR>::init (int strategy/* = REACTOR_NUM_FIXED*/,
       int count,
       int max_handlers/* = 0*/)
{
    TRACE ("MultiReactors");
    // check value
    if (count <= 0) return -1;  // avoid n/0
    // 
    this->max_handles_ = max_handlers;
    if (this->max_handles_ == 0)
    {
	this->max_handles_ = NDK::max_handles ();
	if (this->max_handles_ <= 0)
	{
	    this->close ();
	    return -1;
	}
    }else if (NDK::set_handle_limit (this->max_handles_) != 0)
	return -1;
    this->strategy_ = strategy;
    if (this->strategy_ == REACTOR_NUM_FIXED)
    {
	this->reactor_count_ = count;
	if (this->reactor_count_ <= 0) 
	    return -1;
	this->reactors_  = new REACTOR *[this->reactor_count_];
	for (int i = 0; i < this->reactor_count_; ++i)
	    this->reactors_[i] = 0;

	this->single_reactor_capacity_ 
	    = this->max_handles_ / this->reactor_count_;
	int over = this->max_handles_ % this->reactor_count_;

	for (int i = 0; i < this->reactor_count_; ++i)
	{
	    this->reactors_[i] = new REACTOR(0);
	    // luanch each reactor
	    if (this->reactors_[i])
	    {
		if (this->reactors_[i]->open (i == 0
			    ? this->single_reactor_capacity_ + over
			    : this->single_reactor_capacity_, 
			    0, 
			    false) != 0
			||
			ThreadManager::instance()->spawn (
			    NDK::thr_run_reactor_event_loop, 
			    this->reactors_[i]) == -1)
		{
		    this->close ();
		    return -1;
		}
	    }else
	    {
		this->close ();
		return -1;
	    }
	}
    }else if (this->strategy_ == SINGLE_REACTOR_CAPACITY_FIXED)
    {
	this->single_reactor_capacity_ = count; // !!! Note 1/0
	this->reactor_count_ = 1;
	this->reactor_index_ = 0;
	this->max_reactor_count_ = 
	    this->max_handles_ / this->single_reactor_capacity_;
	int over = this->max_handles_ % this->single_reactor_capacity_;
	if (over > 0)
	    this->max_reactor_count_++;
	this->reactors_    = new REACTOR* [this->max_reactor_count_];
	this->reactors_[this->reactor_index_] = new REACTOR(0);
	if (this->reactors_[this->reactor_index_]->open (
		    this->single_reactor_capacity_, 
		    0, 
		    false) != 0
		||
		ThreadManager::instance()->spawn (
		    NDK::thr_run_reactor_event_loop,
		    this->reactors_[this->reactor_index_]) == -1)
	{
	    this->close ();
	    return -1;
	}
    }else
	return -1;
    return 0; 
}
template<typename REACTOR>
int MultiReactors<REACTOR>::close ()
{
    TRACE ("MultiReactors");
    Guard_T<MUTEX> g (this->mutex_);
    for (int i = 0; i < this->reactor_count_; ++i)
    {
	if (this->reactors_[i])
	    delete this->reactors_[i];
    }
    delete [] this->reactors_;
    this->reactors_ = 0;
    this->single_reactor_capacity_ = 0;
    this->reactor_count_ = 0;
    this->reactor_index_ = 0;
    this->max_reactor_count_ = 0;
    return 0;
}
template<typename REACTOR>
int MultiReactors<REACTOR>::register_handler_i (NDK_HANDLE handle,
	EventHandler *event_handler,
	ReactorMask mask)
{
    TRACE ("MultiReactors");
    if (this->reactors_ == 0) return -1;
    // 1. REACTOR_NUM_FIXED
    if (this->strategy_ == REACTOR_NUM_FIXED)
    {
	int index = 0;
	int min_payload = this->reactors_[index]->curr_payload();
	int payload = 0;
	for (int i = index + 1; i < this->reactor_count_; ++i)
	{
	    payload = this->reactors_[i]->curr_payload();
	    if (payload <= min_payload)
	    {	
		min_payload = payload;
		index = i;
	    }	
	}
	NDK_ASSERT(index < this->reactor_count_);
	NDK_INF ("the %d reactor is low payload %d\n", index, min_payload);
	return this->reactors_[index]->register_handler (handle, 
		event_handler,
		mask);
    }else
    // 2. SINGLE_REACTOR_CAPACITY_FIXED	
	if (this->strategy_ == SINGLE_REACTOR_CAPACITY_FIXED)
	{
	    if (this->reactors_[this->reactor_index_]->curr_payload () 
		    >= this->single_reactor_capacity_)
	    {
		int result = this->append_reactor ();
		if (result != 0)
		{
		    /**
		     * If the number of Reactors is out of max_reactor_count
		     * , then can't append reactor, but it should be poll other
		     * reactors are out of its payload or not.
		     */
		    if (result == -2) // out of limit then poll back
		    {
			int index = 0;
			for (; index < this->reactor_count_; ++index)
			{
			    if (this->reactors_[index]->curr_payload () 
				    < this->single_reactor_capacity_)
			    {
				this->reactor_index_ = index;
				break;
			    }
			}
			if (index == this->reactor_count_)
			    return -1;
		    }else
			return result;
		}
	    }
	    return this->reactors_[this->reactor_index_]->register_handler (
		    handle,
		    event_handler,
		    mask);
	}
    return -1;
}
template<typename REACTOR>
int MultiReactors<REACTOR>::append_reactor ()
{
    TRACE ("MultiReactors");
    if (this->reactors_)
    {
	int index = this->reactor_index_ + 1;
	if (index >= this->max_reactor_count_)
	    return -2;
	this->reactors_[index] = new REACTOR(0);
	if (this->reactors_[index] 
		&& this->reactors_[index]->open (this->single_reactor_capacity_, 
		    0,
		    false) == 0
		&& ThreadManager::instance()->spawn (
		    NDK::thr_run_reactor_event_loop,
		    this->reactors_[index]) != -1)
	{
	    ++this->reactor_count_;
	    ++this->reactor_index_;
	    return 0;
	}
    }
    return -1;
}
template<typename REACTOR>
int MultiReactors<REACTOR>::register_handler (NDK_HANDLE handle,
	EventHandler *event_handler,
	ReactorMask mask)
{
    TRACE ("MultiReactors");
    Guard_T<MUTEX> g (this->mutex_);
    return this->register_handler_i (handle,
	    event_handler,
	    mask);
}
template<typename REACTOR>
int MultiReactors<REACTOR>::register_handler (EventHandler *event_handler,
	ReactorMask mask)
{
    TRACE ("MultiReactors");
    Guard_T<MUTEX> g (this->mutex_);
    return this->register_handler_i (event_handler->handle (),
	    event_handler,
	    mask);
}
template<typename REACTOR>
int MultiReactors<REACTOR>::remove_handler (EventHandler *event_handler,
	ReactorMask mask)
{
    TRACE ("MultiReactors");
    Guard_T<MUTEX> g (this->mutex_);
    return this->remove_handler_i (event_handler->handle(),
	    mask);
}
template<typename REACTOR>
int MultiReactors<REACTOR>::remove_handler (NDK_HANDLE handle,
	ReactorMask mask)
{
    TRACE ("MultiReactors");
    Guard_T<MUTEX> g (this->mutex_);
    return this->remove_handler_i (handle,
	    mask);
}
template<typename REACTOR>
int MultiReactors<REACTOR>::remove_handler_i (NDK_HANDLE handle,
	ReactorMask mask)
{
    TRACE ("MultiReactors");
    return 0;
}
template<typename REACTOR>
int MultiReactors<REACTOR>::schedule_timer (EventHandler *event_handler,
	    const void *arg,
	    const TimeValue &delay,
	    const TimeValue &interval/* = TimeValue::zero*/)
{
    TRACE ("MultiReactors");
    return 0;
}
template<typename REACTOR>
int MultiReactors<REACTOR>::reset_timer_interval (int timer_id,
	    const TimeValue &interval)
{
    TRACE ("MultiReactors");
    return 0;
}
template<typename REACTOR>
int MultiReactors<REACTOR>::cancel_timer (int timer_id,
	    const void **arg/* = 0*/,
	    int dont_call_handle_close/* = 1*/)
{
    TRACE ("MultiReactors");
    return 0;
}
template<typename REACTOR>
int MultiReactors<REACTOR>::cancel_timer (EventHandler *eh,
	    int dont_call_handle_close/* = 1*/)
{
    TRACE ("MultiReactors");
    return 0;
}
template<typename REACTOR>
void MultiReactors<REACTOR>::stop_reactors (int count/* = -1*/)
{
    TRACE ("MultiReactors");
}
template<typename REACTOR>
int MultiReactors<REACTOR>::notify (EventHandler *eh/* = 0*/,
	    ReactorMask mask/* = EventHandler::EXCEPT_MASK*/,
	    const TimeValue *timeout/* = 0*/)
{
    TRACE ("MultiReactors");
    return 0;
}
template<typename REACTOR>
int MultiReactors<REACTOR>::handle_events (const TimeValue *max_wait_time/* = 0*/)
{
    TRACE ("MultiReactors");
    return 0;
}
template<typename REACTOR>
int MultiReactors<REACTOR>::deactivated ()
{
    TRACE ("MultiReactors");
    return 0;
}
template<typename REACTOR>
void MultiReactors<REACTOR>::deactivate (int )
{
    TRACE ("MultiReactors");
}
template<typename REACTOR>
size_t MultiReactors<REACTOR>::size () const
{
    TRACE ("MultiReactors");
    return this->max_handles_;
}
template<typename REACTOR>
size_t MultiReactors<REACTOR>::curr_payload ()
{
    TRACE ("MultiReactors");
    return 0;
}
template<typename REACTOR>
thread_t MultiReactors<REACTOR>::owner ()
{
    TRACE ("MultiReactors");
    return 0;
}
template<typename REACTOR>
void MultiReactors<REACTOR>::owner (thread_t )
{
    TRACE ("MultiReactors");
    return ;
}
#endif

