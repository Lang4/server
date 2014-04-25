inline
RecursiveThreadMutex::RecursiveThreadMutex ()
: nesting_level_ (0)
, owner_thr_ (0)
{
    ::pthread_mutex_init (&this->mutex_, NULL);
}
inline
RecursiveThreadMutex::~RecursiveThreadMutex ()
{
    ::pthread_mutex_destroy (&this->mutex_);
}
inline
const pthread_mutex_t &RecursiveThreadMutex::lock () const
{
    return mutex_;
}
