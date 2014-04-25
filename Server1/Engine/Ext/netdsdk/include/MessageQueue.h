//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-08 12:29
 */
//========================================================================

#ifndef _MESSAGEQUEUE_H_
#define _MESSAGEQUEUE_H_
#include "Pre.h"

#include "Guard_T.h"
#include "Semaphore.h"
#include "ConditionThreadMutex.h"
#include "ThreadMutex.h"
#include "MessageBlock.h"

/**
 * @class MessageQueue
 *
 * @brief This class can be used to communicate between muilt-threads, notify 
 * mechanism use Semaphore.
 *
 * Note : Use Semaphore is better than Condition when enqueue MessageBlock 
 *	  high frequency.
 */
class MessageQueue
{
public:
    // init
    MessageQueue ();

    // releases all resources from the message queue
    virtual ~MessageQueue ();
    
    /**
     * @param new_item can be a list or a single MessageBlock,
     *	      timeout: = 0, block;  nonzero , block until timeout
     * return -1: enqueue failed
     * return >0: the number of mbs enqueue successfully
     * return  0: enqueue 0 mb 
     */
    int enqueue_tail (MessageBlock *new_item, 
	    const TimeValue *timeout = 0);

    /**
     * @param new_item can be a list or a single MessageBlock,
     *	      timeout: = 0, block;  nonzero , block until timeout
     * return -1: enqueue failed
     * return >0: the number of mbs enqueue successfully
     * return  0: enqueue 0 mb 
     */
    int enqueue_head (MessageBlock *new_item, 
	    const TimeValue *timeout = 0);

    /**
     * @param first_item: reference to a mb-pointer that will be set 
     * to the address of the dequeued block
     *	      timeout: = 0, block;  nonzero , block until timeout
     * return -1: dequeue failed, timeout or error
     * return  0: dequeue successfully
     */
    int dequeue_head (MessageBlock *&first_item,  
	    const TimeValue *timeout = 0); 

    /**
     * This method can dequeue the <number> of the MessageBlock that in 
     * MessageQueue, it will dequeue all of the member in the MessageQueue
     * if <number> = -1, 
     * return the number of dequeued successfully actually until <timeout> 
     * timeout. dequeue items from MessageBlock as best as it can if timeout
     * is nonzero. if timeout == 0, it return until dequeue the <number> 
     * of the MessageBlock
     * Note : items is a list, you must check items->next () != 0
     */ 
    int dequeue_head_n (MessageBlock *&items, int number = -1, 
	    const TimeValue *timeout = 0);

    /**
     * @param last_item: reference to a mb-pointer that will be set 
     * to the address of the dequeued block
     * @param timeout: = 0, block;  nonzero , block until timeout
     * return -1: dequeue failed, timeout or error
     * return  0: dequeue successfully
     */
    int dequeue_tail (MessageBlock *&last_item, 
	    const TimeValue *timeout = 0); 

    /**
     * This method can dequeue the <number> of the MessageBlock that in 
     * MessageQueue, it will dequeue all of the member in the MessageQueue
     * if <number> = -1, 
     * return the number of dequeued successfully actually until <timeout> 
     * timeout. dequeue items from MessageBlock as best as it can if timeout
     * is non zero. if timeout is 0, it return until dequeue the <number> 
     * of the MessageBlock
     * Note : items is a list, you must check items->next () != 0
     */ 
    int dequeue_tail_n (MessageBlock *&items, int number = -1, 
	    const TimeValue *timeout = 0);

    // get size of message queue
    size_t size (void);

    // check MessageQueue is empty or not
    bool is_empty ();
protected:
    //
    bool is_empty_i ();

    //
    int enqueue_tail_i (MessageBlock *new_item);

    //
    int enqueue_head_i (MessageBlock *new_item);

    //
    int dequeue_head_i (MessageBlock *&first_item);

    //
    int dequeue_head_n_i (MessageBlock *&items, int number);

    //
    int dequeue_tail_i (MessageBlock *&last_item);

    //
    int dequeue_tail_n_i (MessageBlock *&items, int number);

//
private:
    // count the message block in queue
    size_t	  mb_count_;
    MessageBlock *head_;
    MessageBlock *tail_;

    // sem and lock
    Semaphore not_empty_sem_;
    ConditionThreadMutex not_empty_cond_;
    //
    typedef ThreadMutex MUTEX;
    MUTEX     queue_mutex_;
};

#include "MessageQueue.inl"
#include "Post.h"
#endif

