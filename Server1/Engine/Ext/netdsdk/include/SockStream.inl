inline
SockStream::SockStream ()
{
}
inline
SockStream::SockStream (NDK_HANDLE handle)
{
    this->handle (handle);
}
inline
SockStream::~SockStream ()
{
}
inline
int SockStream::recv (void *buff, 
	size_t len, 
	const TimeValue *timeout/* = 0*/) 
{
    return NDK::recv (this->handle (), buff, len, timeout);
}
inline
int SockStream::recv (void *buff, 
	size_t len, 
	int flags, 
	const TimeValue *timeout/* = 0*/) 
{
    return NDK::recv (this->handle (), buff, len, flags, timeout);
}
inline
int SockStream::recv_n (void *buff, 
	size_t len, 
	const TimeValue *timeout/* = 0*/) 
{
    return NDK::recv_n (this->handle (), buff, len, timeout);
}
inline
int SockStream::recv_n (void *buff, 
	size_t len, 
	int flags, 
	const TimeValue *timeout/* = 0*/) 
{
    return NDK::recv_n (this->handle (), buff, len, flags, timeout);
}
inline
int SockStream::recvv (iovec iov[], 
	size_t count, 
	const TimeValue *timeout/* = 0*/) 
{
    return NDK::recvv (this->handle (), iov, count, timeout);
}
inline
int SockStream::recvv_n (iovec iov[], 
	size_t count, 
	const TimeValue *timeout/* = 0*/) 
{
    return NDK::recvv_n (this->handle (), iov, count, timeout);
}
inline
int SockStream::send (const void *buff, 
	size_t len, 
	const TimeValue *timeout/* = 0*/) 
{
    return NDK::send (this->handle (), buff, len, timeout);
}
inline
int SockStream::send (const void *buff, 
	size_t len, 
	int flags, 
	const TimeValue *timeout/* = 0*/) 
{
    return NDK::send (this->handle (), buff, len, flags, timeout);
}
inline
int SockStream::send_n (const void *buff, 
	size_t len, 
	const TimeValue *timeout/* = 0*/) 
{
    return NDK::send_n (this->handle (), buff, len, timeout);
}
inline
int SockStream::send_n (const void *buff, 
	size_t len, 
	int flags, 
	const TimeValue *timeout/* = 0*/) 
{
    return NDK::send_n (this->handle (), buff, len, flags, timeout);
}
inline
int SockStream::sendv (const iovec iov[], 
	size_t count, 
	const TimeValue *timeout/* = 0*/) 
{
    return NDK::sendv (this->handle (), iov, count, timeout);
}
inline
int SockStream::sendv_n (const iovec iov[], 
	size_t count, 
	const TimeValue *timeout/* = 0*/) 
{
    return NDK::sendv_n (this->handle (), iov, count, timeout);
}


