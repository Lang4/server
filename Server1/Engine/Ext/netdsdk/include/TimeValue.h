//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-04-22 22:45
 */
//========================================================================

#ifndef _TIMEVALUE_H_
#define _TIMEVALUE_H_
#include "Pre.h"

#include <sys/time.h>
#include <sys/select.h>

/**
 * @class TimeValue
 *
 * @brief 
 */
class TimeValue
{
public:
    // Constant "0".
    static const TimeValue zero;
    
    // Default Constructor.
    TimeValue ();

    // Copy Constructor.
    TimeValue (const TimeValue& tv);

    // Constructor
    explicit TimeValue (time_t sec, suseconds_t usec = 0);

    // Construct the TimeValue from a timeval.
    explicit TimeValue (const timeval &t);

    // Construct the TimeValue object from a timespec_t.
    explicit TimeValue (const timespec &t);

    TimeValue &operator = (const TimeValue &tv);

    TimeValue &operator = (time_t tv);

    // Initializes the TimeValue from seconds and useconds.
    void set (time_t sec, suseconds_t usec);

    // Initializes the TimeValue from a timeval.
    void set (const timeval &t);

    // Initializes the TimeValue object from a timespec_t.
    void set (const timespec &t);

    unsigned long msec (void) const;

    // Returns the value of the object as a timespec_t.
    operator timespec () const;

    // Returns the value of the object as a timeval.
    operator timeval () const;

    // Returns a pointer to the object as a timeval.
    operator const timeval *() const;

    // = The following are accessor/mutator methods.
    time_t sec (void) const;

    void sec (time_t sec);

    suseconds_t usec (void) const;

    void usec (suseconds_t usec);

    // Get current time value
    void gettimeofday ();

    TimeValue &operator += (const TimeValue &tv);

    TimeValue &operator += (const time_t tv);

    TimeValue &operator -= (const TimeValue &tv);

    TimeValue &operator -= (const time_t tv);

    friend TimeValue operator + (const TimeValue &tv1, 
	    const TimeValue &tv2);

    friend TimeValue operator - (const TimeValue &tv1, 
	    const TimeValue &tv2);

    friend bool operator < (const TimeValue &tv1,
	    const TimeValue &tv2);

    friend bool operator > (const TimeValue &tv1,
	    const TimeValue &tv2);

    friend bool operator <= (const TimeValue &tv1,
	    const TimeValue &tv2);

    friend bool operator >= (const TimeValue &tv1,
	    const TimeValue &tv2);

    friend bool operator == (const TimeValue &tv1,
	    const TimeValue &tv2);

    friend bool operator != (const TimeValue &tv1,
	    const TimeValue &tv2);
protected:
    // Put the timevalue into a canonical form.
    void normalize (void);
private:
    struct timeval tv_;
};

#include "TimeValue.inl"
#include "Post.h"
#endif

