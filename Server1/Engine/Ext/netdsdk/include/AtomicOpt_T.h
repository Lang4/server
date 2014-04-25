//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-06 15:55
 */
//========================================================================

#ifndef _ATOMICOPT_T_H_
#define _ATOMICOPT_T_H_
#include "Pre.h"

#include "Guard_T.h"

/**
 * @class AtomicOpt_T
 *
 * @brief 
 */

class ThreadMutex;
template<typename TYPE, typename MUTEX = ThreadMutex>
class AtomicOpt_T
{
public:
    // default init data to 0
    AtomicOpt_T ();

    // init value to data
    explicit AtomicOpt_T (const TYPE &v);

    // copy constructor 
    //AtomicOpt_T (const AtomicOpt_T<TYPE, MUTEX> &v);
    
    // assgin
    AtomicOpt_T<TYPE, MUTEX> &operator = (const TYPE &v);

    //
    AtomicOpt_T<TYPE, MUTEX> &operator = (const AtomicOpt_T<TYPE, MUTEX> &v);

    // atomically pre-increment value_.
    TYPE operator ++ (void);

    // atomically post-increment value_.
    TYPE operator ++ (int);

    // atomically increment value_ by rhs
    TYPE operator += (const TYPE &rhs);

    // atomically pre-decrement value_.
    TYPE operator -- (void);

    // atomically post-decrement value_.
    TYPE operator -- (int);

    // atomically decrement value_ by rhs.
    TYPE operator -= (const TYPE &rhs);

    // atomically compare value_ with rhs.
    bool operator == (const TYPE &rhs);

    // atomically compare value_ with rhs.
    bool operator != (const TYPE &rhs);

    // atomically check if value_ less than rhs.
    bool operator  < (const TYPE &rhs);
    
    // atomically check if value_ less than or equal to rhs.
    bool operator <= (const TYPE &rhs);

    // atomically check if value_ greater than rhs.
    bool operator  > (const TYPE &rhs);

    // atomically check if value_ greater than or equal to rhs.
    bool operator >= (const TYPE &rhs);
    
    // return value
    TYPE value (void);
private:
    TYPE   data_;
    MUTEX  mutex_;
};

#include "AtomicOpt_T.inl"
#include "Post.h"
#endif

