inline
ConditionThreadMutex::ConditionThreadMutex (ThreadMutex &mutex)
: mutex_ (mutex)
{
    ::pthread_cond_init (&this->cond_, NULL);
}
inline
ConditionThreadMutex::~ConditionThreadMutex ()
{
    int retry = 0;
    while ((::pthread_cond_destroy (&this->cond_) == EBUSY) 
	    && retry++ < 3)
    {
	this->broadcast ();
	Thread::yield ();
    }
}
inline
int ConditionThreadMutex::wait ()
{
    SET_ERRNO_RETURN (::pthread_cond_wait (&this->cond_, 
	    const_cast<pthread_mutex_t*>(&this->mutex_.lock ())));
}
inline
int ConditionThreadMutex::wait (const TimeValue *timeout)
{
    if (timeout == 0)
	return this->wait ();
#if 0
    struct timespec timeout;
    struct timeval  tp;
    ::gettimeofday (&tp, NULL);
    timeout.tv_sec   = (msec / 1000) + tp.tv_sec;
    timeout.tv_nsec  = ((msec % 1000) * 1000000) + (tp.tv_usec * 1000);
    while (timeout.tv_nsec >= 1000000000)
    {
	timeout.tv_nsec -= 1000000000;
	timeout.tv_sec++;
    }
#endif
    struct timespec ts;
    ts = NDK::gettimeofday () + *timeout;
    // shall not return an error code of [EINTR]
    SET_ERRNO_RETURN (::pthread_cond_timedwait (&this->cond_, 
	    const_cast<pthread_mutex_t*>(&this->mutex_.lock ()), 
	    &ts));
}
inline
int ConditionThreadMutex::signal ()
{
    SET_ERRNO_RETURN (::pthread_cond_signal (&this->cond_));
}
inline
int ConditionThreadMutex::broadcast ()
{
    SET_ERRNO_RETURN (::pthread_cond_broadcast (&this->cond_));
}
inline
ThreadMutex &ConditionThreadMutex::mutex ()
{
    return this->mutex_;
}
