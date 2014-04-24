#include "platfrom.h"

#ifdef PLATFORM == PLATFORM_WIN32
	#include "mythread_win.cpp"
#else
	#include "mythread_posix.cpp"
#endif