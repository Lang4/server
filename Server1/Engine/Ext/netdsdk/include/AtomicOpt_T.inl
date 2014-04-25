template<typename TYPE, typename MUTEX> inline
AtomicOpt_T<TYPE, MUTEX>::AtomicOpt_T ()
:data_ (0)
{
}
template<typename TYPE, typename MUTEX> inline
AtomicOpt_T<TYPE, MUTEX>::AtomicOpt_T (const TYPE &v)
{
    this->data_ = v;
}
template<typename TYPE, typename MUTEX> inline
AtomicOpt_T<TYPE, MUTEX> &AtomicOpt_T<TYPE, MUTEX>::operator = (const TYPE &v)
{
    Guard_T<MUTEX> g (this->mutex_);
    this->data_ = v;
    return *this;
}
template<typename TYPE, typename MUTEX> inline
AtomicOpt_T<TYPE, MUTEX> &AtomicOpt_T<TYPE, MUTEX>::operator = (const AtomicOpt_T<TYPE, MUTEX> &v)
{
    if (&v == this)
	return *this;
    Guard_T<MUTEX> g (this->mutex_);
    this->data_ = v.value ();
    return *this;
}
template<typename TYPE, typename MUTEX> inline
TYPE AtomicOpt_T<TYPE, MUTEX>::operator ++ (void) 
{
    Guard_T<MUTEX> g (this->mutex_);
    return ++this->data_;
}
template<typename TYPE, typename MUTEX> inline
TYPE AtomicOpt_T<TYPE, MUTEX>::operator ++ (int) 
{
    Guard_T<MUTEX> g (this->mutex_);
    return this->data_++;
}
template<typename TYPE, typename MUTEX> inline
TYPE AtomicOpt_T<TYPE, MUTEX>::operator += (const TYPE &rhs) 
{
    Guard_T<MUTEX> g (this->mutex_);
    return this->data_ += rhs;
}
template<typename TYPE, typename MUTEX> inline
TYPE AtomicOpt_T<TYPE, MUTEX>::operator -- (void) 
{
    Guard_T<MUTEX> g (this->mutex_);
    return --this->data_;
}
template<typename TYPE, typename MUTEX> inline
TYPE AtomicOpt_T<TYPE, MUTEX>::operator -- (int) 
{
    Guard_T<MUTEX> g (this->mutex_);
    return this->data_--;
}
template<typename TYPE, typename MUTEX> inline
TYPE AtomicOpt_T<TYPE, MUTEX>::operator -= (const TYPE &rhs) 
{
    Guard_T<MUTEX> g (this->mutex_);
    return this->data_ -= rhs;
}
template<typename TYPE, typename MUTEX> inline
bool AtomicOpt_T<TYPE, MUTEX>::operator == (const TYPE &rhs) 
{
    Guard_T<MUTEX> g (this->mutex_);
    return this->data_ == rhs;
}
template<typename TYPE, typename MUTEX> inline
bool AtomicOpt_T<TYPE, MUTEX>::operator != (const TYPE &rhs) 
{
    Guard_T<MUTEX> g (this->mutex_);
    return this->data_ != rhs;
}
template<typename TYPE, typename MUTEX> inline
bool AtomicOpt_T<TYPE, MUTEX>::operator  < (const TYPE &rhs) 
{
    Guard_T<MUTEX> g (this->mutex_);
    return this->data_ < rhs;
}
template<typename TYPE, typename MUTEX> inline
bool AtomicOpt_T<TYPE, MUTEX>::operator <= (const TYPE &rhs) 
{
    Guard_T<MUTEX> g (this->mutex_);
    return this->data_ <= rhs;
}
template<typename TYPE, typename MUTEX> inline
bool AtomicOpt_T<TYPE, MUTEX>::operator  > (const TYPE &rhs) 
{
    Guard_T<MUTEX> g (this->mutex_);
    return this->data_ > rhs;
}
template<typename TYPE, typename MUTEX> inline
bool AtomicOpt_T<TYPE, MUTEX>::operator >= (const TYPE &rhs) 
{
    Guard_T<MUTEX> g (this->mutex_);
    return this->data_ >= rhs;
}
template<typename TYPE, typename MUTEX> inline
TYPE AtomicOpt_T<TYPE, MUTEX>::value (void)
{
    Guard_T<MUTEX> g (this->mutex_);
    return this->data_;
}

