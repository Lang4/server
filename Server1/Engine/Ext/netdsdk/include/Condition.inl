template<typename MUTEX> inline
Condition<MUTEX>::Condition (MUTEX &m)
: mutex_ (m)
{
    ::pthread_cond_init (&this->cond_, NULL);
}
template<typename MUTEX> inline
Condition<MUTEX>::~Condition ()
{
    int retry = 0;
    while ((::pthread_cond_destroy (&this->cond_) == EBUSY) 
	    && retry++ < 3)
    {
	this->broadcast ();
	Thread::yield ();
    }
}
template<typename MUTEX> inline
int Condition<MUTEX>::wait ()
{
    SET_ERRNO_RETURN (::pthread_cond_wait (&this->cond_, 
	    const_cast<pthread_mutex_t*>(&this->mutex_.lock ())));
}
template<typename MUTEX> inline
int Condition<MUTEX>::wait (const TimeValue *timeout)
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
template<typename MUTEX> inline
int Condition<MUTEX>::signal ()
{
    SET_ERRNO_RETURN (::pthread_cond_signal (&this->cond_));
}
template<typename MUTEX> inline
int Condition<MUTEX>::broadcast ()
{
    SET_ERRNO_RETURN (::pthread_cond_broadcast (&this->cond_));
}
template<typename MUTEX> inline
MUTEX &Condition<MUTEX>::mutex ()
{
    return this->mutex_;
}


