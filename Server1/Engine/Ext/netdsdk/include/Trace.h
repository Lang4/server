//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-10 23:38
 */
//========================================================================

#ifndef _TRACE_H__
#define _TRACE_H__
#include "Pre.h"

#include "Thread.h"

#include <cstdio>

/**
 * @class Trace
 *
 * @brief 
 */
class Trace
{
public:
    Trace (const char *file, const char *func, int line);
    ~Trace ();
private:
    int          line_;
    const  char* file_;
    const  char* func_;
    static int   count_;
};
#ifdef NDK_TRACE
#   define TRACE(X) Trace ____ (__FILE__,  __PRETTY_FUNCTION__, __LINE__)
#else
#   define TRACE(X)
#endif

#include "Trace.inl"
#include "Post.h"
#endif

