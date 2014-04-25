//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-04-09 22:29
 */
//========================================================================

#ifndef _TOKEN_H_
#define _TOKEN_H_
#include "Pre.h"

#include "Trace.h"
#include "Guard_T.h"
#include "GlobalMacros.h"
#include "ConditionThreadMutex.h"

/**
 * @class Token
 *
 * @brief 
 */
class Token
{
public:
    enum
    {
	FIFO    = 1,
	LIFO	= 2
    };
    Token (int q_strategy = FIFO);
    virtual ~Token ();

    // == Guard method
    /**
     * Acquire the lock, sleeping until it is obtained or until the
     * expiration of >timeout>, If some other thread currently holds 
     * the token then <leep_hook> is called before our thread goes 
     * to sleep. This <sleep_hook> can be used by the requesting 
     * thread to unblock a token-holder that is sleeping, e.g., by 
     * means of writing to a pipe (the Reactor uses this functionality).
     * Return values: 0 hold -1 if failure or timeout occurs
     */
    int acquire (void (*sleep_hook_func)(void *),
	    void *arg = 0,
	    const TimeValue *timeout = 0);

    /**
     * This behaves just like the previous <acquire> method, except that
     *  it invokes the virtual function called <sleep_hook> that can be
     *  overridden by a subclass of Token.
     */
    int acquire (const TimeValue *timeout = 0);

    // Behaves like acquire() but at a lower priority.
    int acquire_read (void (*sleep_hook)(void *),
	    void *arg = 0,
	    const TimeValue *timeout = 0);
    /**
     * This should be overridden by a subclass to define the appropriate
     * behavior before <acquire> goes to sleep.  By default, this is a
     * no-op...
     */
    virtual void sleep_hook (void);

    // Implements a non-blocking <acquire>
    int tryacquire (void);

    // Relinquish the lock.  If there are any waiters then the next one
    // in line gets it.
    int release (void);

    // Return the number of threads that are currently waiting to get
    // the token
    int waiters (void);

    // Return the id of the current thread that owns the token.
    thread_t current_owner (void);
protected:
    // The following structure implements a LIFO/FIFO queue of waiter threads
    // that are asleep waiting to obtain the token.
    class TokenQueueEntry
    {
    public:
	TokenQueueEntry (ThreadMutex &lock,
		thread_t thr_id);
	
	// Entry blocks on the token.
	int wait (const TimeValue *timeout/*msec*/);

	// Notify (unblock) the entry.
	int signal (void);

	// Ok to run.
	int runable_;

	// Pointer to next waiter.	
	TokenQueueEntry *next_;

	// Thread id of this waiter.
	thread_t thread_id_;

	// Condition object used to wake up waiter when it can run again.
	ConditionThreadMutex cond_;
    };
    enum TokenOptType
    {
	READ_TOKEN    = 10,
	WRITE_TOKEN
    };
    class TokenQueue
    {
    public:
	TokenQueue ();

	// Remove a waiter from the queue.
	void remove_entry (TokenQueueEntry *);

	// Insert a waiter into the queue.
	void insert_entry (TokenQueueEntry &entry,
		int queue_strategy = FIFO);

	// Head of the list of waiting threads.
	TokenQueueEntry *head_;

	// Tail of the list of waiting threads.
	TokenQueueEntry *tail_;
    };
private:
    // Implements the <acquire> and <tryacquire> methods above.
    int shared_acquire (void (*sleep_hook_func)(void *),
	    void *arg,
	    const TimeValue *timeout,
	    TokenOptType opt_type);

    // Wake next in line for ownership.
    void wakeup_next_waiter (void);
private:
    // Number of waiters
    int waiters_;

    // Some thread (i.e., <owner_>) is useing the lock, we need this
    // extra variable to deal with POIX pthreads madness
    int in_use_; 

    // Cureent nest level
    int nesting_level_;

    // Queueing strategy, LIFO/FIFO.
    int queueing_strategy_;

    // Current owner of the lock
    thread_t owner_;

    // guard
    ThreadMutex lock_;

    // A queue of writer threads.
    TokenQueue writers_;

    // A queue of reader threads.
    TokenQueue readers_;
};

#include "Token.inl"
#include "Post.h"
#endif

