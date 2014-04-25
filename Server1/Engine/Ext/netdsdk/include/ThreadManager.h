//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-04 18:14
 */
//========================================================================

#ifndef _THREADMANAGER_H_
#define _THREADMANAGER_H_
#include "Pre.h"

#include "Trace.h"
#include "Common.h"
#include "Thread.h"
#include "Guard_T.h"
#include "AtomicOpt_T.h"
#include "ThreadMutex.h"

#include <map>
/**
 * the interface push_back of the std::deque if more fast than std::list
 */
#include <deque>   

/**
 * @class ThreadManager
 *
 * @brief 
 */
class TaskBase;
class ThreadDescriptor;
class ThreadManager
{
public:
    // thread state 
    enum
    {
	// uninitialized
	THR_IDLE = 0x00000000,

	// created but not yet running.
	THR_SPAWNED = 0x00000001,

	// thread is active (naturally, we don't know if it's actually
	// *running* because we aren't the scheduler...).
	THR_RUNNING = 0x00000002,

	// thread is suspended.
	THR_SUSPENDED = 0x00000004,

	// thread has been cancelled (which is an indiction that it needs 
	// to terminate...).
	THR_CANCELLED = 0x00000008,

	// thread has shutdown, but the slot in the thread manager hasn't
	// been reclaimed yet.
	THR_TERMINATED = 0x00000010,
	
	// join operation has been invoked on the thread by thread manager.
	THR_JOINING = 0x10000000
    };
    //
public:
    ~ThreadManager ();

    // get pointer to a process-wide thread-manager
    static ThreadManager* instance (void);

    /** 
     * if <grp_id> is assigned, the newly spawned threads are 
     * added into the group. otherwise, the Thread_Manager assigns
     * an new unique group id
     * return : a unique group id that can be used to control
     * other threads added to the same group if success, -1 if failed
     */
    int spawn (THREAD_FUNC_T func,
	   void *arg  = 0,
	   thread_t *thr_id = 0,
	   int flags = Thread::THR_JOIN,
	   int grp_id = -1,
	   size_t stack_size = 0,
	   TaskBase *task = 0);

    /**
     * spawn thr_num new threads, which execute <func> with 
     * argument <arg> if <grp_id> is assigned, the newly spawn
     * ed threads are added into the group. otherwise, the 
     * Thread_Manager assigns an new unique group id
     * return : >0, a unique group id that can be used to control 
     *          -1, one of the new thread create failed
     */
    int spawn_n (size_t thr_num,
	    THREAD_FUNC_T func,
	    void *arg  = 0,
	    thread_t thr_id[] = 0,    //
	    int flags = Thread::THR_JOIN,
	    int grp_id = -1,
	    size_t stack_size = 0,   // shared
	    TaskBase *task = 0);

    /**
     * return a count of the current number of threads active in 
     * the ThreadManager
     */
    size_t thr_count (void);

    // get the group-id related with threadid
    int get_grp_id (thread_t thr_id);

    /**
     * join all thread that in the threadmanager 
     */
    int wait (void);

    // wait one task
    int wait_task (TaskBase *);

    // Set the state of the thread. Returns -1 if the thread is not
    // managed by this thread manager
    int set_thr_state (thread_t thr_id, size_t thr_state);

    // Get the state of the thread. Returns -1 if the thread is not
    // managed by this thread manager
    int get_thr_state (thread_t thr_id, size_t &thr_state);

    // ++ Cancel methods, which provides a cooperative thread-termination 
    // mechanism (will not block)..
    // Cancel a single thread.
    int cancel (thread_t thr_id, int async_cancel = 0);

    // Cancel a group of threads. 
    int cancel_grp (int grp_id, int async_cancel = 0);

    // True if <t_id> is cancelled, else false. Always return false if
    // <t_id> is not managed by the Thread_Manager.
    int testcancel (thread_t thr_id, int async_cancel = 0);

protected:
    // singleton
    ThreadManager ();

    // append a thread in the table
    int append_thr (thread_t thr_id, 
	    int thr_state, 
	    int grp_id,
	    int flags,
	    TaskBase *task);
    //
    int spawn_i (THREAD_FUNC_T func, 
	    void *arg, 
	    thread_t *thr_id, 
	    int flags, 
	    int grp_id, 
	    size_t stack_size, 
	    TaskBase *task);

    // Not bind lock
    ThreadDescriptor *find_thread_i (thread_t thr_id);
    ThreadDescriptor *find_thread_i (thread_t thr_id, int grp_id);

    //
    void remove_thr_i (thread_t thr_id);
    void remove_thr_i (thread_t thr_id, int grp_id);
private:
    typedef ThreadMutex    MUTEX;
    // keep a list of thread descriptors within thread manager
    typedef std::deque<ThreadDescriptor*> thr_list_t;
    typedef thr_list_t::iterator         thr_list_itor;
    typedef std::map<int/*grp_id*/, thr_list_t> thr_table_t;
    typedef thr_table_t::iterator        thr_table_itor;

    // keep track of the next group id to assign
    AtomicOpt_T<int>	   grp_id_;
    // guard for construct ThreadManager instance
    static  ThreadMutex    inst_mutex_;
    static  ThreadManager* thr_mgr_;

    thr_table_t		   thr_table_;
    MUTEX		   thr_table_mutex_;
    //thr_list_t	   thr_list_;
    //MUTEX                thr_list_mutex_;
protected:
    //
    void update_thr_term_list_copy (thr_list_t &term_thr_list_copy, int &thr_count);
};
// 
class ThreadDescriptor
{
    friend class ThreadManager;
public:
    ThreadDescriptor ();
    ~ThreadDescriptor ();

    // 
    ThreadManager* thr_mgr (void);

    //
    void thr_mgr (ThreadManager *thr_mgr);
    //
protected:
    // 
    int		      grp_id_;
    int		      thr_state_;
    int		      thr_flags_;
    thread_t	      thr_id_;
    TaskBase*	      task_;
    ThreadManager*    thr_mgr_;
};

#include "ThreadManager.inl"
#include "Post.h"
#endif

