#include "TaskBase.h"
#include "Guard_T.h"
#include "Trace.h"

TaskBase::TaskBase (ThreadManager *thr_mgr/* = 0*/)
: thr_count_ (0)
, grp_id_ (-1)
, thr_mgr_ (thr_mgr)
{

}
TaskBase::~TaskBase ()
{

}
int TaskBase::open (void * /* = 0*/)
{
    return -1;
}
int TaskBase::close ()
{
    return -1;
}
int TaskBase::activate (int flags/* = THR_JOIN*/,
	size_t n_threads/* = 1*/,
	thread_t thread_ids[]/* = 0*/,
	int append_thr/* = 0*/,
	size_t stack_size/* = 0*/,
	int grp_id/* = -1*/)
{
    Guard_T<LOCK> g (this->lock_); // avoid mutithread reentry
    if (this->thr_count_ > 0 && append_thr == 0)
	return 1;
    else
    {
	if (this->thr_count_ > 0 && this->grp_id_ != -1)
	    grp_id = this->grp_id_;
    }
    //
    if (this->thr_mgr_ == 0)
	this->thr_mgr_ = ThreadManager::instance ();

    int grp_spawned = -1;
    grp_spawned = this->thr_mgr_->spawn_n (n_threads,
	    &TaskBase::svc_run,
	    (void*)this,
	    thread_ids,
	    flags,
	    grp_id,
	    stack_size,
	    this);
    if (grp_spawned == -1)
	return -1;
    this->thr_count_ += n_threads;
    return 0;
}
THREAD_FUNC_RETURN_T TaskBase::svc_run (void *args)
{
    TRACE ("TaskBase");
    if (args == 0) return 0;
    TaskBase* t = (TaskBase *)args;
    ThreadManager *thr_mgr_ptr = t->thr_mgr ();
    if (thr_mgr_ptr)
    {
	thr_mgr_ptr->set_thr_state (Thread::self (), ThreadManager::THR_RUNNING);
    }
    int svc_status = t->svc ();
    THREAD_FUNC_RETURN_T exit_status;
    exit_status = reinterpret_cast<THREAD_FUNC_RETURN_T> (svc_status);

    t->cleanup ();
#if 0
    if (thr_mgr_ptr)
    {
	thr_mgr_ptr->remove_thr(Thread::self (), t->grp_id_);
    }
#endif
    return exit_status;   // maybe used for pthread_join
}
int TaskBase::wait ()
{
    if (this->thr_mgr () != 0)
	return this->thr_mgr ()->wait_task (this);
    else
	return -1;
}
ThreadManager *TaskBase::thr_mgr () const
{
    return this->thr_mgr_;
}
int TaskBase::get_grp_id () const
{
    return this->grp_id_;
}
void TaskBase::cleanup ()
{
    Guard_T<LOCK> g (this->lock_);
    this->thr_count_--;
    this->close ();
}

