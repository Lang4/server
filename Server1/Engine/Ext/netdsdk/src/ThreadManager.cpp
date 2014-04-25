#include "ThreadManager.h"
#include "TaskBase.h"
#include "Debug.h"

#include <memory>

// static members inititalization
ThreadMutex    ThreadManager::inst_mutex_;
ThreadManager *ThreadManager::thr_mgr_ = 0; 

int ThreadManager::spawn_i (THREAD_FUNC_T func,
	void *arg,
	thread_t *thr_id,
	int flags,
	int grp_id,
	size_t stack_size,
	TaskBase *task)
{
    TRACE ("ThreadManager");
    thread_t id;
    if (thr_id == 0)
	thr_id = &id;
    if (Thread::spawn (func, arg, thr_id, flags, stack_size) == -1)
	return -1;
    return this->append_thr (*thr_id, ThreadManager::THR_SPAWNED, grp_id, flags, task);
}
int ThreadManager::append_thr (thread_t thr_id,
	int thr_state,
	int grp_id,
	int flags,
	TaskBase *task)
{
    TRACE ("ThreadManager");
    if (thr_id ==  0) return -1;
    if (grp_id == -1) return -1;
    ThreadDescriptor *new_thr_desc = new ThreadDescriptor;
    if (new_thr_desc == 0) return -1;
    new_thr_desc->grp_id_    = grp_id;
    new_thr_desc->thr_state_ = thr_state;
    new_thr_desc->thr_flags_ = flags;
    new_thr_desc->thr_id_    = thr_id;
    new_thr_desc->task_      = task;
    new_thr_desc->thr_mgr_   = this;

    Guard_T<MUTEX> g (this->thr_table_mutex_);
    thr_table_itor t_itor = this->thr_table_.find (grp_id);
    if (t_itor == this->thr_table_.end ())
    {
	thr_list_t thr_list;
	thr_list.push_back (new_thr_desc);	
	this->thr_table_.insert (std::make_pair (grp_id, thr_list));
    }else
    {
	t_itor->second.push_back (new_thr_desc);
    }
    return 0;
}
void ThreadManager::remove_thr_i (thread_t thr_id, int grp_id)
{
    TRACE ("ThreadManager");
    thr_table_itor t_itor = this->thr_table_.find (grp_id);
    if (t_itor == this->thr_table_.end ())
	return ;
    thr_list_itor  l_itor = t_itor->second.begin ();
    for (; l_itor != t_itor->second.end (); ++l_itor)
    {
	if (Thread::thr_equal ((*l_itor)->thr_id_, thr_id))
	{
	    delete (*l_itor); // first
	    t_itor->second.erase (l_itor);
	    if (t_itor->second.empty ())
		this->thr_table_.erase (t_itor);
	    return;
	}
    }
}
void ThreadManager::remove_thr_i (thread_t thr_id)
{
    TRACE ("ThreadManager");
    thr_table_itor t_itor = this->thr_table_.begin ();
    thr_list_itor  l_itor;
    for (; t_itor != this->thr_table_.end (); ++t_itor)
    {
	l_itor = t_itor->second.begin ();
	for (; l_itor != t_itor->second.end (); ++l_itor)
	{
	    if (Thread::thr_equal ((*l_itor)->thr_id_, thr_id))
	    {
		delete (*l_itor); // first
		t_itor->second.erase (l_itor);
		if (t_itor->second.empty ())
		    this->thr_table_.erase (t_itor);
		return;
	    }
	}
    }
}
int ThreadManager::get_grp_id (thread_t thr_id)
{
    TRACE ("ThreadManager");
    Guard_T<MUTEX> g (this->thr_table_mutex_);
    ThreadDescriptor *td = this->find_thread_i (thr_id);
    if (td)
    {
	return td->grp_id_;
    }
    return -1;
}
size_t ThreadManager::thr_count ()
{
    TRACE ("ThreadManager");
    Guard_T<MUTEX> g (this->thr_table_mutex_);
    int thr_count = 0;
    thr_table_itor t_itor = this->thr_table_.begin ();
    for (; t_itor != this->thr_table_.end (); ++t_itor)
    {
	thr_count += t_itor->second.size ();
    }
    return thr_count;
}
int ThreadManager::wait ()
{
    TRACE ("ThreadManager");
    thr_list_t term_thr_list_copy;
    int thr_count = 0;
    int term_thr_list_copy_size = 0;
    int diff_thr_term_and_thr_table = 0;
    this->update_thr_term_list_copy (term_thr_list_copy, thr_count);
    NDK_INF ("+++++++++++++++ thr_coutn = %d size = %d\n", thr_count, term_thr_list_copy.size ());
    while (!term_thr_list_copy.empty ())
    {
	term_thr_list_copy_size = term_thr_list_copy.size ();
	diff_thr_term_and_thr_table = thr_count - term_thr_list_copy_size;
	//start to join threads
	thr_list_itor  l_itor = term_thr_list_copy.begin ();
	for (; l_itor != term_thr_list_copy.end ();)
	{
	    if (NDK_BIT_DISABLED ((*l_itor)->thr_flags_, Thread::THR_DETACH)
		    && NDK_BIT_ENABLED ((*l_itor)->thr_flags_, Thread::THR_JOIN))
	    {
		Thread::join ((*l_itor)->thr_id_);
		NDK_DBG ("pthread join");
		int new_diff = this->thr_count () - term_thr_list_copy_size ;
		NDK_INF ("---------------- join %lu diff = %d\n", (*l_itor)->thr_id_, new_diff);
		this->remove_thr_i ((*l_itor)->thr_id_, (*l_itor)->grp_id_);
		if (new_diff > diff_thr_term_and_thr_table)
		{
		    //thread_t thr_id = (*l_itor)->thr_id_;
		    //int grp_id      = (*l_itor)->grp_id_;
		    term_thr_list_copy.clear ();
		    this->update_thr_term_list_copy (term_thr_list_copy, thr_count);
		    break; // go to update term_thr_list_copy_size
		}
	    }
	    l_itor = term_thr_list_copy.erase (l_itor);
	    --term_thr_list_copy_size;
	}
    }
    return 0; 
}
void ThreadManager::update_thr_term_list_copy (thr_list_t &term_thr_list_copy, int &thr_count)
{
    TRACE ("ThreadManager");
    Guard_T<MUTEX> g (this->thr_table_mutex_);
    thr_table_itor t_itor = this->thr_table_.begin ();
    thr_list_itor  l_itor;
    thr_count = 0;
    for (; t_itor != this->thr_table_.end ();)
    {
	l_itor = t_itor->second.begin ();
	for (; l_itor != t_itor->second.end (); ++l_itor)
	{
	    thr_count++;
	    if (NDK_BIT_ENABLED ((*l_itor)->thr_flags_, Thread::THR_DETACH)
		    && NDK_BIT_DISABLED ((*l_itor)->thr_flags_, Thread::THR_JOIN))
	    {
		// remove it
		delete (*l_itor);
		l_itor = t_itor->second.erase (l_itor);
	    }else
		term_thr_list_copy.push_back (*l_itor);
	}
	// check it empty or not
	if (t_itor->second.empty ())
	    this->thr_table_.erase (t_itor++);
	else
	    ++t_itor;
    }
} // release lock
int ThreadManager::wait_task (TaskBase *task)
{
    TRACE ("ThreadManager");
    if (task == 0) return -1;
    int grp_id = task->get_grp_id ();
    if (grp_id == -1) return -1;

    // lock scope
    this->thr_table_mutex_.acquire ();
    thr_table_itor t_itor = this->thr_table_.find (grp_id);
    if (t_itor == this->thr_table_.end ())
    {
	this->thr_table_mutex_.release ();
	return -1;
    }
    thr_list_t thr_list (t_itor->second);
    this->thr_table_mutex_.release ();
    // unlock

    thr_list_itor l_itor = thr_list.begin ();
    for (; l_itor != thr_list.end (); ++l_itor)
    {
	NDK_SET_BITS ((*l_itor)->thr_state_, ThreadManager::THR_JOINING);
	if (NDK_BIT_ENABLED ((*l_itor)->thr_flags_, Thread::THR_JOIN))
	    Thread::join ((*l_itor)->thr_id_);
	NDK_SET_BITS ((*l_itor)->thr_state_, ThreadManager::THR_IDLE);
	// to ensure realtime of thr_count
	this->remove_thr_i ((*l_itor)->thr_id_, grp_id);  
    }
    return 0; 
}
ThreadDescriptor* ThreadManager::find_thread_i (thread_t thr_id)
{
    TRACE ("ThreadManager");
    thr_table_itor t_itor = this->thr_table_.begin ();
    thr_list_itor l_itor;
    for (; t_itor != this->thr_table_.end (); ++t_itor)
    {
	l_itor = t_itor->second.begin ();
	for (; l_itor != t_itor->second.end (); ++l_itor)
	{
	    if (Thread::thr_equal ((*l_itor)->thr_id_, thr_id))
	    {
		return (*l_itor);
	    }
	}
    }
    return 0;
}
ThreadDescriptor* ThreadManager::find_thread_i (thread_t thr_id, int grp_id)
{
    TRACE ("ThreadManager");
    thr_table_itor t_itor = this->thr_table_.find (grp_id);
    if (t_itor != this->thr_table_.end ())
    {
	thr_list_itor l_itor;
	l_itor = t_itor->second.begin ();
	for (; l_itor != t_itor->second.end (); ++l_itor)
	{
	    if (Thread::thr_equal ((*l_itor)->thr_id_, thr_id))
	    {
		return (*l_itor);
	    }
	}
    }
    return 0;
}
int ThreadManager::set_thr_state (thread_t thr_id, size_t thr_state)
{
    TRACE ("ThreadManager");
    Guard_T<MUTEX> g (this->thr_table_mutex_);
    ThreadDescriptor *td = this->find_thread_i (thr_id);
    if (td)
    {
	NDK_SET_BITS (td->thr_state_, thr_state);
	return 0;
    }
    return -1;
}
int ThreadManager::get_thr_state (thread_t thr_id, size_t &thr_state)
{
    TRACE ("ThreadManager");
    Guard_T<MUTEX> g (this->thr_table_mutex_);
    ThreadDescriptor *td = this->find_thread_i (thr_id);
    if (td)
    {
	thr_state = td->thr_state_;
	return 0;
    }
    return -1;
}
int ThreadManager::cancel (thread_t thr_id, int async_cancel/* = 0*/)
{
    Guard_T<MUTEX> g (this->thr_table_mutex_);
    ThreadDescriptor *td = this->find_thread_i (thr_id);
    if (td == 0) return -1; // not be managed by ThreadManager
    NDK_SET_BITS (td->thr_state_, ThreadManager::THR_CANCELLED);
    if (async_cancel)
	return Thread::cancel (thr_id);
    return 0;
}
int ThreadManager::cancel_grp (int grp_id, int async_cancel/* = 0*/)
{
    Guard_T<MUTEX> g (this->thr_table_mutex_);
    thr_table_itor t_itor = this->thr_table_.find (grp_id);
    if (t_itor != this->thr_table_.end ())
    {
	thr_list_itor l_itor;
	l_itor = t_itor->second.begin ();
	for (; l_itor != t_itor->second.end (); ++l_itor)
	{
	    NDK_BIT_ENABLED ((*l_itor)->thr_state_, ThreadManager::THR_CANCELLED);
	    if (async_cancel)
		Thread::cancel ((*l_itor)->thr_id_);
	}
	return 0;
    }
    return -1;
}
int ThreadManager::testcancel (thread_t thr_id, int async_cancel/* = 0*/)
{
    Guard_T<MUTEX> g (this->thr_table_mutex_);
    ThreadDescriptor *td = this->find_thread_i (thr_id);
    if (td == 0) return -1;
    if (async_cancel)
	Thread::testcancel ();
    return NDK_BIT_ENABLED (td->thr_state_, ThreadManager::THR_CANCELLED) ? 0 : -1;
}

