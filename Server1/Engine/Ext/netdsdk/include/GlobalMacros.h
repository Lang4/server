//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-02-14 13:24
 */
//========================================================================

#ifndef _GLOBAL_MACROS_H_
#define _GLOBAL_MACROS_H_
#include "Pre.h"

#include <stdlib.h>
#include <pthread.h>        // for pthread_t
// =======================================================================
// Type define 
// =======================================================================
#if __Win32__
    typedef HANDLE NDK_HANDLE;
    typedef SOCKET NDK_SOCKET;
#   define NDK_INVALID_HANDLE INVALID_HANDLE_VALUE

#else  /* !__Win32__ */

    typedef int NDK_HANDLE;
    typedef NDK_HANDLE NDK_SOCKET;
#   define NDK_INVALID_HANDLE -1

    typedef pthread_t thread_t;
#   define NULL_thread  0
    typedef void * THREAD_FUNC_RETURN_T; 
    typedef THREAD_FUNC_RETURN_T (*THREAD_FUNC_T)(void*);

#endif /* __Win32__ */

// =======================================================================
// Bit operation macros 
// =======================================================================
#define NDK_BIT_ENABLED(WORD, BIT)  (((WORD) & (BIT)) != 0)
#define NDK_BIT_DISABLED(WORD, BIT) (((WORD) & (BIT)) == 0)

#define NDK_SET_BITS(WORD, BITS) (WORD |= (BITS))
#define NDK_CLR_BITS(WORD, BITS) (WORD &= ~(BITS))


// =======================================================================
// Useful macros
// =======================================================================
#ifndef NDK_DEBUG
#   define NDK_ASSERT(_condition)
#else
/*inline 
void __ndk_assert (const char* __file, int __line,
	const char* __function, const char* __condition)
{
    NDK_INF("NDK_ASSERT : in file `%s` on line `%d` : `%s` "
	    "assertion failed for '%s'\n", 
	    __file, __line, __function, __condition);
}
#   define NDK_ASSERT(_condition)		\
    do {					\
	if (!(_condition))			\
	__ndk_assert (__FILE__,			\
		__LINE__,			\
		__PRETTY_FUNCTION__,		\
	    #_condition);			\
    }while (false)
*/
#   define NDK_ASSERT(_condition)		\
    do {					\
	if (!(_condition))			\
	{					\
	    NDK_INF ("NDK_ASSERT : in file "	\
		"`%s` on line `%d` : `%s` "	\
		"assertion failed for '%s'"	\
		"\n",				\
		__FILE__,			\
		__LINE__,			\
		__PRETTY_FUNCTION__,		\
		#_condition);			\
	    abort ();				\
	}					\
    }while (false)
#endif

#define SET_ERRNO_RETURN(x)			\
	int ret = x;				\
	if (ret != 0) errno = ret;		\
	return ret == 0 ? 0 : -1

#include "Post.h"
#endif


