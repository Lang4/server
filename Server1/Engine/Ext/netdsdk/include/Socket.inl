inline
Socket::Socket ()
:handle_ (NDK_INVALID_HANDLE)
{
}
inline
Socket::~Socket ()
{
}
inline
int Socket::open (int sock_type, int protocol_family)
{
    NDK_HANDLE handle = ::socket (protocol_family, sock_type, 0);
    if (handle == NDK_INVALID_HANDLE) return -1;
    this->handle (handle);
    return 0;
}
inline
int Socket::close ()
{
    int ret_val = ::close (this->handle_);
    this->handle_ = NDK_INVALID_HANDLE;
    return ret_val;
}
inline
NDK_HANDLE Socket::handle () const
{
    return this->handle_;
}
inline
void Socket::handle (NDK_HANDLE handle)
{
    this->handle_ = handle;
}
inline
int Socket::get_local_addr (InetAddr &local_addr)
{
    int len = local_addr.get_addr_size ();
    sockaddr *addr = reinterpret_cast<sockaddr*> (local_addr.get_addr ());
    if (::getsockname (this->handle (),
		addr,
		reinterpret_cast<socklen_t*>(&len)) == -1)
	return -1;
    local_addr.set_type (addr->sa_family);
    return 0;
}
