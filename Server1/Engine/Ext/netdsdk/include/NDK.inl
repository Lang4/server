inline
int NDK::handle_read_ready (NDK_HANDLE handle, 
	const TimeValue *timeout/* = 0*/) 
{
    return NDK::handle_ready (handle, 1, 0, 0, timeout);
}
inline
int NDK::handle_write_ready (NDK_HANDLE handle, 
	const TimeValue *timeout/* = 0*/)
{
    return NDK::handle_ready (handle, 0, 1, 0, timeout);
}
inline
int NDK::handle_exception_ready (NDK_HANDLE handle, 
	const TimeValue *timeout/* = 0*/)
{
    return NDK::handle_ready (handle, 0, 0, 1, timeout);
}
inline
int NDK::sleep (const TimeValue *timeout)
{
    return NDK::handle_ready (0, 0, 0, 0, timeout);
}
inline
TimeValue NDK::gettimeofday ()
{
    struct timeval tv;
    ::gettimeofday (&tv, 0);
    return TimeValue (tv);
}
inline
int NDK::set_non_block_mode (NDK_HANDLE handle)
{
    int flag = ::fcntl (handle, F_GETFL, 0);
    if (NDK_BIT_ENABLED (flag, O_NONBLOCK)) // already nonblock
	return 0;
    return ::fcntl (handle, F_SETFL, flag | O_NONBLOCK);
}
inline
int NDK::set_block_mode (NDK_HANDLE handle)
{
    int flag = ::fcntl (handle, F_GETFL, 0);
    if (NDK_BIT_ENABLED (flag, O_NONBLOCK)) // nonblock
    {
	NDK_CLR_BITS (flag, O_NONBLOCK);
	return fcntl (handle, F_SETFL, flag);
    }
    return 0; 
}
inline
int NDK::record_and_set_non_block_mode (NDK_HANDLE handle, int &flag)
{
    flag = ::fcntl (handle, F_GETFL, 0);
    if (NDK_BIT_ENABLED (flag, O_NONBLOCK)) // already nonblock
	return 0;
    return ::fcntl (handle, F_SETFL, flag | O_NONBLOCK);
}
inline
int NDK::restore_non_block_mode (NDK_HANDLE handle, int flag)
{
    if (NDK_BIT_ENABLED (flag, O_NONBLOCK)) // nonblock original
	return 0;
    // block original and clear it
    //int val = flag;
    //val &= ~O_NONBLOCK;
    return ::fcntl (handle, F_SETFL, (flag & (~O_NONBLOCK)));
}
inline
int NDK::recv (NDK_HANDLE handle, void *buff, size_t len, 
	const TimeValue *timeout)
{
    return NDK::recv (handle, buff, len, 0, timeout);
}
inline
int NDK::recv_n (NDK_HANDLE handle, void *buff, size_t len, 
	const TimeValue *timeout)
{
    return NDK::recv_n (handle, buff, len, 0, timeout);
}
inline
int NDK::send (NDK_HANDLE handle, const void *buff, size_t len,
       const TimeValue *timeout)
{
    return NDK::send (handle, buff, len, 0, timeout);
}
inline
int NDK::send_n (NDK_HANDLE handle, const void *buff, size_t len, 
	const TimeValue *timeout)
{
    return NDK::send_n (handle, buff, len, 0, timeout);
}

