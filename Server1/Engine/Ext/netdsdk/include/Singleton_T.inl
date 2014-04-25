template<typename TYPE, typename LOCK>
TYPE *Singleton_T<TYPE, LOCK>::instance_ = 0;
template<typename TYPE, typename LOCK>
LOCK *Singleton_T<TYPE, LOCK>::lock_ = new LOCK;

template<typename TYPE, typename LOCK> inline
TYPE *Singleton_T<TYPE, LOCK>::instance ()
{
    if (instance_ == 0)
    {
	Guard_T<LOCK> lock (*lock_);
	if (instance_ == 0)
	    instance_ = new TYPE;
    }
    return instance_;
}
