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
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <string>
#include "screen.h"
#include "ip.h"
#include "globals.h"
#include <sys/socket.h>
#include <typeinfo>

using namespace std;

// #include "timestamp.h"
/*
 * Everything to do with ip and udp networking basics go here under sarnet namespace
 */
namespace sarnet
{


// Given a string create IP
ip::ip(string addr)
{

	char *addrs = new char[addr.length() + 1];
	strcpy(addrs, addr.c_str());
	if (addr.find(":") != std::string::npos){
		_family = AF_INET6;
		saratoga::scr.debug(7,"ip::ip(string): New ipv6 family ip object, addr="+addr);
	}
	else if (addr.find(".") != std::string::npos){
		_family = AF_INET;
		saratoga::scr.debug(7,"ip::ip(string): New ipv4 family ip object, addr="+addr);
	}
	else
	{
		saratoga::scr.debug(7,"ip::ip(string): New ax25 family ip object, addr="+addr);
		_family = AF_AX25;
		bzero(&_ip, sizeof(union in_storage));
		//delete addrs;
		_ax25addr = addr;
		return;
	}
	/* Convert existing string to in_addr */
	if (inet_pton(_family, addrs, &_ip) != 1)
	{
		saratoga::scr.error("ip(%s):Invalid IP Address\n", addrs);
		_family = OTHER;
		bzero(&_ip, sizeof(union in_storage));
		delete addrs;
	}
	return;
}

string
ip::print()
{
	// static string ret;
	string ret;
	char	*s;

	if (this == nullptr)
		return("");
	if (_family == OTHER)
		return("OTHER");
	if (_family == AF_AX25)
		return _ax25addr;
	if (_family == AF_INET)
	{
		s = new char [INET_ADDRSTRLEN];
		inet_ntop(_family, &_ip.v4, s, INET_ADDRSTRLEN);
		ret = s;
		delete s;
		return(ret);
	}
	if (_family == AF_INET6)
	{
		s = new char [INET6_ADDRSTRLEN];
		inet_ntop(_family, &_ip.v6, s, INET6_ADDRSTRLEN);
		ret = s;
		delete s;
		return(ret);
	}
	return("INVALID FAMILY");
}

// Return IP Address in string or nothing
string
ip::straddr()
{
	// static string ret;
	string ret;
	char	*s;

	if (this == nullptr)
		return("");
	if (_family == OTHER)
		return("");
	if (_family == AF_AX25)
		return _ax25addr;
	if (_family == AF_INET)
	{
		s = new char [INET_ADDRSTRLEN];
		inet_ntop(_family, &_ip.v4, s, INET_ADDRSTRLEN);
		ret = s;
		delete s;
		return(ret);
	}
	if (_family == AF_INET6)
	{
		s = new char [INET6_ADDRSTRLEN];
		inet_ntop(_family, &_ip.v6, s, INET6_ADDRSTRLEN);
		ret = s;
		delete s;
		return(ret);
	}
	return("");
}

// List of local IP Addresses for this machine
netifaces::netifaces()
{
	string	s;

	// struct addrinfo	hint, *res, *p;
	struct ifaddrs *res, *p;
	int	status;
	char	ipstr[INET6_ADDRSTRLEN];

	if ((status = getifaddrs(&res)) != 0)	
	{
		saratoga::scr.perror(errno, "Cannot get local machine IP Addresses");
		exit(1);
	}

	// Loop through my ip addresses looking for V4 & V6 ones only
	for (p = res; p != nullptr; p = p->ifa_next)
	{
		void *addr;
		string tmpifname = p->ifa_name;
		if (p->ifa_addr->sa_family == AF_INET)
		{
			struct sockaddr_in *ipv4 = (struct sockaddr_in *) p->ifa_addr;
			addr = &(ipv4->sin_addr);
		}
		else if (p->ifa_addr->sa_family == AF_INET6)
		{
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) p->ifa_addr;
			addr = &(ipv6->sin6_addr);
		}
		else
			continue;
		// Find the index interface associated with this ip address
		inet_ntop(p->ifa_addr->sa_family, addr, ipstr, INET6_ADDRSTRLEN);
		string tmpipstr = ipstr;
		netiface *tmpiface = new netiface(tmpipstr, tmpifname);
		_iface.push_back(tmpiface);
	}
	saratoga::scr.debug(5, "netifaces::netifaces(): LOCAL INTERFACES: %s", this->print().c_str());
	freeifaddrs(res);
}

// Return string with : delimeted MAC address
string
netiface::mac()
{
	string s; 
	char str[128];
	string sip = _ifip->print();
	sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", 
		(unsigned char) _mac[0], 
		(unsigned char) _mac[1], 
		(unsigned char) _mac[2], 
		(unsigned char) _mac[3], 
		(unsigned char) _mac[4], 
		(unsigned char) _mac[5]);
	s += str;
	return(s);
}
	
string
netiface::print() 
{ 
	string s; 
	char str[128];
	
	string sip = _ifip->print();
	sprintf(str, "%d %s=%s MTU=%d", 
		_ifindex, 
		_ifname.c_str(), 
		sip.c_str(),
		_mtu);
	s += str;
	s += " MAC=";
	s += this->mac();
	return(s);
}

// Given an interface name fill in and return *to the MAC address
char *
if_nametomac(char *mac, string ifname)
{

	struct ifreq	ifx;
	int	fd = -1;
	char	s[IF_NAMESIZE];
	strncpy(s, ifname.c_str(), IF_NAMESIZE - 1);

	for (int i = 0; i < 6; i++)
		mac[i] = ifx.ifr_addr.sa_data[i];
	if ((fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0)
	{
		saratoga::scr.error("Can't create socket to do ioctl for MAC lookup of Interface %s\n",
			ifname.c_str());
		return(mac);
	}
	strcpy(ifx.ifr_name, s);

#ifdef MACOSX
        ifaddrs* iflist;

        if (getifaddrs(&iflist) == 0)
        {
                for (ifaddrs* cur = iflist; cur; cur = cur->ifa_next)
                {
                        if (( cur->ifa_addr->sa_family == AF_LINK) &&
                                (strcmp(cur->ifa_name, ifname.c_str()) == 0) &&
                                cur->ifa_addr)
                        {
                                sockaddr_dl* sdl = (sockaddr_dl*)cur->ifa_addr;
                                memcpy(mac, LLADDR(sdl), sdl->sdl_alen);
                                close(fd);
                                freeifaddrs(iflist);
                                return(mac);
                        }
                }
        }
        freeifaddrs(iflist);
        scr.error("Can't ioctl to get MAC address\n");
#else
	if (ioctl(fd, SIOCGIFHWADDR, &ifx) == 0)
	{
		for (int i = 0; i < 6; i++)
			mac[i] = ifx.ifr_addr.sa_data[i];
	}
	else
		saratoga::scr.error("Can't ioctl to get MAC address of Interface %s\n",
			ifname.c_str());
#endif
	close(fd);
	return(mac);
}

// return the interface MTU
int
if_mtu(string ifname)
{
	struct ifreq	ifx;
	int		fd = -1;

	strncpy(ifx.ifr_name, ifname.c_str(), IF_NAMESIZE - 1);
	if ((fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0)
	{
		saratoga::scr.error("can't create socket to do ioctl for MTU lookup of Interface %s\n",
			ifname.c_str());
		return(-1);
	}
	if (ioctl(fd, SIOCGIFMTU, &ifx) == 0)
	{
		close(fd);
		return ifx.ifr_mtu;
	}
	else
		saratoga::scr.error("Can't ioctl to get MTU of Interface %s\n",
			ifname.c_str());
	close(fd);
	return(-1);
}

/*
 ************************************************************************
 * UDP
 ************************************************************************
 */

// Create UDP Socket port for dest ip, fill in the ip structure & set fd
// Used when writing to a particular socket at a destination
udp::udp(string addr, int port)
{
	const int	on = 1;
	struct sockaddr_in *in = (sockaddr_in *) &_sa;
	struct sockaddr_in6 *in6 = (sockaddr_in6 *) &_sa;

	struct protoent	*proto;

	ip	ipa(addr);
	proto = getprotobyname("UDP");
	_readytotx = false; 
	_delay = new timer_group::timer(addr, c_timer.framedelay());

	switch(ipa.family())
	{
	case AF_AX25:
		constructax25(addr);
		return;
	case AF_INET:
		_fd = socket(AF_INET, SOCK_DGRAM, proto->p_proto);
		if (_fd == -1)
		{
			saratoga::scr.perror(errno, "Can't create socket");
			return;
		}
		memcpy(&in->sin_addr, ipa.addr4(), sizeof(struct in_addr));
		in->sin_family = AF_INET;
		in->sin_port = htons(port);
		break;
	case AF_INET6:
		_fd = socket(AF_INET6, SOCK_DGRAM, proto->p_proto);
		if (_fd == -1)
		{
			saratoga::scr.perror(errno, "Can't create socket");
			return;
		}
		memcpy(&in6->sin6_addr, ipa.addr6(), sizeof(struct in6_addr));
		in6->sin6_family = AF_INET6;
		in6->sin6_port = htons(port);
		in6->sin6_flowinfo = htonl(0);
		break;
	default:
		_fd = -1;
		saratoga::scr.error("Can't create socket: Invalid family\n");
		return;
	}
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
	{
		saratoga::scr.error("ip:udp(%s fd=%d port=%" PRIu16 "): Can't setsockopt to SO_REUSEADDR\n",
				ipa.print().c_str(), _fd, port);
		close(_fd);
		_fd = -1;
		return;
	}
	// When writing we will ALWAYS send at least 4 bytes (the size of the smallest sratoga header (flags)
	// On Linux SNDLOWAT cannot be changed and sets errno ENOPROTOOPT
	if (setsockopt(_fd, SOL_SOCKET, SO_SNDLOWAT, &_sndlowat, sizeof(int)) == -1 && errno != ENOPROTOOPT)
	{
		saratoga::scr.error("ip:udp(%s fd=%d port=%" PRIu16 "): Can't setsockopt SO_SNDLOWAT to %d\n",
				ipa.print().c_str(), _fd, port, sarnet::udp::_sndlowat);
		close(_fd);
		_fd = -1;
		return;
	}
}

// Open socket to addr for writing
udp::udp(sarnet::ip *addr, int port)
{
	const int	on = 1;
	struct sockaddr_in *in = (sockaddr_in *) &_sa;
	struct sockaddr_in6 *in6 = (sockaddr_in6 *) &_sa;

	struct protoent	*proto;

	proto = getprotobyname("UDP");
	_readytotx = false; 
	_delay = new timer_group::timer(addr->print(), c_timer.framedelay());

	switch(addr->family())
	{
	case AF_AX25:
		constructax25(addr->addrax25());
		return;
	case AF_INET:
		_fd = socket(AF_INET, SOCK_DGRAM, proto->p_proto);
		if (_fd == -1)
		{
			saratoga::scr.perror(errno, "Can't create socket");
			return;
		}
		memcpy(&in->sin_addr, addr->addr4(), sizeof(struct in_addr));
		in->sin_family = AF_INET;
		in->sin_port = htons(port);
		break;
	case AF_INET6:
		_fd = socket(AF_INET6, SOCK_DGRAM, proto->p_proto);
		if (_fd == -1)
		{
			saratoga::scr.perror(errno, "Can't create socket");
			return;
		}
		memcpy(&in6->sin6_addr, addr->addr6(), sizeof(struct in6_addr));
		in6->sin6_family = AF_INET6;
		in6->sin6_port = htons(port);
		in6->sin6_flowinfo = htonl(0);
		break;
	default:
		_fd = -1;
		saratoga::scr.error("Can't create socket: Invalid family\n");
		return;
	}
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
	{
		saratoga::scr.error("ip:udp(%s fd=%d port=%" PRIu16 "): Can't setsockopt to SO_REUSEADDR\n",
				addr->print().c_str(), _fd, port);
		close(_fd);
		_fd = -1;
		return;
	}
	// When reading we will ALWAYS receive at least 4 bytes (the size of the smallest saratoga header (flags)
	// On Linux SNDLOWAT cannot be changed and sets errno ENOPROTOOPT
	if (setsockopt(_fd, SOL_SOCKET, SO_SNDLOWAT, &_sndlowat, sizeof(int)) == -1 && errno != ENOPROTOOPT)
	{
		saratoga::scr.error("ip:udp(%s fd=%d port=%" PRIu16 "): Can't setsockopt SO_SNDLOWAT to %d\n",
				addr->print().c_str(), _fd, port, _sndlowat);
		close(_fd);
		_fd = -1;
		return;
	}
}

udp::udp(struct sockaddr_storage *p)
{
	
	_readytotx = false;
	_fd = -1; // No fd for this type of udp
	_buf.empty(); // No buffers either
	_delay = new timer_group::timer(0); // No timer

	bzero(&_sa, sizeof (struct sockaddr_storage)); 
	switch (p->ss_family)
	{
	case AF_INET:
		memcpy(&_sa, p, sizeof (struct sockaddr_in));
		break;
	case AF_INET6:
		memcpy(&_sa, p, sizeof (struct sockaddr_in6));
		break;
	case AF_AX25:
		saratoga::scr.error("udp::udp tried to udp::udp sockaddr_storage with AF_AX25 family.");
		return;
	default:
		saratoga::scr.error("udp::udp Invalid Frame Type for udp");
		return;
	}
}

// Create multicast UDP Socket port for a local ip (*p)
// That has already been open and bound
// This is for multicasts
udp::udp(enum mcast_dir dir, udp *p, string addr, int port)
{
	struct ip_mreq	group; // The IPv4 multicast group
	struct ipv6_mreq	group6; // The IPv6 multicast group
	int on = 1;	// Allow socket to be reused

	// The multicast address
	struct sockaddr_in *mcastin = (sockaddr_in *) &_sa;
	struct sockaddr_in6 *mcastin6 = (sockaddr_in6 *) &_sa;

	// The local address to bind to	
	struct sockaddr_in *local = (sockaddr_in *) &p->_sa;
	struct sockaddr_in6 *local6 = (sockaddr_in6 *) &p->_sa;

	struct protoent	*proto;


	proto = getprotobyname("UDP");
	ip	ipa(addr);

	_readytotx = false;
	_delay = new timer_group::timer(addr, c_timer.framedelay());

	if (ipa.family() == AF_AX25){
		saratoga::scr.error("udp::udp: I'm not implemented for AX25!!");
		return;
	}


	// IPv4 Multicast Socket
	if (ipa.family() == AF_INET)
	{
		_fd = socket(AF_INET, SOCK_DGRAM, proto->p_proto);
		if (_fd == -1)
		{
			saratoga::scr.perror(errno, "AF_INET Can't create SOCK_DGRAM socket");
			return;
		}
		memcpy(&mcastin->sin_addr, ipa.addr4(), sizeof(struct in_addr));
		mcastin->sin_family = AF_INET;
		mcastin->sin_port = htons(port);
		if (dir == MCAST_OUT)
		{
			// Bind multicast to the local v4 fd
			if (setsockopt(_fd, IPPROTO_IP, IP_MULTICAST_IF,
				(void *) &local->sin_addr, sizeof (struct in_addr)) < 0)
			{
				string emsg = "Can't bind output multicast " + ipa.print() + 
					" to " + p->straddr();
				saratoga::scr.error(emsg);
				saratoga::scr.perror(errno, "MCAST_OUT v4 ip:udp(%s fd=%d, port=%" PRIu16 ") Can't setsockopt to IP_MULTICAST_IF",
					addr.c_str(), _fd, port);
				close(_fd);
				_fd = -1;
				return;
			}
			string dmsg = "Multicast IPv4 output " + ipa.print() + 
				" bound to " + p->straddr() +
				" port " + p->strport();
			saratoga::scr.debug(7, "%s", dmsg.c_str());

			int ttl = 32; // Site wide multicasts
			if (setsockopt(_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
			{
				string emsg = "Can't set ttl for v4 multicast " + ipa.print();
				saratoga::scr.error(emsg);
				saratoga::scr.perror(errno, "MCAST_OUT v4 ip:udp(%s fd=%d, port=%" PRIu16 ") Can't setsockopt to IP_MULTICAST_IF",
					addr.c_str(), _fd, port);
				close(_fd);
				_fd = -1;
				return;
			}
			// When writing we will ALWAYS send at least 4 bytes (the size of the smallest sratoga header (flags)
			// On Linux SNDLOWAT cannot be changed and sets errno ENOPROTOOPT
			if (setsockopt(_fd, SOL_SOCKET, SO_SNDLOWAT, &_sndlowat, sizeof(int)) == -1 && errno != ENOPROTOOPT)
			{
				saratoga::scr.error("ip:udp(MCAST_OUT %s fd=%d port=%" PRIu16 "): Can't setsockopt SO_SNDLOWAT to %d\n",
						addr.c_str(), _fd, port, _sndlowat);
				close(_fd);
				_fd = -1;
				return;
			}
			return;
		}
		else // MCAST_IN
		{
			if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof(on)) < 0)
			{
				string emsg = "Can't bind input multicast " + ipa.print() + " to " + p->straddr();
				saratoga::scr.error(emsg);
				saratoga::scr.perror(errno, "MCAST_IN v4 ip:udp(%s fd=%d, port=%" PRIu16 ") Can't set SO_REUSEADDR",
					addr.c_str(), _fd, port);
				close(_fd);
				_fd = -1;
				return;
			}
			if (bind(_fd, (struct sockaddr *) local, sizeof (struct sockaddr_in)) != 0)
			{
				saratoga::scr.perror(errno, "MCAST_IN v4 ip:udp(%s fd=%d, port=%" PRIu16 ") Can't bind to local",
					addr.c_str(), _fd, port);
				close(_fd);
				_fd = -1;
				return;
			}
			memcpy(&group.imr_multiaddr, ipa.addr4(), sizeof (struct in_addr));
			memcpy(&group.imr_interface, &local->sin_addr, sizeof(struct in_addr));
			if (setsockopt(_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
				(void *) &group, sizeof(group)) < 0)
			{
				saratoga::scr.perror(errno, "MCAST_IN v4 ip:udp(%s fd=%d, port=%" PRIu16 ") Can't add to multicast group",
					addr.c_str(), _fd, port);
				close(_fd);
				_fd = -1;
				return;
			}
			if (setsockopt(_fd, SOL_SOCKET, SO_RCVLOWAT, &_rcvlowat, sizeof(int)) == -1)
			{
				saratoga::scr.error("MCAST_IN ip:udp(%s fd=%d port=%" PRIu16 "): Can't setsockopt SO_RCVLOWAT to %d\n",
						addr.c_str(), _fd, port, _rcvlowat);
				close(_fd);
				_fd = -1;
				return;
			}
			string dmsg = "Multicast IPv4 input " + ipa.print() + " bound to " + p->straddr() +
				" port " + p->strport();
			saratoga::scr.debug(7, "%s", dmsg.c_str());
			return;
		}
		return;
	}

	// IPv6 Multicast
	if (ipa.family() == AF_INET6)
	{
		// Get the interface index information	
		netifaces interfaces;
		netiface *firstv6 = interfaces.first(AF_INET6);

		_fd = socket(AF_INET6, SOCK_DGRAM, proto->p_proto);
		if (_fd == -1)
		{
			saratoga::scr.perror(errno, "AF_INET6 Can't create socket");
			return;
		}
		memcpy(&mcastin6->sin6_addr, ipa.addr6(), sizeof(struct in6_addr));
		mcastin6->sin6_family = AF_INET6;
		mcastin6->sin6_port = htons(port);
		mcastin6->sin6_flowinfo = htonl(0);
		mcastin6->sin6_scope_id = 0; // htonl(firstv6->ifindex());
		if (dir == MCAST_OUT)
		{
			// Get the Index of the First IPv6 Interface
			uint_t v6index = firstv6->ifindex();
			saratoga::scr.debug(7, "First V6 Index is %d", v6index);
			// Bind it to local v6 fd
			if (setsockopt(_fd, IPPROTO_IPV6, IPV6_MULTICAST_IF,
				(void *) &v6index, sizeof(v6index)) < 0)
			{
				string emsg = "Trying to bind multicast " + ipa.print() + " to " + p->straddr();
				saratoga::scr.error(emsg);
				saratoga::scr.perror(errno, "MCAST_OUT v6 ip:udp(%s fd=%d, port=%" PRIu16 "): Can't setsockopt to IPV6_MULTICAST_IF",
					addr.c_str(), _fd, port);
				close(_fd);
				_fd = -1;
				return;
			}
			string dmsg = "Multicast IPv6 output " + ipa.print() + " bound to " + p->straddr() +
				" port " + p->strport();
			saratoga::scr.debug(7, "%s", dmsg.c_str());

			int hops = 32; // Site wide multicasts
			if (setsockopt(_fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops, sizeof(hops)) < 0)
			{
				string emsg = "Can't set ttl for multicast " + ipa.print();
				saratoga::scr.error(emsg);
				saratoga::scr.perror(errno, "MCAST_OUT v4 ip:udp(%s fd=%d, port=%" PRIu16 ") Can't setsockopt to IP_MULTICAST_IF",
					addr.c_str(), _fd, port);
				close(_fd);
				_fd = -1;
				return;
			}
 
			return;
		}
		else // MCAST_IN
		{
			if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof(on)) < 0)
			{
				saratoga::scr.perror(errno, "MCAST_IN v6 ip:udp(%s fd=%d, port=%" PRIu16 ") Can't set SO_REUSEADDR",
					addr.c_str(), _fd, port);
				close(_fd);
				_fd = -1;
				return;
			}
			if (bind(_fd, (struct sockaddr *) local6, sizeof (struct sockaddr_in6)) != 0)
			{
				string emsg = "Trying to bind to local v6 address " + p->print();
				saratoga::scr.error(emsg);
				saratoga::scr.perror(errno, "MCAST_IN v6 ip:udp(%s fd=%d, port=%" PRIu16 ") Can't bind to local6",
					addr.c_str(), _fd, port);
				close(_fd);
				_fd = -1;
				return;
			}
			memcpy(&group6.ipv6mr_multiaddr, ipa.addr6(), sizeof (struct in6_addr));
			saratoga::scr.debug(7, "Setting V6 Index to %d", firstv6->ifindex());
			group6.ipv6mr_interface = 0; // firstv6->ifindex();
			if (setsockopt(_fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
				(void *) &group6, sizeof(group6)) < 0)
			{
				saratoga::scr.perror(errno, "ip:udp(%s fd=%d, port=%" PRIu16 ") Can't add to multicast group",
					addr.c_str(), _fd, port);
				close(_fd);
				_fd = -1;
				return;
			}
			string dmsg = "Multicast IPv6 input " + ipa.print() + " bound to " + p->straddr() +
				" port " + p->strport();
			saratoga::scr.debug(7, "%s", dmsg.c_str());
			return;
		}
		return;
	}
	saratoga::scr.error("ip:udp(%s fd=%d, port=%" PRIu16 ") Invalid family",
		addr.c_str(), _fd, port);
	_fd = -1;
}

// Open and bind the _fd to incoming UDP frames on a port (socket)
// so we can send and receive
udp::udp(int protocol, int port)
{
	const int	on = 1;
	struct protoent	*proto;
	struct sockaddr_in *in = (sockaddr_in *) &_sa;
	struct sockaddr_in6 *in6 = (sockaddr_in6 *) &_sa;

	proto = getprotobyname("UDP");
	_readytotx = true;
	_delay = new timer_group::timer(c_timer.framedelay());

	bzero(&_sa, sizeof(struct sockaddr_storage));
	switch(protocol)
	{
	case AF_AX25:
		saratoga::scr.error("udp::udp: I'm not implemented for AX25!!");
		return;
	case AF_INET:
		_fd = socket(AF_INET, SOCK_DGRAM, proto->p_proto);
		in->sin_family = AF_INET;
		in->sin_addr.s_addr = htonl(INADDR_ANY);
		in->sin_port = htons(port);
		break;
	case AF_INET6:
		_fd = socket(AF_INET6, SOCK_DGRAM, proto->p_proto);
		in6->sin6_family = AF_INET6;
		in6->sin6_addr = in6addr_any;
		in6->sin6_port = htons(port);
		break;
	default:
		saratoga::scr.error("ip::udp(%d, %" PRIu16 "): Bad protocol only AF_INET or AF_INET6\n",
			protocol, port);
		_fd = -1;
		return;
	}
	if (_fd == -1)
	{
		saratoga::scr.error("ip:udp(%" PRIu16 "): Can't create socket\n",
			port);
		return;
	}
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
	{
		saratoga::scr.error("ip:udp(%d, %" PRIu16 "): Can't setsockopt to SO_REUSEADDR\n",
				_fd, port);
		if (_fd != -1)
		{
			close(_fd);
		}
		_fd = -1;
		return;
	}
	// When reading we will ALWAYS receive at least 4 bytes (the size of the smallest sratoga header (flags)
	if (setsockopt(_fd, SOL_SOCKET, SO_RCVLOWAT, &_rcvlowat, sizeof(int)) == -1)
	{
		saratoga::scr.error("ip:udp(fd=%d port=%" PRIu16 "): Can't setsockopt SO_RCVLOWAT to %d\n",
				_fd, port, _rcvlowat);
		close(_fd);
		_fd = -1;
		return;
	}

	// Now bind the socket to the fd	
	switch(protocol)
	{
	case AF_INET:
		if (bind(_fd, (struct sockaddr *) &_sa, sizeof(struct sockaddr_in)) != 0)
		{
			saratoga::scr.error("udp::bind(%d, %" PRIu16 "): AF_INET Can't bind socket",
				_fd, port);
			close(_fd);
			_fd = -1;
		}
		break;
	case AF_INET6:
		if (setsockopt(_fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) == -1)
			saratoga::scr.error("ip:udp(%d, %" PRIu16 "): Can't setsockopt to IPV6_V6ONLY\n",
				_fd, port);
		if (bind(_fd, (struct sockaddr *) &_sa, sizeof(struct sockaddr_in6)) != 0)
		{
			saratoga::scr.perror(errno, "udp::bind(%d, %" PRIu16 "): AF_INET6 Can't bind socket",
				_fd, port);
			close(_fd);
			_fd = -1;
		}
		break;
	default:
		break;
	}
}

// Should we do a htons here CHECK IT!
int
udp::port()
{
	struct sockaddr_in *in = (sockaddr_in *) &_sa;
	struct sockaddr_in6 *in6 = (sockaddr_in6 *) &_sa;

	switch(this->family())
	{
	case AF_INET:
		return(ntohs(in->sin_port));
	case AF_INET6:
		return(ntohs(in6->sin6_port));
	case AF_AX25:
		saratoga::scr.debug(7,"udp::port(): Returning dummy port (0) for ax25 port.");
		return(0);
	default:
		return(0);
	}
}

string
udp::strport()
{
	if(this->family() == AF_AX25)
		return "0";

	string s;
	char	cstr[16];

	sprintf(cstr, "%d", this->port());
	s += cstr;
	return(s);
}



// Actually send buffers to a udp socket
int
udp::send()
{
	if (this->family() == AF_AX25)
		return ax25send();


	// static socklen_t	tolen;
	socklen_t	tolen;
	ssize_t			nwritten;
	int			flags = MSG_DONTWAIT;
	size_t	bcount = 0;	// # bytes written
	string adr = this->straddr();

	switch(this->family())
	{
	case AF_INET6:
		tolen = sizeof(struct sockaddr_in6);
		break;
	case AF_INET:
		tolen = sizeof(struct sockaddr_in);
		break;
	default:
		saratoga::scr.error("hello udp::send(): Bad protocol only AF_INET & AF_INET6 supported\n");
		return(-1);
	}

	// We only send frames when our delay has timed out
	// Yes we will send all of the frames in our buffersa
	// YES this could be problamatic I know but I dont want to do
	// too many system calls!
	if (!_delay->timedout())
		return(0);
	_delay->reset();
	// Send the buffers & flush the buffers
	while (! _buf.empty())
	{
		saratoga::buffer *tmp = &(_buf.front());
		char	*b = tmp->buf();
		ssize_t	blen = tmp->len();
		if (blen != 0)
		{
			nwritten = sendto(_fd, b, blen, flags, this->saptr(), tolen);
			if (nwritten < 0)
			{
				int err = errno;
				saratoga::scr.perror(err, "udp::send(%d): Cannot write %d bytes to %s Port %d\n",
					this->fd(), 
					blen, 
					adr.c_str(), 
					this->port());
			}
			else
			{
				saratoga::scr.debug(4, "udp::send(%d): Wrote %d bytes to %s Port %d",
					this->fd(),
					nwritten,
					adr.c_str(), 
					this->port());
			}
			bcount += nwritten;
		}
		// Pop it whether we sent it correctoy or not so we don't get
		// into a race condition
		_buf.pop_front();
		// Send back total bytes written
	}
	_readytotx = false;
	return(bcount);
}

ssize_t
udp::rx(char *b, sarnet::ip *from)
{
	memset(b, 0, 9000);
	saratoga::scr.debug(7,"udp::rx: Doing a AX25 detour.");
	if (this->family() == AF_AX25)
		return ax25rx(b,from);


	string			s;

	struct sockaddr_storage	addrbuf;

	socklen_t		socklen;
	ssize_t			nread;
	int			flags = MSG_DONTWAIT;

	s = "Receiving interface " + this->print();
	saratoga::scr.debug(2, s);
	switch (this->family())
	{
	case AF_INET:
		socklen = sizeof (struct sockaddr_in);
		break;
	case AF_INET6:
		socklen = sizeof (struct sockaddr_in6);
		break;
	default:
		saratoga::scr.error("udp::rx() Invalid family");
		return -1;
	}

	nread = recvfrom(_fd, b, 9000, flags, (struct sockaddr *) &addrbuf, &socklen);

	sarnet::ip retaddr(&addrbuf);
	*from = retaddr;
	s = retaddr.straddr();
	saratoga::scr.debug(7, "udp::rx(): Received %d bytes from %s", nread, s.c_str());
	if (nread < 0)
	{
		int err = errno;
		saratoga::scr.perror(err, "udp::rx(): Cannot read\n");
	}
 
	return(nread);
}

// Return string of the IP address
string
udp::straddr()
{
	if(this->family() == AF_AX25)
		return (string)ax25addr;


	saratoga::scr.debug(7,"udp::straddr()\n");
	string ret;
	struct sockaddr_in *in = (struct sockaddr_in *) &_sa;
	struct sockaddr_in6 *in6 = (struct sockaddr_in6 *) &_sa;
	char *s;
	
	switch(this->family())
	{
	case AF_INET:
		s = new char [INET_ADDRSTRLEN];
		// s = (char *) malloc(INET_ADDRSTRLEN);
		inet_ntop(this->family(), &in->sin_addr, s, INET_ADDRSTRLEN);
		ret = s;
		free(s);
		return(ret);
	case AF_INET6:
		s = new char [INET6_ADDRSTRLEN];
		// s = (char *) malloc(INET6_ADDRSTRLEN);
		inet_ntop(this->family(), &in6->sin6_addr, s, INET6_ADDRSTRLEN);
		ret = s;
		free(s);
		return(ret);
	default:
		saratoga::scr.error("udp::straddr() Invalid IP family\n");
		return("OTHER");
	}
}

bool
udp::setopt(int option, void *optval, socklen_t optsize) {

	if (setsockopt(_fd, SOL_SOCKET, option, optval, optsize) < 0)
	{
		saratoga::scr.perror(errno, "Can't setsockopt %d for socket %d\n",
			option, _fd);
		return(0);
	}
	return(1);
};

// Is the option set OK ?
bool 
udp::getopt(int option, void *optval, socklen_t *optlen) {
	
	if (getsockopt(_fd, SOL_SOCKET, option, optval, optlen) < 0)
	{
		saratoga::scr.perror(errno, "Can't getsockopt %d for socket %d\n",
			option, _fd);
		return(0);
	}
	return(1);
};

string
udp::print()
{
	char	tmp[128];
	string ret = "UDP:";

	switch(this->family())
	{
	case AF_INET:
		ret += "AF_INET ";
		break;
	case AF_INET6:
		ret += "AF_INET6 ";
		break;
	case AF_AX25:
		ret += "AF_AX25 ";
		break;
	default:
		ret += "OTHER";
		break;
	}
	ret += this->straddr();
	sprintf(tmp, " PORT=%" PRIu16 " FD=%d", this->port(), this->_fd);
	ret += tmp;
	return(ret);
}

// Return the largest currently open fd - used by maxfd() for select()
int
peers::largestfd()
{
	int	largest = 2; // Take into account stdin, stdout, stderr

	// The peers we always have open to read and write to/from
	largest = FDMAX(largest,v4in->fd());
	largest = FDMAX(largest,v6in->fd());
	largest = FDMAX(largest,v4out->fd());
	largest = FDMAX(largest,v6out->fd());
	largest = FDMAX(largest,v4loop->fd());
	largest = FDMAX(largest,v6loop->fd());
	largest = FDMAX(largest,v4mcastout->fd());
	largest = FDMAX(largest,v6mcastout->fd());
	largest = FDMAX(largest,v4mcastin->fd());
	largest = FDMAX(largest,v6mcastin->fd());

	for (std::list<udp>::iterator i = _peers.begin(); i != _peers.end(); i++)
	{
		if (i->fd() > largest)
			largest = i->fd();
	}
	return(largest);
}

inline bool
peers::exists(int fd)
{
	for (std::list<udp>::iterator i = _peers.begin(); i != _peers.end(); i++)
	{
		udp p = *i;
		if (p.fd() == fd)
			return true;
	}
	return(false);
}

string
peers::print() 
{
	string s = "";

	for (std::list<udp>::iterator i = _peers.begin(); i != _peers.end(); i++)
	{
		s += i->print();
		s += '\n';
	}
	return(s);
}

// Given an already openened socket add it to our list of peers
sarnet::udp*
peers::add(sarnet::udp *p)
{

	string s = p->straddr();
	if (p->fd() <= 2)
	{
		saratoga::scr.error("peers::add cannot add fd <= 2, fd=%d", p->fd());
		return(nullptr);
	}
	for (std::list<udp>::iterator i = _peers.begin(); i != _peers.end(); i++)
	{
		if (i->fd() == p->fd())
		{
			saratoga::scr.error("peers::add fd=%d already added to files", p->fd());
			return(&(*i));
		}
	}
	saratoga::scr.msg("Added new Peer %s port %d fd=%d", s.c_str(), p->port(), p->fd());
	_fdchange = true; // We have definately changed # peers for select()
	_peers.push_back(*p);
	return(p);
}

sarnet::udp*
peers::add(sarnet::ip *address, int port)
{
	sarnet::udp	*newsock;
	string sa = address->straddr();

	for (std::list<udp>::iterator i = _peers.begin(); i != _peers.end(); i++)
	{
		if (i->straddr() == sa && i->port() == port)
		{
			saratoga::scr.msg("peers::add socket already exists to %s port %d",
				sa.c_str(), port);
			return(&(*i));
		}
	}
	// AX25
	if(address->family() == AF_AX25){
		char *cstr = new char[address->addrax25().length() + 1];
		strcpy(cstr, address->addrax25().c_str());
		newsock = new sarnet::udp(cstr);
		saratoga::scr.debug(7,"created new AX25 peer %s",newsock->straddr());
	}else
		newsock = new sarnet::udp(address, port);

	if (newsock->fd() <= 2)
	{
		saratoga::scr.error("peers::add cannot add peer with fd <= 2, fd=%d", newsock->fd());
		delete newsock;
		return(nullptr);
	}
	saratoga::scr.msg("Added new Peer %s port %d fd=%d",
		sa.c_str(), port, newsock->fd());
	_fdchange = true; // We have definately changed # peers for select()
	_peers.push_back(*newsock);
	return(newsock);
}

// Create a new open socket in out list of peers
// return a pointer to the socket
sarnet::udp *
peers::add(string addr, int port)
{
	sarnet::udp	*newsock;

	for (std::list<udp>::iterator i = _peers.begin(); i != _peers.end(); i++)
	{
		if (i->straddr() == addr && i->port() == port)
		{
			saratoga::scr.msg("peers::add socket already exists to %s port %d",
				addr.c_str(), port);
			return(&(*i));
		}
	}
	// AX25
	if((new ip(addr))->family() == AF_AX25){
		char *cstr = new char[addr.length() + 1];
		strcpy(cstr, addr.c_str());
		saratoga::scr.debug(7, "peers::add(string,int) created AX25 sock.");
		newsock = new sarnet::udp(cstr);
	}else
		newsock = new sarnet::udp(addr, port);

	if (newsock->fd() <= 2){
		saratoga::scr.error("peers::add cannot add peer with fd <= 2, fd=%d", newsock->fd());
		delete newsock;
		return(nullptr);
	}
	saratoga::scr.msg("Added new Peer %s port %d fd=%d", addr.c_str(), port, newsock->fd());
	_fdchange = true; // We have definately changed # peers for select()
	_peers.push_back(*newsock);
	return(newsock);
}

// Remove a peer based upon file descriptor
void
peers::remove(int fd)
{

	for (std::list<udp>::iterator i = _peers.begin(); i != _peers.end(); i++)
	{
		if (i->fd() == fd)
		{
			saratoga::scr.msg("peers::remove Removing Peer fd=%d", fd);
			// i->zap();		// Clear the socket
			_peers.erase(i);	// erase it from list
			_fdchange = true; // We have definately changed # peers for select()
			return;
		}
	}
	saratoga::scr.error("fd=%d does not exist in peers to close", fd);
}

// Remove a peer based upon ip address and port #
void
peers::remove(string ipaddr, int port)
{

	for (std::list<udp>::iterator i = _peers.begin(); i != _peers.end(); i++)
	{
		if (i->straddr() == ipaddr && i->port() == port)
		{
			saratoga::scr.msg("peers::remove Removing Peer %s port %d",
				ipaddr.c_str(), port);
			// i->zap();		// Clear the socket
			_peers.erase(i);	// erase it from list
			_fdchange = true; // We have definately changed # peers for select()
			return;
		}
	}
	saratoga::scr.error("Peer %s port %d does not exist in peers to close",
				ipaddr.c_str(), port);
}

// Return pointer to socket if the IP address is a match
sarnet::udp *
peers::match(sarnet::ip *host)
{
	string haddr = host->straddr();
	for (std::list<udp>::iterator i = _peers.begin(); i != _peers.end(); i++)
	{
		//ax25* s = i.pointer;
		//string c = typeid(s).name();
		//saratoga::scr.error(c);
		//ax25 s = i;
		//saratoga::scr.error(s.straddr());
		sarnet::ip	ai(i->straddr());
		if (*host == ai)
			return(&(*i));
	}
	return(nullptr);
}

sarnet::udp *
peers::match(string addr)
{
	sarnet::ip	a(addr);

	for (std::list<udp>::iterator i = _peers.begin(); i != _peers.end(); i++)
	{
		sarnet::ip	ai(i->straddr());
		if (a == ai)
			return(&(*i));
	}
	return(nullptr);
}


/*************************************
 *  AX25
 *************************************/


int udp::ax25outsock;
char* udp::ax25srcaddress;
struct full_sockaddr_ax25 udp::ax25src;
bool udp::ax25available = false;
char* udp::ax25portcall;
int udp::ax25slen;
int udp::ax25insock;
char* udp::dev;




int udp::initax25(){
	// AX25
	if (ax25_config_load_ports() == 0) {
		saratoga::scr.error("[AX25] No AX.25 ports defined\n");
		return -1;
	}else {

		//Bootstrap
		if ((ax25portcall = ax25_config_get_addr((char*)ax25port)) == NULL) {
			saratoga::scr.error("[AX25] Invalid AX.25 port \n"+ string(ax25port) );
			return -1;
		}

		// Prepare out socket
		if ((ax25slen = ax25_aton(ax25portcall, &ax25src)) == -1) {
			saratoga::scr.error("[AX25] Unable to convert source callsign \n" + string(ax25portcall));
			return -1;
		}
		if ((ax25outsock = socket(AF_AX25, SOCK_DGRAM, 0)) == -1) {
			saratoga::scr.error( "[AX25] socket() error");
			return -1;
		}
		if (bind(ax25outsock, (struct sockaddr *)&ax25src, ax25slen) == -1) {
			saratoga::scr.error( "[AX25] bind() error");
			return -1;
		}

		ax25srcaddress = ax25_ntoa(&ax25src.fsa_ax25.sax25_call);



		// Prepare in socket
		if ((dev = ax25_config_get_dev((char*)ax25port)) == NULL) {
			saratoga::scr.error("AX25 could not init outsocket: invalid port name \n");
			return 1;
		}
		if ((ax25insock = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_AX25))) == -1) {
			saratoga::scr.error("AX25 could not init outsocket \n"+to_string(ax25insock));
			return 1;
		}


		ax25available=true;
		saratoga::ax25multiout = new sarnet::udp(udp::ax25multidestcall);
		saratoga::ax25multiin = new sarnet::udp(true);
		saratoga::scr.msg("[AX25] AX.25 started with success.\n");


	}

	return 0;
}

void udp::constructax25(char* destcall){

	ax25addr=(string)destcall;
	ax25destcall = destcall;
	_maxbuff=_ax25size;

	// AX25 Stuff
	if ((ax25dlen = ax25_aton(ax25destcall, &ax25dest)) == -1) {
		saratoga::scr.error("[AX25] Unable to convert destination callsign \n" + string(ax25destcall));
		return ;
	}


	// IP Stuff
	int port= 0;

	const int	on = 1;
	struct sockaddr_in *in = (sockaddr_in *) &_sa;


	_isax25 = true;
	_readytotx = false;
	_buf.empty(); // No buffers either
	_delay = new timer_group::timer(0); // No timer
	_fd=ax25outsock;
}

// For in socket
udp::udp(bool imanidiot )
{
	_maxbuff=_ax25size;

	// IP Stuff
	int port= 0;

	const int       on = 1;
	struct sockaddr_in *in = (sockaddr_in *) &_sa;
	struct protoent *proto;


	_readytotx = false;

	_isax25 = true;
	_readytotx = false;
	_buf.empty(); // No buffers either
	_delay = new timer_group::timer(0); // No timer
	_fd=ax25insock;

}



int udp::ax25send(){

	saratoga::scr.debug(7,"udp::ax25send() entered.");
	// static socklen_t	tolen;
	socklen_t	tolen;
	ssize_t			nwritten;
	int			flags = MSG_DONTWAIT;
	size_t	bcount = 0;	// # bytes written
	string adr = this->straddr();



	// We only send frames when our delay has timed out
	// Yes we will send all of the frames in our buffersa
	// YES this could be problamatic I know but I dont want to do
	// too many system calls!
	if (!_delay->timedout())
		return(0);
	_delay->reset();
	// Send the buffers & flush the buffers
	while (! _buf.empty())
	{
		saratoga::buffer *tmp = &(_buf.front());
		char	*b = tmp->buf();
		ssize_t	blen = tmp->len();
		if (blen != 0)
		{
			//nwritten = sendto(_fd, b, blen, flags, this->saptr(), tolen);
			saratoga::scr.debug(7,"[AX25] sendto AX25 something!");
			nwritten = sendto(udp::ax25outsock, b, blen, flags, (struct sockaddr *)&ax25dest, ax25dlen);
			if (nwritten < 0)
			{
				int err = errno;
				saratoga::scr.perror(err, "ax25::send(%d): Cannot write %d bytes to %s Port %d\n",
					this->fd(),
					blen,
					adr.c_str(),
					this->port());
			}
			else
			{
				saratoga::scr.debug(4, "ax25::send(%d): Wrote %d bytes to %s Port %d",
					this->fd(),
					nwritten,
					adr.c_str(),
					this->port());
			}
			bcount += nwritten;
		}
		// Pop it whether we sent it correctoy or not so we don't get
		// into a race condition
		_buf.pop_front();
		// Send back total bytes written
	}
	_readytotx = false;
	return(bcount);
}


// For out socket.
udp::udp(char* destcall){
	constructax25(destcall);
}


static char* readable_dump(unsigned char *data, int length)
{
	unsigned char c;
	int i;
	int cr = 1;
	char buf[1500];
	memset(buf,0,1500);
	char *str;
	char *d, *m, *y, *h, *min, *s, *upd, *uph, *upm, *load0, *load1, *load2, *freemem;

	for (i = 0; length > 0; i++) {

		c = *data++;
		length--;

		switch (c) {
		case 0x00:
			buf[i] = ' ';
		case 0x0A:	/* hum... */
		case 0x0D:
			if (cr)
				buf[i] = ' ';
			else
				i--;
			break;
		default:
			buf[i] = c;
		}
		cr = (buf[i] != '\n');
	}
	if (cr)
		buf[i++] = '\n';
	buf[i++] = '\0';
	str = (char *) malloc(sizeof(char)*i);
	memcpy(str, &buf[17], i);
	return str;
}

ssize_t udp::ax25rx(char *b, sarnet::ip *from)
{
	saratoga::scr.debug(7,"udp::rx: Doing a AX25 detour.");
	struct sockaddr sa;
	socklen_t asize = sizeof(sa);
	int nread;
	unsigned char* data = ((unsigned char*)malloc(sizeof(unsigned char) * 9000));
	memset(data,0,9000);

	saratoga::scr.msg("AX25 Checking for messages.");

	if ((nread = recvfrom(ax25insock, data, 9000, MSG_DONTWAIT, &sa, &asize)) == -1) {
		saratoga::scr.debug(7,"Nothing to read from ax25insock!");
		return 0;
	}

	b=readable_dump(data,nread);

	string addr = sa.sa_data;
	saratoga::scr.msg("AX25 Received beacon from "+addr);

	sarnet::ip retaddr(addr);
	*from = retaddr;

	saratoga::scr.debug(7, "ax25::rx(): Received %d bytes from "+addr, nread);
	if (nread < 0)
	{
		int err = errno;
		saratoga::scr.perror(err, "ax25::rx(): Cannot read\n");
	}

	return(nread);

}


}; // namespace sarnet
