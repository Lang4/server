inline
int Thread::spawn_n (size_t thr_num, 
	THREAD_FUNC_T func, 
	void *arg/* = 0*/, 
	thread_t *thr_id/* = 0*/, 
	int flags/* = THR_JOIN*/, 
	size_t stack_size/* = 0*/)
{
    thread_t tid = 0;
    size_t i;
    for (i = 0; i < thr_num; ++i)
    {
	if (Thread::spawn (func, arg, &tid, flags, stack_size) == 0)
	{
	    if (thr_id != 0 && tid != 0)
		thr_id[i] = tid;
	}else
	    break;
    }
    return i;
}
inline
int Thread::join (thread_t thr_id)
{
    SET_ERRNO_RETURN (::pthread_join (thr_id, NULL));
}
inline
int Thread::detach (thread_t thr_id)
{
    SET_ERRNO_RETURN (::pthread_detach (thr_id));
}
inline
thread_t Thread::self ()
{
    return ::pthread_self ();
}
inline
int Thread::thr_equal (thread_t thr_id_1, thread_t thr_id_2)
{
    return ::pthread_equal (thr_id_1, thr_id_2);
}
inline
int Thread::kill (thread_t thr_id, int signum)
{
    SET_ERRNO_RETURN (::pthread_kill (thr_id, signum));
}
inline
void Thread::yield ()
{
    ::sched_yield ();
}
inline
int Thread::cancel (thread_t thr_id)
{
    SET_ERRNO_RETURN (::pthread_cancel (thr_id));
}
inline
void Thread::testcancel ()
{
    ::pthread_testcancel ();
}


