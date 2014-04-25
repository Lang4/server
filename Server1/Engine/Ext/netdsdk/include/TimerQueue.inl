#include "Debug.h"
inline
TimerNode::TimerNode()
: timer_id_ (-1)
, handler_ (0)
, arg_ (0)
, timer_value_ (0)
, interval_ (0)
, prev_ (0)
, next_ (0)
{
    //NDK_INF ("TimerNode construct\n");
}
inline
TimerNode::~TimerNode()
{
    //NDK_INF ("TimerNode destruct\n");
}
inline
void TimerNode::set (int timer_id,
	const EventHandler *handler,
	const void *arg,
	const TimeValue &t,
	const TimeValue &i,
	TimerNode *p/* = 0*/,
	TimerNode *n/* = 0*/)
{
    this->timer_id_ = timer_id;
    this->handler_  = const_cast<EventHandler *>(handler);
    this->arg_      = arg;
    this->timer_value_ = t;
    this->interval_ = i;
    this->prev_ = p;
    this->next_ = n;
}
inline
TimerNode &TimerNode::operator = (const TimerNode &tn)
{
    this->timer_id_ = tn.timer_id_;
    this->handler_  = tn.handler_;
    this->arg_	    = tn.arg_;
    this->timer_value_ = tn.timer_value_;
    this->interval_ = tn.interval_;
    this->prev_	    = tn.prev_;
    this->next_	    = tn.next_;
    return *this;
}
inline
EventHandler *TimerNode::handler ()
{
    return this->handler_;
}
inline
void TimerNode::handler (EventHandler *h)
{
    this->handler_ = h;
}
inline
const void *TimerNode::arg ()
{
    return this->arg_;
}
inline
void TimerNode::arg (void *arg)
{
    this->arg_ = arg;
}
inline
const TimeValue &TimerNode::timer_value () const
{
    return this->timer_value_;
}
inline
void TimerNode::timer_value (const TimeValue &t)
{
    this->timer_value_ = t;
}
inline
const TimeValue &TimerNode::interval () const
{
    return this->interval_;
}
inline
void TimerNode::interval (const TimeValue &i)
{
    this->interval_ = i;
}
inline
TimerNode *TimerNode::prev ()
{
    return this->prev_;
}
inline
void TimerNode::prev (TimerNode* p)
{
    this->prev_ = p;
}
inline
TimerNode *TimerNode::next ()
{
    return this->next_;
}
inline
void TimerNode::next (TimerNode *n)
{
    this->next_ = n;
}
inline
int TimerNode::timer_id () const
{
    return this->timer_id_;
}
inline
void TimerNode::timer_id (int id)
{
    this->timer_id_ = id;
}
//--------------------------------------------------------------------------
inline
int TimerQueue::is_empty () const
{
    return this->cur_size_ == 0;
}

