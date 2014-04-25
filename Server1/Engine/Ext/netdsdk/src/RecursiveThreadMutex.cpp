#include "RecursiveThreadMutex.h"
#include "Thread.h"
#include "Debug.h"

int RecursiveThreadMutex::acquire ()
{
    thread_t t_id = Thread::self ();
    if (Thread::thr_equal (this->owner_thr_, t_id))
	++this->nesting_level_;
    else
    {
	int ret = 0;
	if ((ret = ::pthread_mutex_lock (&this->mutex_)) == 0)
	{
	    this->owner_thr_ = t_id;
	    this->nesting_level_ = 0;
	}else
	{
	    NDK_DBG ("RecursiveThreadMutex->pthread_mutex_lock failed");
	    errno = ret;
	    return -1;
	}
    }
    return 0;
}
int RecursiveThreadMutex::acquire (int msec)
{
    thread_t t_id = Thread::self ();
    if (Thread::thr_equal (this->owner_thr_, t_id))
	++this->nesting_level_;
    else
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
	int ret = 0;
	if ((ret = ::pthread_mutex_timedlock (&this->mutex_, &ts)) == 0)
	{
	    this->owner_thr_ = t_id;
	    this->nesting_level_ = 0;
	}else
	{
	    NDK_DBG ("RecursiveThreadMutex->pthread_mutex_timedlock failed");
	    errno = ret;
	    return -1;
	}
    }
    return 0;
}
int RecursiveThreadMutex::release ()
{
    if (Thread::thr_equal (Thread::self (), this->owner_thr_))
    {
	if (this->nesting_level_ > 0)
	    --this->nesting_level_;
	else
	{
	    this->owner_thr_ = 0;
	    SET_ERRNO_RETURN (::pthread_mutex_unlock (&this->mutex_));
	}
    }else
    {
	NDK_DBG ("RecursiveThreadMutex: This thread is't the owner of the lock");
	return -1;
    }
    return 0;
}
int RecursiveThreadMutex::tryacquire ()
{
    thread_t t_id = Thread::self ();
    if (Thread::thr_equal (this->owner_thr_, t_id))
	++this->nesting_level_;
    else
    {
	int ret = 0;
	if ((ret = ::pthread_mutex_trylock (&this->mutex_)) == 0)
	{
	    this->owner_thr_ = t_id;
	    this->nesting_level_ = 0;
	}else
	{
	    errno = ret;
	    return -1;
	}
    }
    return 0;
}

