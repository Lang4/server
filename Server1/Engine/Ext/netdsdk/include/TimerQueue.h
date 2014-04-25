//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-04-20 22:50
 */
//========================================================================

#ifndef _TIMERQUEUE_H_
#define _TIMERQUEUE_H_
#include "Pre.h"

#include <deque>

#include "TimeValue.h"
#include "EventHandler.h"

/**
 * @class TimerNode
 *
 * @brief 
 */
class TimerNode
{
public:
    TimerNode ();

    // 
    ~TimerNode ();

    // 
    void set (int timer_id,
	    const EventHandler *handler,
	    const void *arg,
	    const TimeValue &t,
	    const TimeValue &i,
	    TimerNode *p = 0,
	    TimerNode *n = 0);

    TimerNode & operator = (const TimerNode &tn);

    // Get the timer handler.
    EventHandler *handler ();

    // Set the timer handler
    void handler (EventHandler *h);

    // Get the handler arg.
    const void *arg (void);

    // Set the handler arg
    void arg (void *arg);

    // Get the timer value.
    const TimeValue &timer_value (void) const;

    // Set the timer value.
    void timer_value (const TimeValue &timer_value);

    // Get the timer interval.
    const TimeValue &interval (void) const;

    // Set the timer interval.
    void interval (const TimeValue &interval);

    // Get the previous pointer.
    TimerNode *prev (void);

    // Set the previous pointer.
    void prev (TimerNode *prev);

    // Get the next pointer.
    TimerNode *next (void);

    // Set the next pointer.
    void next (TimerNode *next);

    // Get the timer_id.
    int timer_id (void) const;

    // Set the timer_id.
    void timer_id (int timer_id);
private:
    // Id of this timer (used to cancel timers before they expire).
    int timer_id_;
    
    // Timer handler object
    EventHandler *handler_;

    // If timer expires then <arg> is passed in as the parameter of 
    // the handler
    const void *arg_;

    // Time until the timer expires.
    TimeValue timer_value_;

    // If this is a periodic timer this holds the time until the next
    // timeout.
    TimeValue interval_;

    // Pointer to previous timer.
    TimerNode *prev_;

    // Pointer to next timer.
    TimerNode *next_;
};
/**
 * @class TimerQueue
 *
 * @brief 
 */
class TimerQueue
{
public:
    // Default constructor
    TimerQueue (); 

    // 
    TimerQueue (size_t max_size, bool preallocated = true);

    // 
    virtual ~TimerQueue ();

    // Initialize members.
    void init (bool preallocated);

    // True if queue is empty, else false.
    virtual int is_empty (void) const;

    // Returns the time of the earlier node in the Timer_Queue.
    virtual const TimeValue &earliest_time (void) const;

    /**
     * Schedule that will expire at <delay_time>. If it expires 
     * then <act> is passed in as the value to the <callback>.
     * Returns -1 on failure (which is guaranteed never to 
     * be a valid <timer_id>), else return a valid timer_id
     */
    virtual int schedule (const EventHandler *eh,
	    const void *arg,
	    const TimeValue &delay_time,	
	    const TimeValue &interval_time);

    /**
     * Resets the interval of the timer represented by <timer_id> to
     * <interval>. If <interval> is equal to <0> the timer will 
     * become a non-rescheduling timer.  Returns 0 if successful, 
     * -1 if not.
     */
    virtual int reset_interval (int timer_id,
	    const TimeValue &interval_time);

    /**
     * Cancel all timer. If <dont_call_handle_close> is 0 then the 
     * <callback> will be invoked, which typically invokes the 
     * <handle_close> hook.  Returns number of timers cancelled.
     *
     * Note: The method will canll EventHandler::handle_close() when
     * <dont_call_handle_close> is zero, but the first argument which 
     * points to the <arg> is zero, because this <eh> maybe register 
     * multiple argument and we don't know which should be passed out.
     */
    virtual int cancel (const EventHandler *eh,
	    int dont_call_handle_close = 1);

    /**
     * Cancel the single timer that matches the <timer_id> value (which
     * was returned from the <schedule> method).  If <arg> is non-NULL
     * then it will be set to point to the <argument> passed in when the 
     * timer was registered. This makes it possible to free up the memory 
     * and avoid memory leaks. If <dont_call> is 0 then the <handle_close> 
     * will be invoked. EventHandler::handle_close () will pass the argument
     * that passed in when the handler was registered, This makes it possible 
     * to release the memory of the <arg> in handle_close.
     *
     * Returns 0 if cancellation succeeded and -1 if the <timer_id> 
     * wasn't found.
     */
    virtual int cancel (int timer_id,
	    const void **arg = 0,
	    int dont_call_handle_close = 1);

    // Determine the next event to timeout. 
    virtual TimeValue* calculate_timeout (TimeValue* max_wait_time);

    // Determine the next event to timeout. 
    virtual TimeValue* calculate_timeout (TimeValue* max_wait_time,
	    TimeValue* the_timeout);

    /** 
     * Run the <functor> for all timers whose values are <= <cur_time>.
     * Returns the number of timers canceled.
     */
    virtual int expire (const TimeValue &current_time);

    // Return nonzero on dispatch an timer
    virtual int dispatch_timer (const TimeValue &current_time,
	    TimerNode &dispatched_node);

    int upcall (EventHandler *event_handler,
	    int (EventHandler::*callback)(const void *act, const TimeValue &),
	    const void *arg,
	    const TimeValue &current_time);
protected:
    // Schedule a timer.
    int schedule_i (const EventHandler *eh,
	    const void *arg,
	    const TimeValue &delay_time,
	    const TimeValue &interval_time);

    // Pops and returns a new timer id from the freelist.
    int pop_freelist ();

    // Pushes <old_id> onto the freelist.
    void push_freelist (int old_id);

    // Reschedule an "interval" ACE_Timer_Node.
    void reschedule (TimerNode *);    

    // Factory method that allocates a new node.
    TimerNode *alloc_node (void);

    // Factory method that frees a previously allocated node.
    void free_node (TimerNode *); 

    // Insert a new_node into the heap and restore the heap property.
    void insert (TimerNode *new_node);

    // Remove and return the a slotth TimerNode and restore the
    // heap property.
    TimerNode *remove (size_t slot);

    /**
     * Doubles the size of the heap and the corresponding timer_ids array.
     * If preallocation is used, will also double the size of the
     * preallocated array of TimerNode.
     */
    void grow_heap (void);

    // Restore the heap property, starting at @a slot.
    void reheap_up (TimerNode *new_node,
	    size_t slot,
	    size_t parent);

    // Restore the heap property, starting at @a slot.
    void reheap_down (TimerNode *moved_node,
	    size_t slot,
	    size_t child);

    // Copy <moved_node> into the @a slot slot of <heap_> and move
    // a slot into the corresponding slot in the <timer_id_> array.
    void copy (size_t slot, TimerNode *moved_node);

    /**
     * Removes the earliest node from the queue and returns it. Note that
     * the timer is removed from the heap, but is not freed, and its ID
     * is not reclaimed. The caller is responsible for calling either
     * reschedule() or free_node() after this function returns. Thus,
     * this function is for support of TimerQueue::expire and should 
     * not be used unadvisedly in other conditions.
     */
    TimerNode *remove_first (void);

    // Returns the current time of day.
    TimeValue gettimeofday (void);
private:
    // Maximum size of the heap 
    size_t max_size_;

    // Current size of the heap 
    size_t cur_size_;
    
    // Number of heap entries in transition (removed from the queue, but
    // not freed) and may be rescheduled or freed.
    size_t cur_limbo_;

    // "Pointer" to the element in the <timer_ids_> array that was
    // last given out as a timer ID.
    size_t timer_ids_curr_;

    // Index representing the lowest timer ID that has been freed. When
    // the timer_ids_next_ value wraps around, it starts back at this
    // point.
    size_t timer_ids_min_free_;
    
    // Current contents of the Heap, which is organized as a "heap" of
    // TimerNode *'s.  In this context, a heap is a "partially
    // ordered, almost complete" binary tree, which is stored in an
    // array.
    TimerNode **heap_;
    
    /**
     * An array of "pointers" that allows each TimerNode in the 
     * <heap_> to be located in O(1) time.  Basically, <timer_id_[i]>
     * contains the slot in the <heap_> array where an TimerNode *
     * with timer id \<i\> resides.  Thus, the timer id passed back from
     * <schedule> is really a slot into the <timer_ids> array.  The
     * <timer_ids_> array serves two purposes: negative values are
     * indications of free timer IDs, whereas positive values are
     * "pointers" into the <heap_> array for assigned timer IDs.
     */
    int *timer_ids_;

    /**
     * If this is non-0, then we preallocate <max_size_> number of
     * TimerNode objects in order to reduce dynamic allocation
     * costs.  In auto-growing implementation, this points to the
     * last array of nodes allocated.
     */
    TimerNode *preallocated_nodes_;

    // This points to the head of the <preallocated_nodes_> freelist,
    // which is organized as a stack.
    TimerNode *preallocated_nodes_freelist_;

    // Set of pointers to the arrays of preallocated timer nodes.
    // Used to delete the allocated memory when required.
    std::deque<TimerNode *> preallocated_node_set_;
    
    // Returned by <calculate_timeout>.
    TimeValue timeout_;
private:
    TimerQueue (const TimerQueue &);
    void operator = (const TimerQueue &); 
};

#include "TimerQueue.inl"
#include "Post.h"
#endif

