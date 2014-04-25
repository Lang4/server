inline
InetAddr::InetAddr ()
{
    this->addr_type_ = -1;
    ::memset (&this->inet_addr_, 0, sizeof (this->inet_addr_));
}
inline
InetAddr::~InetAddr ()
{
}
inline
InetAddr::InetAddr (const InetAddr &addr)
{
    ::memset (&this->inet_addr_, 0, sizeof (this->inet_addr_)); 
    this->set_type (addr.get_type ());
}
inline
InetAddr::InetAddr (const sockaddr_in *addr, int len)
{
    this->reset ();
    this->set (addr, len);
}
inline
InetAddr::InetAddr (unsigned short port_number, const char *host_name)
{
    this->reset ();
    this->set (port_number, host_name);
}
inline
InetAddr::InetAddr (unsigned short port_number, unsigned int ip_addr/* = INADDR_ANY*/)
{
    this->reset ();
    this->set (port_number, ip_addr);
}
inline
InetAddr::InetAddr (const char *address, int address_family/* = AF_INET*/)
{
    this->reset ();
    this->set (address, address_family);
}
inline
void InetAddr::reset () 
{
    ::memset (&this->inet_addr_, 0, sizeof (this->inet_addr_));
    if (this->get_type() == AF_INET)
    {
	this->inet_addr_.in4_.sin_family = AF_INET;
    }
}
inline
unsigned short InetAddr::get_port_number () const
{
    if (this->get_type () == AF_INET)
	return ntohs (this->inet_addr_.in4_.sin_port);
    return 0;
}
inline
int InetAddr::get_type () const
{
    return this->addr_type_;
}
inline
void InetAddr::set_type (int address_family)
{
    this->addr_type_ = address_family;
}
inline
int InetAddr::ip_addr_size () const
{
    return static_cast<int>(sizeof (this->inet_addr_.in4_.sin_addr));
}
inline
int InetAddr::get_addr_size () const
{
    if (this->get_type () == AF_INET)
	return static_cast<int>(sizeof (this->inet_addr_.in4_));
    return static_cast<int>(sizeof (this->inet_addr_.in4_));
}
inline
bool InetAddr::is_any () const
{
    return (this->inet_addr_.in4_.sin_addr.s_addr == INADDR_ANY);
}
inline
bool InetAddr::is_loopback () const
{
    // IPv4
    // RFC 3330 defines loopback as any address with 127.x.x.x
    return ((this->get_ip_address () & 0XFF000000) 
	    == (INADDR_LOOPBACK & 0XFF000000));
}
inline
bool InetAddr::is_multicast () const
{
    // IPv4
    return this->inet_addr_.in4_.sin_addr.s_addr >= 0xE0000000 &&  // 224.0.0.0
	this->inet_addr_.in4_.sin_addr.s_addr <= 0xEFFFFFFF;   // 239.255.255.255
}
inline
void *InetAddr::get_addr () const
{
    return (void*)&this->inet_addr_;
}
inline
int InetAddr::set (const sockaddr_in *addr, int len)
{
    if (addr->sin_family == AF_INET)
    {
	int addr_size = static_cast<int>(sizeof (this->inet_addr_.in4_));
	if (len >= addr_size)
	{
	    ::memcpy (&this->inet_addr_.in4_, addr, addr_size);
	    this->addr_type_ = AF_INET;
	    //this->addr_size_ = addr_size;
	    return 0;
	}
    }
    return -1;
}
inline
int InetAddr::set (const char *addr, int address_family/* = AF_INET*/)
{
    return this->string_to_addr (addr, address_family);
}
inline
int InetAddr::set (const unsigned short port_number, 
	unsigned int inet_address/* = INADDR_ANY*/)
{
    this->addr_type_ = AF_INET;
    this->inet_addr_.in4_.sin_family = AF_INET;
    this->inet_addr_.in4_.sin_port   = htons (port_number);
    //this->inet_addr_.in4_.sin_len    = sizeof (this->inet_addr_.in4);
    unsigned int ip4 = htonl (inet_address);
    ::memcpy (&this->inet_addr_.in4_.sin_addr, &ip4, sizeof (unsigned int));
    return 0;
}

