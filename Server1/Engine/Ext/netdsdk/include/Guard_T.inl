template<typename LOCK> inline
Guard_T<LOCK>::Guard_T (LOCK &lock)
:lock_ (lock)
{
    lock_.acquire ();
}
template<typename LOCK> inline
Guard_T<LOCK>::~Guard_T ()
{
    lock_.release ();
}
template<typename LOCK> inline
int Guard_T<LOCK>::acquire ()
{
    return lock_.acquire ();
}
template<typename LOCK> inline
int Guard_T<LOCK>::tryacquire ()
{
    return  lock_.tryacquire ();
}
template<typename LOCK> inline
int Guard_T<LOCK>::release ()
{
    return lock_.release ();
}


