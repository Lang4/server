//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-03-31 11:03
 */
//========================================================================

#ifndef _DEBUG_H_
#define _DEBUG_H_
#include "Pre.h"

#include "LogSvr.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/**
 * @class Debug
 *
 * @brief 
 */
class Debug : public LogSvr
{
    friend class Singleton_T<Debug, ThreadMutex>;
protected:
    Debug ();
    ~Debug ();
};

typedef Singleton_T<Debug, ThreadMutex> NDK_DEBUG_;

#ifdef NDK_DEBUG
#   define NDK_DBG(x) NDK_DEBUG_::instance ()->put (LOG_DEBUG, x\
	" [errno = %d][errdes = %s]\n", errno, strerror (errno))
#   define NDK_INF(x, ...) NDK_DEBUG_::instance ()->put (LOG_RINFO,\
	x, ##__VA_ARGS__)
#else
#   define NDK_DBG(x) 
#   define NDK_INF(x, ...) 
#endif

#include "Debug.inl"
#include "Post.h"

#endif

