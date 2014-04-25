inline
EventHandler::EventHandler ()
: reactor_ (0)
{
}
inline
EventHandler::~EventHandler ()
{

}
inline
int EventHandler::handle_input (NDK_HANDLE /* = NDK_INVALID_HANDLE*/) 
{
    return -1;
}
inline
int EventHandler::handle_output (NDK_HANDLE /* = NDK_INVALID_HANDLE*/) 
{
    return -1;
}
inline
int EventHandler::handle_exception (NDK_HANDLE /* = NDK_INVALID_HANDLE*/)
{
    return -1;
}
inline
int EventHandler::handle_timeout (const void *, const TimeValue &) 
{
    return -1;
}
inline
int EventHandler::handle_close (NDK_HANDLE , ReactorMask ) 
{
    return -1;
}
inline
int EventHandler::handle_close (const void *, ReactorMask ) 
{
    return -1;
}
inline
NDK_HANDLE EventHandler::handle () const
{
    return NDK_INVALID_HANDLE;
}
inline
void EventHandler::handle (NDK_HANDLE)
{
}
inline
Reactor *EventHandler::reactor () const
{
    return this->reactor_;
}
inline
void EventHandler::reactor (Reactor *r)
{
    this->reactor_  = r;
}

