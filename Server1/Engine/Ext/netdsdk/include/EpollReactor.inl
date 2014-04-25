inline
EpollReactorHandlerRepository::EpollReactorHandlerRepository ()
: max_size_ (0)
, handlers_ (0)
{

}
inline
int EpollReactorHandlerRepository::is_invalid_handle (NDK_HANDLE handle) const
{
    if (handle < 0 || handle >= this->max_size_)
	return 1;
    return 0;
}
inline
int EpollReactorHandlerRepository::handle_in_range (NDK_HANDLE handle) const
{
    if (handle >= 0 && handle < this->max_size_)
	return 1;
    return 0;
}
inline
void EpollReactorHandlerRepository::mask (NDK_HANDLE handle,
	ReactorMask mask)
{
    if (this->handle_in_range (handle))
	this->handlers_[handle].mask = mask;
}
inline
ReactorMask EpollReactorHandlerRepository::mask (NDK_HANDLE handle)
{
    if (this->handle_in_range (handle))
	return this->handlers_[handle].mask;
    return EventHandler::NULL_MASK;
}
//----------------------------------------------------------------------------
inline
EpollEventTuple::EpollEventTuple ()
: event_handler (0)
, mask (EventHandler::NULL_MASK)
{
}
//----------------------------------------------------------------------------
inline
int EpollReactor::upcall (EventHandler *event_handler,
	int (EventHandler::*callback)(NDK_HANDLE),
	NDK_HANDLE handle)
{
    return (event_handler->*callback)(handle);
}
inline
int EpollReactor::notify (EventHandler *event_handler/* = 0*/,
	ReactorMask mask/* = EventHandler::EXCEPT_MASK*/,
	const TimeValue *timeout/* = 0*/)
{
    return this->notify_handler_->notify (event_handler, 
	    mask, 
	    timeout);
}
//---------------------------------------------------------------------------------
inline
EpollReactor::TokenGuard::TokenGuard (ReactorToken &token)
: token_(token)
, owner_ (0)
{
}
inline
EpollReactor::TokenGuard::~TokenGuard ()
{
    this->release_token ();
}
inline
void EpollReactor::TokenGuard::release_token ()
{
    if (this->owner_ == 1)
    {
	this->token_.release ();
	this->owner_ = 0;
    }
}
inline
int EpollReactor::TokenGuard::is_owner ()
{
    return this->owner_;
}

