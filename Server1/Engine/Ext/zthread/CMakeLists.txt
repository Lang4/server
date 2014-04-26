set(LIB_HEADERS
)
set(LIB_SRCS 
	src/AtomicCount.cxx 
	src/Condition.cxx 
	src/ConcurrentExecutor.cxx 
	src/CountingSemaphore.cxx 
	src/FastMutex.cxx 
	src/FastRecursiveMutex.cxx 
	src/Mutex.cxx 
	src/RecursiveMutexImpl.cxx 
	src/RecursiveMutex.cxx 
	src/Monitor.cxx 
	src/PoolExecutor.cxx 
	src/PriorityCondition.cxx 
	src/PriorityInheritanceMutex.cxx 
	src/PriorityMutex.cxx 
	src/PrioritySemaphore.cxx 
	src/Semaphore.cxx 
	src/SynchronousExecutor.cxx 
	src/Thread.cxx 
	src/ThreadedExecutor.cxx 
	src/ThreadImpl.cxx 
	src/ThreadLocalImpl.cxx 
	src/ThreadQueue.cxx 
	src/Time.cxx 
	src/ThreadOps.cxx
)
include_directories(
	.
)
add_library(zthread ${LIB_SRCS} ${LIB_HEADERS})

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set_target_properties(zthread PROPERTIES OUTPUT_NAME "zthread")
