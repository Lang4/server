inline
TaskWithMQ::TaskWithMQ (ThreadManager *thr_mgr/* = 0*/,
	MessageQueue *mq/* = 0*/)
: TaskBase (thr_mgr)
, delete_msg_queue_ (0)
, msg_queue_ (0)
{
    if (mq == 0)
    {
	mq = new MessageQueue;
	if (mq)
	    delete_msg_queue_ = 1;
    }
    if (mq)
	msg_queue_ = mq;
}
inline
TaskWithMQ::~TaskWithMQ ()
{
    if (this->delete_msg_queue_)
	delete this->msg_queue_;
    this->delete_msg_queue_ = 0;
}
inline
MessageQueue *TaskWithMQ::msg_queue ()
{
    return this->msg_queue_;
}
inline
void TaskWithMQ::msg_queue (MessageQueue *msg_queue)
{
    if (this->delete_msg_queue_)
    {
	delete this->msg_queue_;
	this->delete_msg_queue_ = 0;
    }
    this->msg_queue_ = msg_queue;
}
inline
int TaskWithMQ::putq (MessageBlock *mb, 
	const TimeValue* timeout/*= 0*/)
{
    return this->msg_queue_->enqueue_tail (mb, timeout);
}
inline
int TaskWithMQ::getq (MessageBlock *&mb, 
	const TimeValue* timeout/*= 0*/)
{
    return this->msg_queue_->dequeue_head (mb, timeout);
}
inline
int TaskWithMQ::getq_n (MessageBlock *&mb, int number/* = -1*/,
	const TimeValue* timeout/*= 0*/)
{
    return this->msg_queue_->dequeue_head_n (mb,number, timeout);
}
inline
int TaskWithMQ::ungetq (MessageBlock *mb, 
	const TimeValue* timeout/*= 0*/)
{
    return this->msg_queue_->enqueue_head (mb, timeout);
}


