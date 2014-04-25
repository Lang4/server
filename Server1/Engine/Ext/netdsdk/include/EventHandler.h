//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-06 15:48
 */
//========================================================================

#ifndef _EVENTHANDLER_H_
#define _EVENTHANDLER_H_
#include "Pre.h"

#include "Common.h"
#include "GlobalMacros.h"

class Reactor;
class TimeValue;
typedef unsigned long ReactorMask;
/**
 * @class EventHandler
 *
 * @brief 
 */
class EventHandler
{
public:
    enum
    {
	NULL_MASK      = 0x00000000,
	READ_MASK      = 0x00000001,
	WRITE_MASK     = 0x00000002,
	EXCEPT_MASK    = 0x00000004,
	ACCEPT_MASK    = 0x00000008,
	CONNECT_MASK   = 0x00000010,
	TIMER_MASK     = 0x00000020,
	SIGNAL_MASK    = 0x00000040,
	DONT_CALL      = 0x10000000,

	ALL_EVENTS_MASK= READ_MASK |
	    WRITE_MASK   |
	    EXCEPT_MASK  |
	    ACCEPT_MASK  |
	    CONNECT_MASK |
	    TIMER_MASK   |
	    SIGNAL_MASK,
    };
public:
    virtual ~EventHandler ();

    // The handle_close(NDK_HANDLE, ReactorMask) will be called if return -1. 
    virtual int handle_input (NDK_HANDLE handle = NDK_INVALID_HANDLE);
    
    // The handle_close(NDK_HANDLE, ReactorMask) will be called if return -1. 
    virtual int handle_output (NDK_HANDLE handle = NDK_INVALID_HANDLE);

    // The handle_close(NDK_HANDLE, ReactorMask) will be called if return -1. 
    virtual int handle_exception (NDK_HANDLE handle = NDK_INVALID_HANDLE);

    // The handle_close(const void *, ReactorMask) will be called if return -1.
    virtual int handle_timeout (const void *arg, const TimeValue &curent_time);

    //
    virtual int handle_close (NDK_HANDLE handle, ReactorMask mask);

    // 
    virtual int handle_close (const void *arg, ReactorMask mask);

    // Get the I/O handle
    virtual NDK_HANDLE handle () const;

    // Set the I/O handle
    virtual void handle (NDK_HANDLE handle);

    // Get the event demultiplexors.
    virtual void reactor (Reactor *r);

    // Set the event demultiplexors.
    virtual Reactor *reactor (void) const;
protected:
    // Force EventHandler to be an abstract base class.
    EventHandler ();

    Reactor *reactor_;
private:
};

#include "EventHandler.inl"
#include "Post.h"
#endif

