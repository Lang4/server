#include "Token.h"
#include "Debug.h"

Token::Token (int q_strategy/* = FIFO*/)
: waiters_ (0)
, in_use_ (0)
, nesting_level_ (0)
, queueing_strategy_ (q_strategy)
, owner_ (NULL_thread)
, lock_ ()
{
}
Token::~Token ()
{
}
int Token::shared_acquire (void (*sleep_hook_func)(void *),
	void *arg,
	const TimeValue * timeout,
	TokenOptType opt_type)
{
    Guard_T<ThreadMutex> g (this->lock_);

    thread_t thr_id = Thread::self ();
    if (!this->in_use_)
    {
	this->in_use_ = opt_type;
	this->owner_  = thr_id;
	return 0;
    }

    // Someone already holds the lock
    if (Thread::thr_equal (thr_id, this->owner_))
    {
	++this->nesting_level_;
	return 0;
    }

    //
    // We've got to sleep until we get the token.
    //

    TokenQueue *queue = (opt_type == READ_TOKEN 
	    ? &this->readers_
	    : &this->writers_);
    // Allocate queue entry on stack.  This works since we don't exit
    // this method's activation record until we've got the token.
    Token::TokenQueueEntry my_entry (this->lock_,
	    thr_id);
    queue->insert_entry (my_entry, this->queueing_strategy_);

    ++this->waiters_;

    if (sleep_hook_func)
    {
	(*sleep_hook_func) (arg);
    }else
    {
	this->sleep_hook ();
    }

    // Sleep until we've got the token
    int time_out = 0;
    int error = 0;
    do
    {
	int result = my_entry.wait (timeout);
	if (result == -1)
	{
	    if (errno == EINTR)
		continue;
	    if (errno == ETIMEDOUT)
		time_out = 1;
	    else
		error = 1;
	    this->wakeup_next_waiter ();
	    break;
	}
    }while (!Thread::thr_equal (thr_id, this->owner_));
    // Do this always and irrespective of the result of wait().
    --this->waiters_;
    queue->remove_entry (&my_entry);

    if (time_out)
    {
	// This thread was still selected to own the token.
	if (my_entry.runable_)
	{
	    // Wakeup next waiter since this thread timed out.
	    this->wakeup_next_waiter ();
	}
	return -1;
    }else if (error == 1)
	return -1;
    // If this is a normal wakeup, this thread should be runnable.
    NDK_ASSERT (my_entry.runable_);
    return 0;
}
int Token::release ()
{
    Guard_T<ThreadMutex> g (this->lock_);
    
    if (this->nesting_level_ > 0)
	--this->nesting_level_;
    else
    {
	this->wakeup_next_waiter ();
    }
    return 0;
}
void Token::wakeup_next_waiter ()
{
    this->in_use_ = 0;
    this->owner_  = NULL_thread;

    // Any waiters ...
    if (this->writers_.head_ == 0 
	    && this->readers_.head_ == 0)
    {
	// No more waiters
	return ;
    }
    // Wakeup next waiter
    TokenQueue *queue = 0;
    // Writer threads get priority to run first.
    if (this->writers_.head_ != 0)
    {
	this->in_use_ = Token::WRITE_TOKEN;
	queue = &this->writers_;
    }else
    {
	this->in_use_ = Token::READ_TOKEN;
	queue = &this->readers_;
    }
    // Wake up waiter and make it runable.
    queue->head_->runable_ = 1;
    this->owner_ = queue->head_->thread_id_;
    queue->head_->signal ();
}
//-----------------------------------------------------------------
void Token::TokenQueue::remove_entry (Token::TokenQueueEntry *entry)
{
    TRACE ("Token::TokenQueue");
    Token::TokenQueueEntry *curr = 0;
    Token::TokenQueueEntry *prev = 0;

    if (this->head_ == 0)
	return ;
    for (curr = this->head_; 
	    curr != 0 && curr != entry;
	    curr = curr->next_)
	prev = curr;
    if (curr == 0) // Didn't find the entry...
	return ;
    else if (prev == 0) // Delete at the head
	this->head_ = this->head_->next_;
    else  // Delete in the middle.
	prev->next_ = curr->next_;
    // We need to update the tail of the list if we've deleted the last
    // entry.
    if (curr->next_ == 0)
	this->tail_ = prev;
}
void Token::TokenQueue::insert_entry (Token::TokenQueueEntry &entry,
	int queue_strategy/* = FIFO*/)
{
    if (this->head_ == 0)
    {
	// No other threads - just add me
	this->head_ = &entry;
	this->tail_ = &entry;
    }else if (queue_strategy == LIFO)
    {
	// Insert at head of queue
	entry.next_ = this->head_;
	this->head_ = &entry;
    }else // if (queue_strategy == FIFO)
    {
	// Insert at the end of the queue.
	this->tail_->next_ = &entry;
	this->tail_ = &entry;
    }
}

