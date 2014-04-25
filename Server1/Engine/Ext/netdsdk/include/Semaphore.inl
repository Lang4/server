inline
Semaphore::Semaphore (int init_num/* = 0*/)
{
    ::sem_init (&sem_, 0, init_num);
}
inline
Semaphore::~Semaphore ()
{
    size_t retry = 0;
    while ((::sem_destroy (&sem_) == -1) && retry < 3)
    {
	if (errno == EBUSY)
	{
	    retry ++;
	    this->post ();
	    Thread::yield ();
	}else 
	    break;
    }
}
inline
int Semaphore::wait ()
{
    int ret = -1;
    do{
	ret = ::sem_wait (&sem_);
    }while (ret == -1 && errno == EINTR);
    return ret; 
}
inline
int Semaphore::wait (const TimeValue *timeout)
{
    if (timeout == 0)
	return this->wait ();
#if 0
    struct timespec timeout;
    struct timeval  tp;
    ::gettimeofday (&tp, 0);
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
    int ret = -1;
    do{
	ret = ::sem_timedwait (&sem_, &ts);
    }while (ret == -1 && errno == EINTR);
    return ret;
}
inline
int Semaphore::trywait ()
{
    return ::sem_trywait (&sem_);
}
inline
int Semaphore::post ()
{
    return ::sem_post (&sem_);
}
inline
int Semaphore::post (int post_count)
{
    int ret = 0;
    int n = post_count;
    while (n-- > 0)
	if (::sem_post (&sem_) == 0) ret++;
    return post_count != ret ? -1 : ret;
}

