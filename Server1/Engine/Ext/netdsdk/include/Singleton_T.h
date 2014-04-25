//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-05 18:11
 */
//========================================================================

#ifndef _SINGLETON_H_
#define _SINGLETON_H_
#include "Pre.h"

#include "Guard_T.h"

/**
 * @class Singleton_T
 *
 * @brief 
 */
class ThreadMutex;

template<typename TYPE, typename LOCK = ThreadMutex>
class Singleton_T
{
public:
    static TYPE *instance ();
protected:
    Singleton_T ();
    ~Singleton_T ();
private:
    static TYPE *instance_;
    static LOCK *lock_;
};

#include "Singleton_T.inl"
#include "Post.h"
#endif

