//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-01-11 19:53
 */
//========================================================================

#ifndef _INET_ADDR_H_
#define _INET_ADDR_H_
#include "Pre.h"

#include "Common.h"
#include "Trace.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>          // for strtol
#include <string.h>
#include <netdb.h>

#define MAX_HOSTNAME_LEN            255
/**
 * @class InetAddr
 *
 * @brief 
 */
class InetAddr
{
public:
    InetAddr ();

    // Copy constructor
    InetAddr (const InetAddr &addr);
    
    // Creates an InetAddr from a sockaddr_in structure.
    explicit InetAddr (const sockaddr_in *addr, int len);

    // 
    InetAddr (unsigned short port_number, const char *host_name);

    /** 
     * Create an InetAddr from the <address>, which can be 
     * "ip-number:port-number" (e.g., "tango.cs.wustl.edu:1234" or
     * "128.252.166.57:1234"). if there is no ':' in the <address> 
     * it is assumed to be a port number, with the IP address being 
     * INADDR_ANY.
     */
    explicit InetAddr (const char *address, int address_family = AF_INET);

    /**
     * Creates an InetAddr from a <port_number> and an Internet 
     * <ip_addr>, This method assumes that <port_number> and <ip_addr>
     * are in host byte order. 
     */
    explicit InetAddr (unsigned short port_number, 
	    unsigned int ip_addr = INADDR_ANY);

    ~InetAddr ();

    // Creates an InetAddr from a sockaddr_in structure.
    int set (const sockaddr_in *addr, int len);

    /**
     * Initializes an InetAddr from the addr, which can be
     * "ip-number:port-number" (e.g., "tango.cs.wustl.edu:1234" or
     * "128.252.166.57:1234").  If there is no ':' in the <address> it
     * is assumed to be a port number, with the IP address being
     * INADDR_ANY.
     */
    int set (const char addr[], int address_family = AF_INET);

    /**
     * Initializes an InetAddr from a port_number and an Internet ip_addr. 
     */
    int set (const unsigned short port_number, unsigned int inet_address = INADDR_ANY);

    /**
     * Initializes an InetAddr from a <port_number> and the remote <host_name>.
     */
    int set (const unsigned short port_number, const char *host_name, int address_family = AF_INET);

    // 
    void set_type (int address_family);

    // 
    void* get_addr (void) const;

    // Equel
    bool operator == (const InetAddr& addr) const;

    // 
    bool operator != (const InetAddr& addr) const;

    /**
     * Transform the current InetAddr address into string format. 
     * If <ipaddr_format> is non-0 this produces "ip-number:port-number" 
     * (e.g., "128.252.166.57:1234"), whereas if <ipaddr_format> is 0 
     * this produces "ip-name:port-number" (e.g., "tango.cs.wustl.edu:1234"). 
     * Returns -1 if the size of the <buffer> is too small, else 0.
     */
    int addr_to_string (char *str, size_t size, int ipaddr_format = 1) const;

    /** 
     * Initializes an InetAddr from the @arg address, which can be
     * "ip-addr:port-number" (e.g., "128.252.166.57:1234") If there
     * is no ':' in the <address> it is assumed to be a port number,
     * with the IP address being INADDR_ANY.
     */
    int string_to_addr (const char *address, int address_family = AF_INET);

    /**
     * Return the "dotted decimal" Internet address representation 
     * of the hostname storing it in the addr (which is assumed to 
     * be <addr_size> bytes long). 
     */
    const char *get_host_addr (char *addr, size_t addr_size) const;

    /**
     * Return the character representation of the name of the host,
     * storing it in the <hostname> (which is assumed to be 
     * <hostname_len> bytes long). If <hostname_len> is greater 
     * than 0 then <hostname> will be NUL-terminated even if -2 is 
     * returned, error if -1.
     */
    int get_host_name (char *hostname, size_t hostname_len) const;

    // Get the port number, converting it into host byte-order.
    unsigned short get_port_number (void) const;

    // Return the 4-byte IP address, converting it into host byte
    // order.
    unsigned int get_ip_address (void) const;

    // 
    bool is_any (void) const;
    
    // Return true if the IP address is IPv4/ loopback address.
    bool is_loopback (void) const;
    bool is_multicast () const;

    //
    int get_type (void) const;

    int ip_addr_size (void) const;

    //
    int get_addr_size (void) const;

    // static member
    static const InetAddr addr_any;
private:
    // Initialize underlying inet_addr_ to default values
    void reset (void);

    // 
private:
    // e.g., AF_UNIX, AF_INET, AF_SPIPE, etc.
    int  addr_type_;

    // Number of bytes in the address.
    //int  addr_size_;

    union
    {
	sockaddr_in	    in4_;
#if defined HAS_IPV6_
	sockaddr_in6        in6_;
#endif
    }inet_addr_;
};

#include "InetAddr.inl"
#include "Post.h"
#endif

