#include "MessageQueue.h"
#include "Common.h"

MessageQueue::~MessageQueue ()
{
    Guard_T<MUTEX> g (this->queue_mutex_);
    if (this->head_ != 0)
    {
	MessageBlock *mb = 0;
	for (this->tail_ = 0; this->head_ != 0; )
	{
	    mb = this->head_;
	    this->head_ = this->head_->next ();
	    delete mb;  // ???
	}
    }
}
int MessageQueue::enqueue_tail (MessageBlock *new_item, 
	const TimeValue *timeout/* = 0*/)
{
    unused_arg (timeout);
    if (new_item == 0) return -1;
    // lock
    int num = 0;
    {
	Guard_T<MUTEX> g (this->queue_mutex_);
	num = this->enqueue_tail_i (new_item);
    }
    this->not_empty_cond_.signal();
    return num;
}
int MessageQueue::enqueue_tail_i (MessageBlock *new_item)
{
    // maybe new_item is a short queue
    MessageBlock *seq_tail = new_item;
    int num = 1;

    // count the number of the sub message queue
    while (seq_tail->next () != 0)
    {
	seq_tail->next ()->prev (seq_tail);
	seq_tail = seq_tail->next ();
	++num;
    }	

    // queue is empty, so build a new one
    if (this->tail_ == 0)
    {
	this->head_ = new_item;
	this->tail_ = seq_tail;
	new_item->prev (0);
    }else  // link at the end
    {
	new_item->prev (this->tail_);
	this->tail_->next (new_item);
	this->tail_ = seq_tail;
    }
    this->mb_count_ += num;
    return num;
} 
int MessageQueue::enqueue_head (MessageBlock *new_item, 
	const TimeValue *timeout/* = 0*/)
{
    unused_arg (timeout);
    if (new_item == 0) return -1;
    // lock
    int num = 0;
    {
	Guard_T<MUTEX> g (this->queue_mutex_);
	num = this->enqueue_head_i (new_item);
    }
    this->not_empty_cond_.signal();
    return num;
}
int MessageQueue::enqueue_head_i (MessageBlock *new_item)
{
    // maybe new_item is a short queue
    MessageBlock *seq_tail = new_item;
    int num = 1;

    // count the number of the sub message queue
    while (seq_tail->next () != 0)
    {
	seq_tail->next ()->prev (seq_tail);
	seq_tail = seq_tail->next ();
	++num;
    }	

    new_item->prev (0);
    seq_tail->next (this->head_);
    if (this->head_ != 0)
	this->head_->prev (seq_tail);  
    else
	this->tail_ = seq_tail;
    this->head_ = new_item;

    return num;
} 
int MessageQueue::dequeue_head (MessageBlock *&first_item, 
	const TimeValue *timeout/* = 0*/)
{
    Guard_T<MUTEX> g (this->queue_mutex_);
    while (this->is_empty_i ())
    {
	if (this->not_empty_cond_.wait (timeout) != 0)
	    return -1;  // timeout
    }
    // wake up
    // lock
    return this->dequeue_head_i (first_item);
}
int MessageQueue::dequeue_head_i (MessageBlock *&first_item)
{    
    if (this->head_ == 0) return -1;  // check empty !!

    first_item   = this->head_;
    this->head_ = this->head_->next ();
    if (this->head_ == 0) 
	this->tail_ = 0;
    else  
	// the prev pointer of first message block must point to 0
	this->head_->prev (0);
    --this->mb_count_;
    if (this->mb_count_ == 0 && this->head_ == this->tail_)
	this->head_ = this->tail_ = 0;
    // clean 
    first_item->prev (0);
    first_item->next (0);
    return 0;
}
int MessageQueue::dequeue_head_n (MessageBlock *&items, 
	int number/* = -1*/, 
	const TimeValue *timeout/* = 0*/)
{
    if (number == 0) return 0;
    Guard_T<MUTEX> g (this->queue_mutex_);
    while (this->is_empty_i ())
    {
	if (this->not_empty_cond_.wait (timeout) != 0)
	    return -1;  // timeout
    }
    // wake up
    // lock
    return this->dequeue_head_n_i (items, number);
}
int MessageQueue::dequeue_head_n_i (MessageBlock *&items, int number)
{
    if (this->head_ == 0) return 0;
    int count = 0;
    if (number == -1)  // dequeue all
    {
	items = this->head_;
	this->head_ = 0;
	this->tail_ = 0;
	count = this->mb_count_;
	this->mb_count_ = 0;
    }
    else
    {
	items = this->head_;
	while (number-- > 0 && this->head_ != 0)
	{
	    this->head_ = this->head_->next ();
	    ++count;
	}
	if (this->head_ == 0)
	{
	    this->tail_ = 0;
	    this->mb_count_ = 0;
	}
	else
	{
	    this->head_->prev ()->next (0); // the items's tail 
	    this->head_->prev (0);  // the prev pointer of the first 
				    // message block must point to 0
	    this->mb_count_ -= count;
	}
    }
    return count;
}
int MessageQueue::dequeue_tail (MessageBlock *&last_item, 
	const TimeValue *timeout/* = 0*/)
{
    Guard_T<MUTEX> g (this->queue_mutex_);
    while (this->is_empty_i ())
    {
	if (this->not_empty_cond_.wait (timeout) != 0)
	    return -1;  // timeout
    }
    // wake up
    // lock
    return this->dequeue_tail_i (last_item);
}
int MessageQueue::dequeue_tail_i (MessageBlock *&last_item)
{
    if (this->head_ == 0) return -1;  // check empty !!

    last_item   = this->tail_;
    if (this->tail_->prev () == 0) // only one mb
    {
	this->head_ = 0;
	this->tail_ = 0;
    }else
    {
	this->tail_->prev ()->next (0);  // set eof
	this->tail_ = this->tail_->prev ();  // 
    }
    
    --this->mb_count_;
    if (this->mb_count_ == 0 && this->head_ == this->tail_)
	this->head_ = this->tail_ = 0;
    // clean 
    last_item->prev (0);
    last_item->next (0);
    return 0;
}
int MessageQueue::dequeue_tail_n (MessageBlock *&items, 
	int number/* = -1*/, 
	const TimeValue *timeout/* = 0*/)
{
    if (number == 0) return 0;  // must check at first
    Guard_T<MUTEX> g (this->queue_mutex_);
    while (this->is_empty_i ())
    {
	if (this->not_empty_cond_.wait (timeout) != 0)
	    return -1;  // timeout
    }
    // wake up
    // lock
    return this->dequeue_tail_n_i (items, number);
}
int MessageQueue::dequeue_tail_n_i (MessageBlock *&items, int number)
{
    if (this->head_ == 0) return 0;  // check empty !!
    int count = 0;
    if (number == -1)  // dequeue all
    {
	items = this->head_;
	this->head_ = 0;
	this->tail_ = 0;
	count = this->mb_count_;
	this->mb_count_ = 0;
    }
    else
    {
	while (number-- > 0 && this->tail_ != 0)
	{
	    this->tail_ = this->tail_->prev ();
	    ++count;
	}
	if (this->tail_ == 0) // not enough
	{
	    items = this->head_;
	    this->head_ = 0;
	    count = this->mb_count_;
	    this->mb_count_ = 0;
	}
	else
	{
	    items = this->tail_->next ();
	    items->prev (0);
	    this->tail_->next (0); // the items's tail 
	    this->mb_count_ -= count;
	}
    }
    return count;
}

