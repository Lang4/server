//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-05 18:35
 */
//========================================================================

#ifndef _GUARD_T_H_
#define _GUARD_T_H_
#include "Pre.h"

/**
 * @class Guard_T
 *
 * @brief 
 */
template<typename LOCK>
class Guard_T
{
public:
    // auto lock
    Guard_T (LOCK &lock);

    // auto unlock
    ~Guard_T ();
    //
    int acquire (void);

    // 
    int release (void);

    //
    int tryacquire ();
private:
    LOCK &lock_;
};

#include "Guard_T.inl"
#include "Post.h"
#endif

