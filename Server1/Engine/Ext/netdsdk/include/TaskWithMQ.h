//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-04-01 11:08
 */
//========================================================================

#ifndef _TASKWITHMQ_H_
#define _TASKWITHMQ_H_
#include "Pre.h"

#include "TaskBase.h"
#include "MessageQueue.h"

/**
 * @class TaskWithMQ
 *
 * @brief 
 */
class TaskWithMQ : public TaskBase
{
public:
    /**
     * Initialize a Task, supplying a thread manager and a message
     * queue.  If the user doesn't supply a ACE_Message_Queue pointer
     * then we'll allocate one dynamically.  Otherwise, we'll use the
     * one passed as a parameter.
     */
    TaskWithMQ (ThreadManager *thr_mgr = 0,
	    MessageQueue *mq = 0);

    // Destructor
    virtual ~TaskWithMQ ();

    // Gets the message queue associated with this task.
    MessageQueue *msg_queue (void);

    // Sets the message queue associated with this task.
    void msg_queue (MessageQueue *);

    // Insert message into the message queue tail.
    int putq (MessageBlock *, const TimeValue* timeout = 0);

    // Extract the first message from the queue. 
    int getq (MessageBlock *&, const TimeValue* timeout = 0);

    // Extract n message from the queue head;
    int getq_n (MessageBlock *&, int number = -1, const TimeValue* timeout = 0);

    // Return a message to the queue.
    int ungetq (MessageBlock *, const TimeValue* timeout = 0);
protected:
    // 1 if should delete MessageQueue, 0 otherwise.
    int delete_msg_queue_;

    // Queue of messages on the Task
    MessageQueue *msg_queue_;
};

#include "TaskWithMQ.inl"

#include "Post.h"
#endif

