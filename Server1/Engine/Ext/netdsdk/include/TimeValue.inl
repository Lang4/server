inline
TimeValue::TimeValue ()
{
    set (0, 0);
}
inline
TimeValue::TimeValue (const TimeValue& tv)
{
    set (tv.tv_.tv_sec, tv.tv_.tv_usec);
}
inline
TimeValue::TimeValue (const timeval &tv)
{
    set (tv);
}
inline
TimeValue::TimeValue (const timespec &tv)
{
    set (tv);
}
inline
TimeValue::TimeValue (time_t sec, suseconds_t usec)
{
    set (sec, usec);
}
inline
void TimeValue::set (const timeval &tv)
{
    this->set (tv.tv_sec, tv.tv_usec);
}
inline
void TimeValue::set (time_t sec, suseconds_t usec)
{
    this->tv_.tv_sec  = sec;
    this->tv_.tv_usec = usec;
#if __GNUC__
    if (__builtin_constant_p(sec) &&
	    __builtin_constant_p(usec) &&
	    (sec >= 0 && usec >= 0 && usec < 1000000))
	return ;
#endif
    this->normalize ();
}
inline
time_t TimeValue::sec (void) const
{
    return this->tv_.tv_sec;
}
inline
void TimeValue::sec (time_t sec)
{
    this->tv_.tv_sec = sec;
}
inline
unsigned long TimeValue::msec (void) const
{
    return this->tv_.tv_sec * 1000 + this->tv_.tv_usec / 1000;
}
inline
suseconds_t TimeValue::usec (void) const
{
    return this->tv_.tv_usec;
}
inline
void TimeValue::usec (suseconds_t usec)
{
    this->tv_.tv_usec = usec;
}
inline
void TimeValue::gettimeofday ()
{
    struct timeval tv;
    ::gettimeofday (&tv, 0);
    this->set (tv);
}
inline
bool operator > (const TimeValue &tv1, const TimeValue &tv2)
{
    if (tv1.sec () > tv2.sec ())
	return true;
    else if (tv1.sec () == tv2.sec ()
	    && tv1.usec () > tv2.usec ())
	return true;
    return false;
}
inline
bool operator >= (const TimeValue &tv1, const TimeValue &tv2)
{
    if (tv1.sec () > tv2.sec ())
	return true;
    else if (tv1.sec () == tv2.sec ()
	    && tv1.usec () >= tv2.usec ())
	return true;
    return false;
}
inline
bool operator < (const TimeValue &tv1, const TimeValue &tv2)
{
    return tv2 > tv1;
}
inline
bool operator <= (const TimeValue &tv1, const TimeValue &tv2)
{
    return tv2 >= tv1;
}
inline
bool operator == (const TimeValue &tv1, const TimeValue &tv2)
{
    return tv1.sec () == tv2.sec ()
	&& tv1.usec () == tv2.usec ();
}
inline
bool operator != (const TimeValue &tv1, const TimeValue &tv2)
{
    return !(tv1 == tv2);
}
inline
TimeValue::operator timespec () const
{
    timespec tv;
    tv.tv_sec = this->tv_.tv_sec;
    tv.tv_nsec = this->tv_.tv_usec * 1000;
    return tv;
}
inline
TimeValue::operator timeval () const
{
    return this->tv_;
}
inline
TimeValue &TimeValue::operator = (const TimeValue &tv)
{
    this->sec (tv.sec ());
    this->usec (tv.usec ());
    return *this;
}
inline
TimeValue &TimeValue::operator = (time_t tv)
{
    this->sec (tv);
    this->usec (0);
    return *this;
}
inline
TimeValue &TimeValue::operator += (const TimeValue &tv)
{
    this->sec (this->sec () + tv.sec ());
    this->usec (this->usec () + tv.usec ());
    this->normalize ();
    return *this;
}
inline
TimeValue &TimeValue::operator += (const time_t tv)
{
    this->sec (this->sec () + tv);
    return *this;
}
inline
TimeValue &TimeValue::operator -= (const TimeValue &tv)
{
    this->sec (this->sec () - tv.sec ());
    this->usec (this->usec () - tv.usec ());
    this->normalize ();
    return *this;
}
inline
TimeValue &TimeValue::operator -= (const time_t tv)
{
    this->sec (this->sec () - tv);
    return *this;
}
inline
TimeValue operator + (const TimeValue &tv1, const TimeValue &tv2)
{
    TimeValue sum (tv1);
    sum += tv2;
    return sum;
}
inline
TimeValue operator - (const TimeValue &tv1, const TimeValue &tv2)
{
    TimeValue sum (tv1);
    sum -= tv2;
    return sum;
}

