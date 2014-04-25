inline
int Token::acquire (void (*sleep_hook_func)(void *),
	void *arg/* = 0*/,
	const TimeValue *timeout/* = 0*/)
{
    return this->shared_acquire (sleep_hook_func, arg, timeout, WRITE_TOKEN);
}
inline
int Token::acquire (const TimeValue *timeout/* = 0*/)
{
    return this->shared_acquire (0, 0, timeout, WRITE_TOKEN);
}
inline
int Token::acquire_read (void (*sleep_hook_func)(void *),
	void *arg,
	const TimeValue *timeout/* = 0*/)
{
    return this->shared_acquire (sleep_hook_func, arg, timeout, READ_TOKEN);
}
inline
void Token::sleep_hook ()
{
    return ;
}
inline
int Token::tryacquire ()
{
    return this->shared_acquire (0, 0, 0, WRITE_TOKEN);
}
inline
int Token::waiters ()
{
    Guard_T<ThreadMutex> g (this->lock_);
    return this->waiters_;
}
inline
thread_t Token::current_owner ()
{
    Guard_T<ThreadMutex> g (this->lock_);
    return this->owner_;
}
//----------------------------------------------------------------------
inline
Token::TokenQueueEntry::TokenQueueEntry (ThreadMutex &lock,
	thread_t thr_id)
: runable_ (0)
, next_ (0)
, thread_id_ (thr_id)
, cond_ (lock)
{
    TRACE ("Token::TokenQueueEntry");
}
inline
int Token::TokenQueueEntry::wait (const TimeValue *timeout)
{
    return this->cond_.wait (timeout);
}
inline
int Token::TokenQueueEntry::signal ()
{
    return this->cond_.signal ();
}
//----------------------------------------------------------------------
inline
Token::TokenQueue::TokenQueue ()
: head_ (0)
, tail_ (0)
{
    TRACE ("Token::TokenQueue");
}

