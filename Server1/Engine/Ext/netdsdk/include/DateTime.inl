inline
DateTime::DateTime () 
{ 
    time_ = ::time (NULL); 
    ::localtime_r (&time_, &tm_); 
}
inline
DateTime::DateTime (const DateTime &dt) 
{ 
    time_ = dt.time_; 
    tm_ = dt.tm_;
}
inline
DateTime::DateTime (const time_t tt)     
{ 
    time_  = tt; 
    ::localtime_r (&time_, &tm_); 
} 
inline
DateTime::DateTime (const struct tm &tm_val)
{
    tm_ = tm_val; 
    time_ = ::mktime (&tm_); 
}
inline
void DateTime::update ()
{
    this->time_ = ::time (NULL); 
    ::localtime_r (&this->time_, &this->tm_); 
}
inline
void DateTime::update (time_t dtime)
{
    this->time_ = dtime;
    ::localtime_r (&this->time_, &this->tm_); 
}
inline
time_t DateTime::time () const 
{ 
    return this->time_ ;   
}
inline
int DateTime::year  () const { return this->tm_.tm_year + 1900; }
inline
int DateTime::month () const { return this->tm_.tm_mon  + 1;    }
inline
int DateTime::mday  () const { return this->tm_.tm_mday ;       }
inline
int DateTime::wday  () const { return this->tm_.tm_wday ;       }
inline
int DateTime::hour  () const { return this->tm_.tm_hour ;       }
inline
int DateTime::min   () const { return this->tm_.tm_min ;        }
inline
int DateTime::sec   () const { return this->tm_.tm_sec ;        }

inline
void DateTime::year (const int nyear)
{
    this->tm_.tm_year = nyear;
    this->time_ = mktime(&this->tm_);
}
inline
void DateTime::month (const int nmonth)
{
    this->tm_.tm_mon = nmonth - 1;
    this->time_ = mktime(&this->tm_); 
}
inline
void DateTime::mday (const int nday)
{ 
    this->tm_.tm_mday = nday; 
    this->time_ = mktime(&this->tm_); 
}
inline
void DateTime::hour (const int nhou) 
{ 
    this->tm_.tm_hour = nhou;
    this->time_ = mktime(&this->tm_); 
}
inline
void DateTime::min (const int nmin) 
{ 
    this->tm_.tm_min  = nmin;
    this->time_ = mktime(&this->tm_); 
}
inline
void DateTime::sec (const int nsec) 
{ 
    this->tm_.tm_sec  = nsec; 
    this->time_ = mktime(&this->tm_); 
}
inline
DateTime& DateTime::operator = (const DateTime &dt) 
{
    if (this != &dt)
    {
	this->time_ = dt.time_;
	this->tm_   = dt.tm_;
    }
    return *this;
}
inline
DateTime DateTime::operator - (const DateTime &dt) const
{
    return DateTime (this->time_ - dt.time_);
}
inline
DateTime DateTime::operator - (const DateTime &dt)
{
    return DateTime (this->time_ - dt.time_);
}
inline
DateTime DateTime::operator + (const DateTime &dt) const
{
    return DateTime (this->time_ + dt.time_);
}
inline
DateTime DateTime::operator + (const DateTime &dt)
{
    return DateTime (this->time_ + dt.time_);
}
inline
bool DateTime::operator < (const DateTime &dt) const
{
    return this->time_ < dt.time_;
}
inline
bool DateTime::operator < (const DateTime &dt)
{
    return this->time_ < dt.time_;
}
inline
bool DateTime::operator <=(const DateTime &dt) const
{
    return *this < dt || *this == dt;
}
inline
bool DateTime::operator <=(const DateTime &dt)
{
    return *this < dt || *this == dt;
}
inline
bool DateTime::operator ==(const DateTime &dt) const
{
    return this->time_ == dt.time_;
}
inline
bool DateTime::operator ==(const DateTime &dt)
{
    return this->time_ == dt.time_;
}
inline
void DateTime::to_str (char *str, int len)
{
    if (len <= 0) return;
    snprintf (str, len, "%04d-%02d-%02d %02d:%02d:%02d",
	    this->tm_.tm_year + 1900, this->tm_.tm_mon + 1, 
	    this->tm_.tm_mday, this->tm_.tm_hour, 
	    this->tm_.tm_min, this->tm_.tm_sec);
    if (len > 20) // length of "2000-12-12 23:23:23" = 19
	len = 20;
    str[len-1] = '\0';
}
inline
void DateTime::date_to_str (char *str, int len) 
{
    if (len <= 0) return ;
    snprintf (str, len, "%04d-%02d-%02d", 
	    this->tm_.tm_year + 1900, 
	    this->tm_.tm_mon + 1, 
	    this->tm_.tm_mday);
    if (len > 11) // length of "2000-12-12" = 10
	len = 11;
    str[len-1] = '\0';
}
inline
void DateTime::time_to_str (char *str, int len) 
{
    if (len <= 0) return ;
    snprintf (str, len, "%02d:%02d:%02d", 
	    this->tm_.tm_hour, 
	    this->tm_.tm_min, 
	    this->tm_.tm_sec);
    if (len > 9) // length of "23:23:23" = 8
	len = 9;
    str[len-1] = '\0';
}


