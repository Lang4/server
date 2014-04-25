#ifndef _COMMMON_H_
#define _COMMMON_H_
#include "Pre.h"

/**
 * This helps prevent some "unused variable" warnings under, for instance
 */
template <typename T> inline void unused_arg (const T&) { }


#include "Post.h"
#endif


