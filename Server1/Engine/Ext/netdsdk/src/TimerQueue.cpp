#include "TimerQueue.h"
#include "config.h"
#include "Debug.h"
#include "Trace.h"
#include "NDK.h"

#include <limits>

#define NDK_HEAP_PARENT(X) (X == 0 ? 0 : (((X) - 1) / 2))
#define NDK_HEAP_LCHILD(X) (((X)+(X))+1)

TimerQueue::TimerQueue (size_t max_size, bool preallocated/* = true*/)
: max_size_ (max_size)
, cur_size_ (0)
, cur_limbo_ (0)
, timer_ids_curr_ (0)
, timer_ids_min_free_ (0)
, preallocated_nodes_ (0)
, preallocated_nodes_freelist_ (0)
{
    if (max_size > static_cast<size_t>(std::numeric_limits<int>::max ()))
    {
	max_size = static_cast<size_t>(std::numeric_limits<int>::max ());
	max_size_ = max_size;
    }
    init (preallocated);
}
TimerQueue::TimerQueue ()
: max_size_ (NDK_DEFAULT_TIMERS)
, cur_size_ (0)
, cur_limbo_ (0)
, timer_ids_curr_ (0)
, timer_ids_min_free_ (0)
, preallocated_nodes_ (0)
, preallocated_nodes_freelist_ (0)
{
    init (true);
}
TimerQueue::~TimerQueue ()
{
    size_t current_size = this->cur_size_;

    // Clean up all the nodes still in the queue
    for (size_t i = 0; i < current_size; ++i)
	this->free_node (this->heap_[i]);

    delete [] this->heap_;
    delete [] this->timer_ids_;

    // clean up any preallocated timer nodes
    if (this->preallocated_nodes_ != 0)
    {
	std::deque<TimerNode *>::iterator itor;
       	for (itor = this->preallocated_node_set_.begin ();
		itor != this->preallocated_node_set_.end ();
		++itor)
	{
	    delete [] *itor;
	}
    }
}
void TimerQueue::init (bool preallocated)
{
    TRACE ("TimerQueue");
    // Create the heap array.
    this->heap_  =  new TimerNode *[this->max_size_];

    // Create the parallel
    this->timer_ids_ = new int[this->max_size_];

    // Initialize the "freelist," which uses negative values to
    // distinguish freelist elements from "pointers" into the <heap_>
    // array.
    for (size_t i = 0; i < this->max_size_; ++i)
	this->timer_ids_[i] = -1;

    if (preallocated)
    {
	this->preallocated_nodes_ = new TimerNode[this->max_size_];

	// Add allocated array to set of such arrays for deletion on
	// cleanup.
	this->preallocated_node_set_.push_back (this->preallocated_nodes_);

	// Form the freelist by linking the next_ pointers together.
	for (size_t j = 1; j < this->max_size_; ++j)
	    this->preallocated_nodes_[j - 1].next (&this->preallocated_nodes_[j]);
	// NULL-terminate the freelist.
	this->preallocated_nodes_[this->max_size_ - 1].next (0);

	// Assign the freelist pointer to the front of the list.
	this->preallocated_nodes_freelist_ = &this->preallocated_nodes_[0];
    }
}
int TimerQueue::pop_freelist ()
{
    TRACE ("TimerQueue");
    // Scan for a free timer ID. Note that since this function is called
    // _after_ the check for a full timer heap, we are guaranteed to find
    // a free ID, even if we need to wrap around and start reusing freed IDs.
    // On entry, the curr_ index is at the previous ID given out; start
    // up where we left off last time.
    // NOTE - a timer_ids_ slot with -2 is out of the heap, but not freed.
    // It must be either freed (free_node) or rescheduled (reschedule).
    while (this->timer_ids_curr_ < this->max_size_ &&
	    (this->timer_ids_[this->timer_ids_curr_] >= 0 ||
	     this->timer_ids_[this->timer_ids_curr_] == -2  ))
	++this->timer_ids_curr_;
    if (this->timer_ids_curr_ == this->max_size_)
    {
	NDK_ASSERT (this->timer_ids_min_free_ < this->max_size_);
	this->timer_ids_curr_ = this->timer_ids_min_free_;
	// We restarted the free search at min. Since min won't be
	// free anymore, and curr_ will just keep marching up the list
	// on each successive need for an ID, reset min_free_ to the
	// size of the list until an ID is freed that curr_ has already
	// gone past (see push_freelist).
	this->timer_ids_min_free_ = this->max_size_;
    }
    return this->timer_ids_curr_++;
}
void TimerQueue::push_freelist (int old_id)
{
    TRACE ("TimerQueue");
    NDK_ASSERT (this->timer_ids_[old_id] >= 0 || this->timer_ids_[old_id] == -2);
    if (this->timer_ids_[old_id] == -2)
	--this->cur_limbo_;
    else
	--this->cur_size_;
    this->timer_ids_[old_id] = -1;
    if ((static_cast<size_t>(old_id) < this->timer_ids_min_free_) 
	    && (static_cast<size_t>(old_id) <= this->timer_ids_curr_))
	this->timer_ids_min_free_ = old_id;
    return ;
}
TimeValue* TimerQueue::calculate_timeout (TimeValue* max_wait_time)
{
    TRACE ("TimerQueue");
    
    if (this->is_empty ())
	return max_wait_time;
    else
    {
	const TimeValue cur_time = NDK::gettimeofday ();
	if (this->earliest_time () > cur_time)
	{
	    // The earliest item on the TimerQueue is still in the
	    // future.  Therefore, use the smaller of (1) caller's wait
	    // time or (2) the delta time between now and the earliest
	    // time on the TimerQueue.
	    this->timeout_ = this->earliest_time () - cur_time;
	    if (max_wait_time == 0 || *max_wait_time > this->timeout_)
		return &this->timeout_;
	    else
		return max_wait_time;
	}else
	{
	    // The earliest item on the TimerQueue is now in the past.
	    // Therefore, we've got to "poll" the Reactor, i.e., it must
	    // just check the descriptors and then dispatch timers, etc.
	    this->timeout_ = TimeValue::zero;
	    return &this->timeout_;
	}
    }
}
TimeValue* TimerQueue::calculate_timeout (TimeValue* max_wait_time,
	TimeValue* the_timeout)
{
    TRACE ("TimerQueue");
    if (the_timeout == 0) return 0; 

    if (this->is_empty ())
    {
	if (max_wait_time)
	    *the_timeout = *max_wait_time;
	else
	    return 0;
    }
    else
    {
	const TimeValue cur_time = NDK::gettimeofday ();
	if (this->earliest_time () > cur_time)
	{
	    // The earliest item on the TimerQueue is still in the
	    // future.  Therefore, use the smaller of (1) caller's wait
	    // time or (2) the delta time between now and the earliest
	    // time on the TimerQueue.
	    *the_timeout = this->earliest_time () - cur_time;
	    if (!(max_wait_time == 0 || *max_wait_time > *the_timeout))
		*the_timeout = *max_wait_time;
	}else
	{
	    // The earliest item on the TimerQueue is now in the past.
	    // Therefore, we've got to "poll" the Reactor, i.e., it must
	    // just check the descriptors and then dispatch timers, etc.
	    *the_timeout = TimeValue::zero;
	}
    }
    return the_timeout;
}
const TimeValue &TimerQueue::earliest_time () const
{
    TRACE ("TimerQueue");
    return this->heap_[0]->timer_value ();
}
int TimerQueue::schedule (const EventHandler *eh, 
	const void  *arg,
	const TimeValue &delay_time,
	const TimeValue &interval_time)
{
    TRACE ("TimerQueue");
    return this->schedule_i (eh,
	    arg, 
	    NDK::gettimeofday () + delay_time,
	    interval_time);
}
int TimerQueue::schedule_i (const EventHandler *eh, 
	const void *arg,
	const TimeValue &delay_time,
	const TimeValue &interval_time)
{
    TRACE ("TimerQueue");
    if ((this->cur_size_ + this->cur_limbo_) < this->max_size_)
    {
	// Obtain the memory to the new node.
	TimerNode *temp = 0;
	temp = this->alloc_node ();
	if (temp == 0) return -1;

	// Obtain the next unique sequence number.
	int timer_id = this->pop_freelist ();

	temp->set (timer_id,
		eh,
		arg, 
		delay_time,
		interval_time);
	this->insert (temp);
	return timer_id;
    }
    return -1;
}
void TimerQueue::insert (TimerNode *new_node)
{
    TRACE ("TimerQueue");
    if (this->cur_size_ + this->cur_limbo_ + 2 >= this->max_size_)
	this->grow_heap ();

    this->reheap_up (new_node, 
	    this->cur_size_,
	    NDK_HEAP_PARENT (this->cur_size_));
    this->cur_size_++;
}
void TimerQueue::grow_heap ()
{
    TRACE ("TimerQueue");
    // All the containers will double in size from max_size_.
    size_t new_size = this->max_size_ * 2;
    // First grow the heap itself.
    TimerNode **new_heap = 0;
    new_heap = new TimerNode*[new_size];
    ::memcpy (new_heap, 
	    this->heap_, 
	    this->max_size_ * sizeof(*new_heap));
    delete [] this->heap_;
    this->heap_ = new_heap;
    
    // Grow the array of timer ids.
    int *new_timer_ids = 0;
    new_timer_ids = new int[new_size];
    ::memcpy (new_timer_ids, 
	    this->timer_ids_, 
	    this->max_size_ * sizeof (int));
    delete [] timer_ids_;
    this->timer_ids_ = new_timer_ids;

    // And add the new elements to the end of the "freelist".
    for (size_t i = this->max_size_; i < new_size; ++i)
	this->timer_ids_[i] = -(static_cast<int> (i) + 1);

    // Grow the preallocation array (if using preallocation)
    if (this->preallocated_nodes_ != 0)
    {
	// Create a new array with max_size elements to link in to
	// existing list.
	this->preallocated_nodes_ = new TimerNode[this->max_size_];

	// Add allocated array to set of such arrays for deletion on
	// cleanup.
	this->preallocated_node_set_.push_back (this->preallocated_nodes_);

	// Link new nodes together (as for original list).
	for (size_t k = 1; k < this->max_size_; ++k)
	    this->preallocated_nodes_[k - 1].next (&this->preallocated_nodes_[k]);
	// NULL-terminate the new list.
	this->preallocated_nodes_[this->max_size_ - 1].next (0);

	// Link new array to the end of the existling list.
	if (this->preallocated_nodes_freelist_ == 0)
	    this->preallocated_nodes_freelist_ =
		&preallocated_nodes_[0];
	else
	{
	    TimerNode *previous = this->preallocated_nodes_freelist_;
	    for (TimerNode *current = this->preallocated_nodes_freelist_->next ();
		    current != 0;
		    current = current->next ())
		previous = current;
	    previous->next (&this->preallocated_nodes_[0]);
	}
    }
    this->max_size_ = new_size;
    this->timer_ids_min_free_ = this->max_size_;
}
void TimerQueue::reheap_up (TimerNode *moved_node,
	size_t slot,
	size_t parent)
{
    TRACE ("TimerQueue");
    // Restore the heap property after an insertion.
    while (slot > 0)
    {
	// If the parent node is greater than the <moved_node> we need
	// to copy it down.
	if (moved_node->timer_value ()
		< this->heap_[parent]->timer_value ())
	{
	    this->copy (slot, this->heap_[parent]);
	    slot = parent;
	    parent = NDK_HEAP_PARENT (slot);
	}else
	    break;
    }
    // Insert the new node into its proper resting place in the heap and
    // update the corresponding slot in the parallel <timer_ids> array.
    this->copy (slot, moved_node);
}
void TimerQueue::reheap_down (TimerNode *moved_node,
	size_t slot,
	size_t child)
{
    TRACE ("TimerQueue");
    // Restore the heap property after a deletion.
    while (child < this->cur_size_)
    {
	// Choose the smaller of the two children.
	if (child + 1 < this->cur_size_
		&& this->heap_[child + 1]->timer_value ()
		< this->heap_[child]->timer_value ())
	    ++child;
	// Perform a <copy> if the child has a larger timeout value than
	// the <moved_node>.
	if (this->heap_[child]->timer_value ()
		< moved_node->timer_value ())
	{
	    this->copy (slot, this->heap_[child]);
	    slot = child;
	    child = NDK_HEAP_LCHILD (child);
	}else
	    // We've found our location in the heap.
	    break;
    }
    this->copy (slot, moved_node);
}
void TimerQueue::copy (size_t slot,
	TimerNode *moved_node)
{
    TRACE ("TimerQueue");
    // Insert <moved_node> into its new location in the heap.
    this->heap_[slot] = moved_node;
    NDK_ASSERT (moved_node->timer_id () >= 0
	    && moved_node->timer_id () < static_cast<int>(this->max_size_));

    // Update the corresponding slot in the parallel <timer_ids_> array.
    this->timer_ids_[moved_node->timer_id ()] = static_cast<int>(slot);
}
void TimerQueue::reschedule (TimerNode *expired)
{
    if (this->timer_ids_[expired->timer_id ()] == -2)
	--this->cur_limbo_;
    this->insert (expired);
}
TimerNode *TimerQueue::alloc_node ()
{
    TRACE ("TimerQueue");
    TimerNode *temp = 0;
    // Only allocate a node if we are *not* using the preallocated heap.
    if (this->preallocated_nodes_ == 0)
    {
	temp = new TimerNode;
    }else
    {
	// check to see if the heap needs to grow
	if (this->preallocated_nodes_freelist_ == 0)
	    this->grow_heap ();
	temp = this->preallocated_nodes_freelist_;
	// Remove the first element from the freelist.
	this->preallocated_nodes_freelist_ =
	    this->preallocated_nodes_freelist_->next ();
    }
    return temp;
}
void TimerQueue::free_node (TimerNode *node)
{
    TRACE ("TimerQueue");
    // Return this timer id to the freelist.
    this->push_freelist (node->timer_id ());
    // Only free up a node if we are *not* using the preallocated heap.
    if (this->preallocated_nodes_ == 0)
	delete node;
    else
    {
	node->next (this->preallocated_nodes_freelist_);
	this->preallocated_nodes_freelist_ = node;
    }
}
TimerNode *TimerQueue::remove(size_t slot)
{
    TRACE("TimerQueue");

    TimerNode *removed_node = this->heap_[slot];

    // NOTE - the cur_size_ is being decremented since the queue has one
    // less active timer in it. However, this TimerNode is not being 
    // freed, and there is still a place for it in timer_ids_ (the timer ID
    // is not being relinquished). The node can still be rescheduled, or
    // it can be freed via free_node.
    --this->cur_size_;

    // Only try to reheapify if we're not deleting the last entry.
    if (slot < this->cur_size_)
    {
	TimerNode *moved_node = this->heap_[this->cur_size_];
	// Move the end node to the location being removed and update
	// the corresponding slot in the parallel <timer_ids> array.
	this->copy (slot, moved_node);

	// If the <moved_node->time_value_> is great than or equal its
	// parent it needs be moved down the heap.
	size_t parent = NDK_HEAP_PARENT (slot);
	if (moved_node->timer_value ()
		>= this->heap_[parent]->timer_value ())
	    this->reheap_down (moved_node,
		    slot,
		    NDK_HEAP_LCHILD (slot));
	else
	    this->reheap_up (moved_node,
		    slot,
		    parent);
    }
    this->timer_ids_[removed_node->timer_id ()] = -2;
    ++this->cur_limbo_;
    return removed_node;
}
int TimerQueue::cancel (int timer_id,
	const void **arg/* = 0*/,
	int dont_call/* = 1*/)
{
    TRACE("TimerQueue");
    //Guard_T g (this->mutex_);

    if (timer_id < 0 
	    // timer id begin from 0
	    || static_cast<size_t>(timer_id) >= this->max_size_)
	return -1;

    int timer_node_slot = this->timer_ids_[timer_id];
    // Check to see if timer_id is still valid.
    if (timer_node_slot < 0)
	return -1;

    if (timer_id != this->heap_[timer_node_slot]->timer_id ())
    {
	NDK_INF ("exception timerid = %d, slot = %d\n", timer_id, timer_node_slot);
	NDK_ASSERT (timer_id == this->heap_[timer_node_slot]->timer_id ());
	return -1;
    }else
    {
	TimerNode *temp = this->remove (timer_node_slot);
	// Call the close hooks.
	if (!dont_call)
	{
	    EventHandler *eh = temp->handler ();
	    if (eh)
		eh->handle_close (temp->arg (), EventHandler::TIMER_MASK);
	}
	if (arg != 0)
	    *arg = temp->arg ();
	this->free_node (temp);	
    }
    return 0;
}
int TimerQueue::cancel (const EventHandler *eh,
	int dont_call_handle_close/* = 1*/)
{
    TRACE ("TimerQueue");
    if (eh == 0) return 0;
    int number_of_cancellations = 0;
    for (size_t i = 0; i < this->cur_size_; )
    {
	if (this->heap_[i]->handler () == eh)
	{
	    TimerNode *temp = 0;
	    temp = this->remove (i);
	    number_of_cancellations++;
	    this->free_node (temp);
	    // We reset to zero so that we don't miss checking any nodes
	    // if a reheapify occurs when a node is removed.  There
	    // may be a better fix than this, however.
	    i = 0;
	}else
	    ++i;
    }
    if (!dont_call_handle_close)
    {
	EventHandler *e = const_cast<EventHandler *>(eh);
	e->handle_close (reinterpret_cast<const void *>(0), 
		EventHandler::TIMER_MASK);
    }
    return number_of_cancellations;
}
int TimerQueue::reset_interval (int timer_id,
	const TimeValue &interval)
{
    TRACE ("TimerQueue");
    //Guard_T g (this->mutex_);
    // Check to see if the timer_id is out of range
    if (timer_id < 0
	    || (size_t) timer_id > this->max_size_)
	return -1;
    int timer_node_slot = this->timer_ids_[timer_id];
    // Check to see if timer_id is still valid.
    if (timer_node_slot < 0) return -1;
    if (timer_id != this->heap_[timer_node_slot]->timer_id ())
    {
	NDK_ASSERT (timer_id == this->heap_[timer_node_slot]->timer_id ());
	return -1;
    }else
    {
	// Reset the timer interval
	this->heap_[timer_node_slot]->interval (interval);
    }
    return 0;
}
int TimerQueue::expire (const TimeValue &current_time)
{
    TRACE ("TimerQueue");
    //Guard_T g (this->mutex_);
    // Keep looping while there are timers remaining and the earliest
    // timer is <= the <cur_time> passed in to the method.
    if (this->is_empty ())
	return 0;
    int number_of_timers_expired = 0;

    TimerNode dispatched_node;
    while (this->dispatch_timer (current_time, dispatched_node))
    {
	if (this->upcall (dispatched_node.handler (),
		&EventHandler::handle_timeout,
		dispatched_node.arg (),
		current_time) < 0)
	{
	    if (dispatched_node.interval () > TimeValue::zero)
	    {
		this->cancel (dispatched_node.timer_id (),
		    0,
		    0);
	    }
	    // We must ensure the timer that expired one time, notify to user
	    // when it call done
	    else
	    {
		dispatched_node.handler ()->handle_close (
			dispatched_node.arg (),
			EventHandler::TIMER_MASK);
	    }
	    /*
	    TimerNode *temp = 
		this->remove (this->timer_ids_[dispatched_node.timer_id ()]);
	    if (temp && temp->handler ())
	    {
		temp->handler ()->handle_close (temp->arg (), 
			EventHandler::TIMER_MASK);
	    }
	    */
	}
	++number_of_timers_expired;
    }
    return number_of_timers_expired;
}
int TimerQueue::dispatch_timer (const TimeValue &current_time,
	TimerNode &dispatched_node)
{
    TRACE ("TimerQueue");
    if (this->is_empty ())
	return 0;

    TimerNode *expired = 0;
    if (this->earliest_time () <= current_time)
    {
	expired = this->remove_first ();

	dispatched_node = *expired;

	if (expired->interval () > TimeValue::zero)
	{
	    // Make sure that we skip past values that have already 
	    // 'expired'.
	    do
	    {
		expired->timer_value (expired->timer_value () + 
			expired->interval ());
	    }while (expired->timer_value () <= current_time);
	    // Since this is an interval timer, we need to reschedule it.
	    this->reschedule (expired);
	}else
	{
	    // Call the factory method to free up the node.
	    this->free_node (expired);
	}
	return 1;
    }
    return 0;
}
TimerNode *TimerQueue::remove_first ()
{
    TRACE ("TimerQueue");
    if (this->cur_size_ == 0)
	return 0;
    return this->remove (0);
}
int TimerQueue::upcall (EventHandler *event_handler,
	int (EventHandler::*callback)(const void *act, const TimeValue &),
	const void *arg,
	const TimeValue &current_time)
{
    if (event_handler == 0) return -1;
    return (event_handler->*callback)(arg, current_time);
}


