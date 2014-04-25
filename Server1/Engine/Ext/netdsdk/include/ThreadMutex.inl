inline
ThreadMutex::ThreadMutex ()
{
    ::pthread_mutex_init (&mutex_, NULL);
}
inline
ThreadMutex::~ThreadMutex ()
{
    ::pthread_mutex_destroy (&mutex_);
}
inline
int ThreadMutex::acquire ()
{
    SET_ERRNO_RETURN (::pthread_mutex_lock (&mutex_));
}
inline
int ThreadMutex::acquire (int msec)
{
    struct timespec ts;
    struct timeval  tv;
    ::gettimeofday (&tv, NULL);
    ts.tv_sec   = (msec / 1000) + tv.tv_sec;
    ts.tv_nsec  = ((msec % 1000) * 1000000) + (tv.tv_usec * 1000);
    while (ts.tv_nsec >= 1000000000)
    {
	ts.tv_nsec -= 1000000000;
	ts.tv_sec++;
    }
    SET_ERRNO_RETURN (::pthread_mutex_timedlock (&mutex_, &ts));
}
inline
int ThreadMutex::tryacquire ()
{
    SET_ERRNO_RETURN (::pthread_mutex_trylock (&mutex_));
}
inline
int ThreadMutex::release ()
{
    SET_ERRNO_RETURN (::pthread_mutex_unlock (&mutex_));
}
inline
const pthread_mutex_t &ThreadMutex::lock () const
{
    return mutex_;
}


