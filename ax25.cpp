#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <string>
#include "screen.h"
#include "ip.h"
#include "globals.h"
#include "ax25.h"
#include <list>


using namespace std;

// #include "timestamp.h"
/*
 * Everything to do with ip and udp networking basics go here under sarnet namespace
 */
namespace sarnet
{

/*
 ************************************************************************
 * UDP
 ************************************************************************
 */

// Create UDP Socket port for dest ip, fill in the ip structure & set fd
// Used when writing to a particular socket at a destination


ax25::ax25(char* destcall)
{
	// AX25 Stuff
	if ((ax25dlen = ax25_aton(ax25destcall, &ax25dest)) == -1) {
		saratoga::scr.error("[AX25] Unable to convert destination callsign \n" + string(ax25destcall));
		return ;
	}
	ax25::ax25destcall = destcall;

	// IP Stuff
	string addr = "0.0.0.0";
	int port= sarport;

	const int	on = 1;
	struct sockaddr_in *in = (sockaddr_in *) &_sa;
	struct protoent	*proto;

	ip	ipa(addr);
	proto = getprotobyname("UDP");
	_readytotx = false;
	_delay = new timer_group::timer(addr, c_timer.framedelay());
	memcpy(&in->sin_addr, ipa.addr4(), sizeof(struct in_addr));
	in->sin_family = AF_INET;
	in->sin_port = htons(port);

	_readytotx = false;
	_buf.empty(); // No buffers either
	_delay = new timer_group::timer(0); // No timer

}

int ax25::initax25(){

	// AX25
	if (ax25_config_load_ports() == 0) {
		saratoga::scr.error("[AX25] No AX.25 ports defined\n");
		return -1;
	}else {

		//Bootstrap
		if ((ax25portcall = ax25_config_get_addr(ax25port)) == NULL) {
			saratoga::scr.error("[AX25] Invalid AX.25 port \n"+ string(ax25port) );
			return -1;
		}
		if ((ax25slen = ax25_aton(ax25portcall, &ax25src)) == -1) {
			saratoga::scr.error("[AX25] Unable to convert source callsign \n" + string(ax25portcall));
			return -1;
		}

		// Prepare
		if ((ax25sock = socket(AF_AX25, SOCK_DGRAM, 0)) == -1) {
			saratoga::scr.error( "[AX25] socket() error");
			return -1;
		}
		if (bind(ax25sock, (struct sockaddr *)&ax25src, ax25slen) == -1) {
			saratoga::scr.error( "[AX25] bind() error");
			return -1;
		}

		ax25srcaddress = ax25_ntoa(&ax25src.fsa_ax25.sax25_call);
		ax25available=true;
		saratoga::scr.msg("[AX25] AX.25 started with success.\n");
	}

	return 0;

}

// Should we do a htons here CHECK IT!
int
ax25::port()
{
	struct sockaddr_in *in = (sockaddr_in *) &_sa;
	struct sockaddr_in6 *in6 = (sockaddr_in6 *) &_sa;

	switch(this->family())
	{
	case AF_INET:
		return(ntohs(in->sin_port));
	case AF_INET6:
		return(ntohs(in6->sin6_port));
	default:
		return(0);
	}
}

string
ax25::strport()
{
	string s;
	char	cstr[16];

	sprintf(cstr, "%d", this->port());
	s += cstr;
	return(s);
}

// Actually send buffers to a udp socket
int
ax25::send()
{
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
		saratoga::scr.error("udp::send(): Bad protocol only AF_INET & AF_INET6 supported\n");
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
	while (! _bufax25.empty())
	{
		saratoga::buffer *tmp = &(_bufax25.front());
		char	*b = tmp->buf();
		ssize_t	blen = tmp->len();
		if (blen != 0)
		{
			//nwritten = sendto(_fd, b, blen, flags, this->saptr(), tolen);
			nwritten = sendto(ax25sock, b, blen, flags, (struct sockaddr *)&ax25dest, ax25dlen);
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
ax25::rx(char *b, sarnet::ip *from)
{

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
ax25::straddr()
{
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
ax25::setopt(int option, void *optval, socklen_t optsize) {

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
ax25::getopt(int option, void *optval, socklen_t *optlen) {

	if (getsockopt(_fd, SOL_SOCKET, option, optval, optlen) < 0)
	{
		saratoga::scr.perror(errno, "Can't getsockopt %d for socket %d\n",
			option, _fd);
		return(0);
	}
	return(1);
};

string
ax25::print()
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
	default:
		ret += "OTHER";
		break;
	}
	ret += this->straddr();
	sprintf(tmp, " PORT=%" PRIu16 " FD=%d", this->port(), this->_fd);
	ret += tmp;
	return(ret);
}

}
