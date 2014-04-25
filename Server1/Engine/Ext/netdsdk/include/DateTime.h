//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2007-12-26 18:28
 */
//========================================================================

#ifndef _DATETIME_H_
#define _DATETIME_H_
#include "Pre.h"

#include <time.h>
#include <stdio.h>

/**
 * @class DateTime
 *
 * @brief 
 */
class DateTime
{
public:
    // Constructors 
    DateTime (void);
    DateTime (const DateTime &dt);
    DateTime (const struct tm &tm_);
    explicit DateTime (const time_t tt);
    
    // Override operator
    DateTime& operator = (const DateTime &dt);
    DateTime  operator - (const DateTime &dt) const;
    DateTime  operator - (const DateTime &dt);
    DateTime  operator + (const DateTime &dt) const;
    DateTime  operator + (const DateTime &dt);

    bool operator < (const DateTime &dt) const;
    bool operator < (const DateTime &dt);
    bool operator <=(const DateTime &dt) const;
    bool operator <=(const DateTime &dt);
    bool operator ==(const DateTime &dt) const; 
    bool operator ==(const DateTime &dt); 
    
    int year  (void) const;
    int month (void) const;
    int wday  (void) const;
    int mday  (void) const;
    int hour  (void) const;
    int min   (void) const;
    int sec   (void) const;
 
    void year (const int year);
    void month(const int nmon);
    void mday (const int nday);
    void hour (const int nhou);
    void min  (const int nmin);
    void sec  (const int nsec);

    // Get time
    time_t time(void) const;

    // Format date and time to "2008-12-12 23:23:23"
    void to_str (char *str, int len);

    // Format date to "2008-12-12"
    void date_to_str (char *str, int len);

    // Format time to "23:23:23"
    void time_to_str (char *str, int len);

    // Get current date and time
    void update (void);

    // Set date and time
    void update (time_t dtime);
private:
    time_t    time_;
    struct tm tm_;
};

#include "DateTime.inl"
#include "Post.h"
#endif

