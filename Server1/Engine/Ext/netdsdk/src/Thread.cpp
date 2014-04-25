#include "Thread.h"
#include "Debug.h"
#include "GlobalMacros.h"

#include <limits.h>

int Thread::spawn (THREAD_FUNC_T func, 
	void *arg/* = NULL*/, 
	thread_t *thr_id/* = 0*/, 
	int flags/* = THR_JOIN*/, 
	size_t stack_size/* = 0*/)
{
    int ret_val = 0;
    // default thread stack size is 2M
    pthread_attr_t attr;
    // init
    if (::pthread_attr_init (&attr) != 0) return -1;
    if (stack_size != 0)
    {
	if (stack_size < PTHREAD_STACK_MIN)
	   stack_size = PTHREAD_STACK_MIN; 

	if (::pthread_attr_setstacksize (&attr, stack_size) != 0)
	{
	    NDK_DBG ("set stacksize failed");
	    ::pthread_attr_destroy (&attr);
	    return -1;
	}
    }
    thread_t id;
    if (thr_id == 0)
	thr_id = &id;
    if (::pthread_create (thr_id, &attr, func, arg) == 0)
    {
	if (NDK_BIT_ENABLED (flags, THR_DETACH))
	    ::pthread_detach (*thr_id);
    }else
    {
	NDK_DBG ("create pthread failed");
	ret_val = -1;
    }
    // release
    ::pthread_attr_destroy (&attr);
    return ret_val;
}

