inline
MessageQueue::MessageQueue ()
: mb_count_ (0)
, head_ (0)
, tail_ (0)
, not_empty_cond_ (queue_mutex_)
{
}
inline
size_t MessageQueue::size ()
{
    Guard_T<MUTEX> g (this->queue_mutex_);
    return mb_count_;
}
inline
bool MessageQueue::is_empty ()
{
    Guard_T<MUTEX> g (this->queue_mutex_);
    return this->is_empty_i ();
}
inline
bool MessageQueue::is_empty_i ()
{
    return this->tail_ == 0;
}


