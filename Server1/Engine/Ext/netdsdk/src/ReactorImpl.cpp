#include "ReactorImpl.h"
#include "Trace.h"

ReactorImpl::ReactorImpl ()
{
    TRACE ("ReactorImpl");
}
ReactorImpl::~ReactorImpl ()
{
    TRACE ("ReactorImpl");
}
// ---------------------------------------------------------------------------
NotificationBuffer::NotificationBuffer ()
{
    TRACE ("NotificationBuffer");
}
NotificationBuffer::NotificationBuffer (EventHandler *eh,
	ReactorMask mask)
: eh_ (eh)
, mask_ (mask)
{
    TRACE ("NotificationBuffer");
}
NotificationBuffer::~NotificationBuffer ()
{
    TRACE ("NotificationBuffer");
}

