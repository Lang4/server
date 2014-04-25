inline
ThreadManager::ThreadManager ()
: grp_id_ (0x1314)
{
    TRACE ("ThreadManager");
}
inline
ThreadManager* ThreadManager::instance ()
{
    if (ThreadManager::thr_mgr_ == 0)
    {
	Guard_T<ThreadManager::MUTEX> g (inst_mutex_);
	if (ThreadManager::thr_mgr_ == 0)
	    ThreadManager::thr_mgr_ = new ThreadManager;
    }
    return ThreadManager::thr_mgr_;
}
inline
int ThreadManager::spawn (THREAD_FUNC_T func, 
	void *arg/* = 0*/, 
	thread_t *thr_id/* = 0*/, 
	int flags/* = THR_JOIN*/, 
	int grp_id/* = -1*/, 
	size_t stack_size/* = 0*/, 
	TaskBase *task/* = 0*/)
															    
{
    TRACE ("ThreadManager");
    if (grp_id == -1)
	grp_id = this->grp_id_++;

    if (this->spawn_i (func, 
		arg, 
		thr_id, 
		flags, 
		grp_id, 
		stack_size, 
		task) == -1)
	return -1;
    return grp_id;
}
inline
int ThreadManager::spawn_n (size_t thr_num,
	THREAD_FUNC_T func,
	void *arg/*  = 0*/,
	thread_t thr_id[]/* = 0*/,    //
	int flags/* = THR_JOIN*/,
	int grp_id/* = -1*/,
	size_t stack_size/* = 0*/,   // shared
	TaskBase *task/* = 0*/)
{
    TRACE ("ThreadManager");
    if (grp_id == -1)
	grp_id = this->grp_id_++;
    //
    size_t n = 0;
    for (; n < thr_num; ++n)
    {
	if (this->spawn_i (func, 
		    arg, 
		    thr_id == 0 ? 0 : &thr_id[n],
		    flags, 
		    grp_id, 
		    stack_size, 
		    task) == -1)
	    return -1;
    }
    return grp_id;
}
// ------------------ ThreadDescriptor ----------------------
inline
ThreadDescriptor::ThreadDescriptor ()
: grp_id_ (-1)
, thr_state_ (ThreadManager::THR_IDLE)
, thr_flags_ (0)
, thr_id_ (NULL_thread)
, task_ (0)
, thr_mgr_ (0)
{
    TRACE ("ThreadDescriptor");
}
inline
ThreadDescriptor::~ThreadDescriptor ()
{
    TRACE ("ThreadDescriptor");
}

