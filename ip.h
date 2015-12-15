/*

 Copyright (c) 2014, Charles Smith
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this
      list of conditions and the following disclaimer in the documentation and/or
      other materials provided with the distribution.
    * Neither the name of Vallona Networks nor the names of its contributors
      may be used to endorse or promote products derived from this software without
      specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#ifndef _IP_H
#define _IP_H

#include <iostream>
#include <cstring>
#include <string>
#include <list>
#include <inttypes.h>

/* Socket handling includes */
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/select.h>
#include <ifaddrs.h>
#include <netdb.h>

#include <netax25/axlib.h>
#include <netax25/ax25.h>
#include <netax25/axconfig.h>
#include <linux/if_ether.h>



#include "screen.h"
#include "sarflags.h"
#include "timestamp.h"
#include "timer.h"

using namespace std;
using namespace timer_group;
using namespace saratoga;

//timer c;

// Return greatest of 2 fd numbers
template<typename T>
inline T FDMAX(const T& x, const T& y)
{
	return (x > y) ? x : y;
}

namespace sarnet
{

// We don't do jumbo frames
#ifndef JUMBO
#undef JUMBO
#endif	// JUMBO

/*
 **********************************************************************
 * IP
 **********************************************************************
 */

// Multicast direction
enum mcast_dir
{
	MCAST_IN = 0x00,
	MCAST_OUT = 0x01
};

union in_storage
{
	struct in_addr		v4;
	struct in6_addr		v6;
};

// typedef union inv4or6	IPVER;

class ip
{
private:
	const int OTHER	= -1;
	union in_storage	_ip; // The IP in_addr or in6_addr data
	int			_family; // What are V4 or V6 or OTHER
public:
	// Blank placeholder
	ip() { bzero(&_ip, sizeof(union in_storage)); _family = OTHER; };
	// An IP Address filled in
	ip(string addr);
	// A blank placeholder for a certain protocol
	ip(int protocol) { 
		_family = protocol; 
		if (_family == AF_INET)
			bzero(&_ip.v4, sizeof(struct in_addr)); 
		else if (_family == AF_INET6)
			bzero(&_ip.v6, sizeof(struct in6_addr));
		else
			bzero(&_ip, sizeof(union in_storage));
		return;
	};

	// Given a pointer to another ip create an identical one
	ip(ip *p) {
		_family = p->family();
		bzero(&_ip, sizeof(union in_storage));
		if (_family == AF_INET)
			bcopy(&p->_ip.v4, &_ip.v4, sizeof(struct in_addr));
		else if (_family == AF_INET6)
			bcopy(&p->_ip.v6, &_ip.v6, sizeof(struct in6_addr));
	}
	
	~ip() { 
		bzero(&_ip, sizeof(union in_storage)); _family = OTHER; 
	};

	void zap() {
		bzero(&_ip, sizeof(union in_storage)); _family = OTHER; 
	}

	ip(struct in_addr* a)
	{
		_family = AF_INET;
		bzero(&_ip, sizeof(union in_storage));
		bcopy(a, &_ip.v4, sizeof(struct in_addr));
	}

	ip(struct in6_addr* a)
	{
		_family = AF_INET6;
		bzero(&_ip, sizeof(union in_storage));
		bcopy(a, &_ip.v6, sizeof(struct in6_addr));
	}

	// Given a soxket get me the ip address info
	ip(struct sockaddr_storage *s)
	{
		struct sockaddr_in	*pv4;
		struct sockaddr_in6	*pv6;
		switch(s->ss_family)
		{
		case AF_INET:
			_family = AF_INET;
			pv4 = (struct sockaddr_in *) s;
			bzero(&_ip, sizeof(union in_storage));
			bcopy(&(pv4->sin_addr), &_ip.v4, sizeof(struct in_addr));
			return;
		case AF_INET6:
			_family = AF_INET6;
			pv6 = (struct sockaddr_in6 *) s;
			bzero(&_ip, sizeof(union in_storage));
			bcopy(&(pv6->sin6_addr), &_ip.v6, sizeof(struct in6_addr));
			return;
		default:
			_family = OTHER;
			bzero(&_ip, sizeof(union in_storage));
			return;
		}
	}
	
	// Assignment via x = y;
	ip& operator =(const ip& addr) {
		_family = addr._family;
		bzero(&_ip, sizeof(union in_storage));
		bcopy(&addr._ip, &_ip, sizeof(union in_storage));
		return(*this);
	};

	// True is ip x == y
	bool operator ==(const ip &rhs)
	{
		if (_family != rhs._family)
			return(false);
		if (_family == AF_INET)
		{
			if (memcmp(&_ip, &rhs._ip.v4, sizeof(struct in_addr)) != 0)
				return(false);
			return(true);
		}
		if (_family == AF_INET6)
		{
			if (memcmp(&_ip, &rhs._ip.v6, sizeof(struct in6_addr)) != 0)
				return(false);
			return(true);
		}
		if (_family == OTHER)
			return(false);
		return(false);
	};

	bool operator ==(const string s)
	{
		ip	tmp(s);
		
		if (tmp._family != AF_INET && tmp._family != AF_INET6)
			return(false);

		if (_family != tmp._family)
			return(false);
		if (_family == AF_INET)
		{
			if (memcmp(&_ip, &tmp._ip.v4, sizeof(struct in_addr)) != 0)
				return(false);
			return(true);
		}
		if (_family == AF_INET6)
		{
			if (memcmp(&_ip, &tmp._ip.v6, sizeof(struct in6_addr)) != 0)
				return(false);
			return(true);
		}
		return(false);
	}

	// True is ip x != y
	bool operator !=(const ip &rhs)
	{
		if (_family != rhs._family)
			return(true);
		if (_family == AF_INET)
		{
			if (memcmp(&_ip, &rhs._ip.v4, sizeof(struct in_addr)) != 0)
				return(true);
			return(false);
		}
		if (_family == AF_INET6)
		{
			if (memcmp(&_ip, &rhs._ip.v6, sizeof(struct in6_addr)) != 0)
				return(true);
			return(false);
		}
		if (_family == OTHER)
			return(true);
		return(true);
	}

	bool operator !=(const string s)
	{
		ip	tmp(s);
		
		if (tmp._family != AF_INET && tmp._family != AF_INET6)
			return(true);

		if (_family != tmp._family)
			return(true);
		if (_family == AF_INET)
		{
			if (memcmp(&_ip, &tmp._ip.v4, sizeof(struct in_addr)) != 0)
				return(true);
			return(false);
		}
		if (_family == AF_INET6)
		{
			if (memcmp(&_ip, &tmp._ip.v6, sizeof(struct in6_addr)) != 0)
				return(true);
			return(false);
		}
		return(true);
	}

	// What are we v4 or v6
	int family() { return(_family); };
	bool isother() { return(_family == OTHER); };
	bool isv4() { return(_family == AF_INET); };
	bool isv6() { return(_family == AF_INET6); };
	struct in_addr *addr4() { return &_ip.v4; };
	struct in6_addr *addr6() { return &_ip.v6; };

	string	straddr();
	string	print();
};

/*
 *********************************************************************************
 * INTERFACES lo0, eth0 ...
 *********************************************************************************
 */

class netiface
{
private:
	ip	*_ifip;	// IP Address
	int	_ifindex;	// What index # it is used for V6 multicast
	string	_ifname;	// What interface name it is on eg. eth0
	int	_mtu; 		// Ethernet MTU of the interface
	char	_mac[6];	// What is the MAC address
public:
	netiface(string ipaddr, string ifname) { 
		extern char *if_nametomac(char*, string);
		extern int if_mtu(string);

		char s[IF_NAMESIZE];

		_ifip = new ip(ipaddr); 
		strcpy(s, ifname.c_str());
		_ifindex = if_nametoindex(s);
		_ifname = ifname;
		if_nametomac(_mac, ifname);
		_mtu = if_mtu(ifname);
	};

	~netiface() {};

	string ifname() { return _ifname; };
	int	ifindex() { return _ifindex; };	
	ip *ifip() { return(_ifip); };
	string mac();
	int	mtu() { return _mtu; };

	bool ifmatch(string s) { return (s == _ifname); };
	bool ifmatch(ip *myip) { return (*myip == *_ifip); };
	bool familymatch(int fam) { return (fam == _ifip->family()); };

	string print();
};

// System list of network interfaces
class netifaces
{
private:
	std::list<netiface *>	_iface; // List of local IP Addresses
					// associated to interfaces
public:
	netifaces();
	~netifaces() { this->zap(); };

	std::list<netiface *>::iterator begin() { return(_iface.begin()); };
	std::list<netiface *>::iterator end() { return(_iface.end()); };

	void zap() { 
		while (! _iface.empty()) 
			_iface.pop_front(); 
	};
	
	// Is the address given in our local iface list
	bool islocal(string s)
	{
		ip	tmp(s);
		for (std::list<netiface *>::iterator i = _iface.begin(); i != _iface.end(); i++)
		{
			if ((*i)->ifmatch(&tmp))
				return(true);
		}
		return(false);
	}

	// Return first entry in list for AF_INET or AF_INET6
	// ignoring loopbacks, but as a last resort send the loopback
	netiface *first(int fam)
	{
		for (std::list<netiface *>::iterator i = _iface.begin(); i != _iface.end(); i++)
		{
			if (!(*i)->familymatch(fam))
				continue;
			ip	tmp((*i)->ifip());
			if (tmp != "127.0.0.1" && tmp != "::1")
				return((*i));
		}
		// No active interfaces so lets send back the loopback
		for (std::list<netiface *>::iterator i = _iface.begin(); i != _iface.end(); i++)
		{
			if (!(*i)->familymatch(fam))
				continue;
			ip	tmp((*i)->ifip());
			if (tmp == "127.0.0.1" || tmp == "::1")
				return((*i));
		}
		return(nullptr);
	}

	string print() {
		string s;
		for (std::list<netiface *>::iterator i = _iface.begin(); i != _iface.end(); i++)
		{
			s += (*i)->print();
			s += "\n";
		}
		return(s);
	};

	netiface *ipmatch(string ipaddr)
	{
		for (std::list<netiface *>::iterator i = _iface.begin(); i != _iface.end(); i++)
		{
			ip	tmp((*i)->ifip());
			if (tmp.straddr() == ipaddr)
				return((*i));
		}
		return(nullptr);
	}
};

/*
 **********************************************************************
 * UDP
 **********************************************************************
 */

class udp
{
private:
	struct sockaddr_storage	_sa; // sockaddr info and it is big enough to hold v4 & v6 info
	int	_fd; // file descriptor
	std::list<saratoga::buffer>	_buf; // Frames queued to send
	bool	_readytotx;	// Sets FD_SET() or FD_CLR() for tx
	timer_group::timer	*_delay;	// Used to implement a delay between sending frames

	static const ssize_t		_jumbosize = 8192; // Jumbo ethernet frame size
	static const ssize_t		_ethsize = 1500; // Normal ethernet frame size
	static const ssize_t		_udpheader = 8; // Size of the udp header
	static const ssize_t		_v4header = 20; // Max Size of an ipv4 header
	static const ssize_t		_v6header = 40; // Size of an ipv6 header
	const ssize_t				_maxbuff = _ethsize;

	// We will ALWAYS send or recv a minumum of 4 bytes as this is the size of
	// the saratoga flags header so set the watermarks to this
	const int		_rcvlowat = 4;	
	const int		_sndlowat = 4;	

	ssize_t _maxframesize() {
		if (this->family() == AF_INET)
			return (_maxbuff - _v4header - _udpheader);
		else
			return (_maxbuff - _v6header - _udpheader);
	}; 
public:

	udp() { _fd = -1; 
		_buf.empty(); 
		bzero(&_sa, sizeof (struct sockaddr_storage)); 
		_readytotx = false;
		_delay = new timer_group::timer(); };

	// Create a socket to a particular address and port
	udp(string addr, int port);

	udp(sarnet::ip *addr, int port);

	// Create & listen (bind) to inbound sockets on a port
	udp(int proto, int port);

	udp(struct sockaddr_storage *p);

	// Use for multicast to bind to existing input or output socket	
	udp(enum mcast_dir direction, udp *p, string addr, int port);

	~udp() { this->zap(); };

	void zap() {
		// Clear the buffers
		_buf.clear();
		if (_fd > 2)
		{
			shutdown(_fd, SHUT_RDWR);
			close(_fd);
			_fd = -1;
			bzero(&_sa, sizeof(struct sockaddr_storage));
			_readytotx = false;;
		}
	}

	// Copy constructor
	udp(udp *b) {
		_fd = b->fd();
		_readytotx = b->_readytotx;
		_buf = b->_buf;
		bzero(&_sa, sizeof(struct sockaddr_storage));
		bcopy(&_sa, b->addr(), sizeof(struct sockaddr_storage));
	}
		
	// Copy a socket assignment
	udp&	operator=(udp& s1) {
		_fd = s1.fd();
		_readytotx = s1.ready();
		bzero(&_sa, sizeof(struct sockaddr_storage));
		bcopy(&_sa, s1.addr(), sizeof(struct sockaddr_storage));
		return(*this);
	};

	bool	operator==(const udp &s)
	{
		return ((_fd == s._fd) &&
			(_readytotx == s._readytotx) &&
//			(_buf == s._buf) &&
			(memcmp(&_sa, &s._sa, 
				sizeof(struct sockaddr_storage)) == 0));
	};

	ssize_t	framesize() { return _maxframesize(); };

	// What are we v4 or v6
	int family() { 
		struct sockaddr *s = (struct sockaddr *) &_sa;
		return(s->sa_family); 
	};

	// Handle integer setsockopt
	bool setopt(int option, void *optval, socklen_t optsize);

	// Is the option set OK ?
	bool getopt(int option, void *optval, socklen_t *optlen);

	bool set(string addr, int port);

	// Add a buffer to the list to be sent
	virtual ssize_t tx(char *buf, size_t buflen) {
		// Add the frame to the end of the list of buffers
		// alloc the memory for it and then push it
		saratoga::buffer *tmp = new saratoga::buffer(buf, buflen);
		_buf.push_back(*tmp); 
	
		// make FD_SET ready() test to true
		// that way we can call send() when ready
		_readytotx = true;
		// delete tmp;
		return(buflen);
	}

	// Receive a buffer, return # chars sent - 
	// You catch the error if <0 
	virtual ssize_t	rx(char *, sarnet::ip *);

	// Socket #
	int port();

	// fd
	int	fd() { return(_fd); };

	// Actually transmit the frames in the buffers
	virtual int send();

	// Are we ready to tx (controls select()
	bool ready() { return _readytotx; };
	bool ready(bool x) { _readytotx = x; return _readytotx; };

	// Printable IP Address
	string straddr();

	// Printable Port Number
	string strport();

	struct in_addr *addr4() { 
		struct sockaddr_in *p = (struct sockaddr_in *) &_sa;
		return(&p->sin_addr); 
	}

	struct in6_addr *addr6() 
	{
		struct sockaddr_in6 *p = (struct sockaddr_in6 *) &_sa;
		return(&p->sin6_addr);
	}

	struct sockaddr_storage *addr() { return &_sa; };

	struct sockaddr *saptr() { return (struct sockaddr *) &_sa; };

	string print();
};

// List of currently opened udp peers
class peers
{
private:
	const size_t	_max = 100; // Maximum # of sockets
	std::list<udp>	_peers; // List of open peers
	bool		_fdchange; // We have added/removed a peer used for select()
public:
	peers() { };
	~peers() { this->zap(); };

	sarnet::udp	*add(sarnet::udp *p); // Previos opened socket put it in list
	sarnet::udp	*add(string ipaddr, int port); // Create & open a peer and put it in list
	sarnet::udp	*add(sarnet::ip *addr, int port); // Create & open a peer and put it in list
	void	remove(int fd);	// Remove the peer from the list with matching fd
	void	remove(string ipaddr, int port);	// Remove the peer from the list with matching ip & port

	void zap() {
		_fdchange = true;
		// Close down all of the peer sockets
		for (std::list<udp>::iterator p = this->begin(); p != this->end(); p++)
			p->zap(); 
		// Clear the list
		_peers.clear();
	};
	
	std::list<udp>::iterator begin() { return(_peers.begin()); };
	std::list<udp>::iterator end() { return(_peers.end()); };

	// What is the largest fd in the list (for select())
	int	largestfd();

	// Is the fd in the current list
	bool	exists(int fd);

	bool	fdchange() { return(_fdchange); };
	bool	fdchange(bool state) { _fdchange = state; return(_fdchange); };

	// Does the peer match an IP address
	sarnet::udp	*match(ip *host);
	sarnet::udp	*match(string addr);

	// Print all the open sockets info
	string print();
};



class ax25 : public udp
{
private:

	//AX25lib
	// AX25
	static struct full_sockaddr_ax25 ax25src;
	static char* ax25portcall;
	static int ax25slen;
	static char* dev;


	struct full_sockaddr_ax25 ax25dest;
	char *ax25destcall;
	int ax25dlen;

	struct sockaddr_storage	_sa; // sockaddr info and it is big enough to hold v4 & v6 info
	int	_fd; // file descriptor

	std::list<saratoga::buffer>	_bufax25; // Frames queued to send
	bool	_readytotx;	// Sets FD_SET() or FD_CLR() for tx
	timer_group::timer	*_delay;	// Used to implement a delay between sending frames

	static constexpr char* ax25port = "spacelink" ;
	static constexpr char* ax25multidestcall = "ALL";
	static const int proto = ETH_P_AX25;
	static const ssize_t		_jumbosize = 8192; // Jumbo ethernet frame size
	static const ssize_t		_ethsize = 1500; // Normal ethernet frame size
	static const ssize_t		_udpheader = 8; // Size of the udp header
	static const ssize_t		_v4header = 20; // Max Size of an ipv4 header
	static const ssize_t		_v6header = 40; // Size of an ipv6 header
	static const ssize_t 		_ax25size = 255;
	const ssize_t				_maxbuff = _ax25size;

	// We will ALWAYS send or recv a minumum of 4 bytes as this is the size of
	// the saratoga flags header so set the watermarks to this
	const int		_rcvlowat = 4;
	const int		_sndlowat = 4;

	ssize_t _maxframesize() {
			return (_maxbuff - _v4header - _udpheader);
	};
public:

	static int ax25insock;
	static int ax25outsock;
	static bool ax25available;
	static char* ax25srcaddress;


	static int initax25();
	ax25(char* dest);

	~ax25() { this->zap(); };

	void zap() {
		// Clear the buffers
		_bufax25.clear();
	}

	ssize_t	framesize() { return _maxframesize(); };

	// What are we v4 or v6
	int family() {
		return AF_INET;
	};

	// Handle integer setsockopt
	bool setopt(int option, void *optval, socklen_t optsize);

	// Is the option set OK ?
	bool getopt(int option, void *optval, socklen_t *optlen);

	bool set(string addr, int port);

	// Add a buffer to the list to be sent
	virtual ssize_t tx(char *buf, size_t buflen) override {
		// Add the frame to the end of the list of buffers
		// alloc the memory for it and then push it
		saratoga::buffer *tmp = new saratoga::buffer(buf, buflen);
		_bufax25.push_back(*tmp);

		// make FD_SET ready() test to true
		// that way we can call send() when ready
		_readytotx = true;
		// delete tmp;
		return(buflen);
	}



	// Receive a buffer, return # chars sent -
	// You catch the error if <0
	ssize_t	rx(char* b, sarnet::ip *from);

	// Socket #
	int port();

	// fd
	int	fd() { return(_fd); };

	// Actually transmit the frames in the buffers
	virtual int send() override;

	// Are we ready to tx (controls select()
	bool ready() { return _readytotx; };
	bool ready(bool x) { _readytotx = x; return _readytotx; };

	// Printable IP Address
	string straddr();

	// Printable Port Number
	string strport();

	struct in_addr *addr4() {
		struct sockaddr_in *p = (struct sockaddr_in *) &_sa;
		return(&p->sin_addr);
	}

	struct in6_addr *addr6()
	{
		struct sockaddr_in6 *p = (struct sockaddr_in6 *) &_sa;
		return(&p->sin6_addr);
	}

	struct sockaddr_storage *addr() { return &_sa; };

	struct sockaddr *saptr() { return (struct sockaddr *) &_sa; };

	string print();
};



}; // Namespace sarnet




#endif // _IP_H
