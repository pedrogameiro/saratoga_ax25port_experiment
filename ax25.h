/*
 * ax25.h
 *
 *  Created on: 13 Dec 2015
 *      Author: petrucci
 */

#ifndef AX25_H_
#define AX25_H_



#include <netax25/axlib.h>
#include <netax25/ax25.h>
#include <netax25/axconfig.h>
#include "ip.h"


//timer c;
using namespace std;
using namespace timer_group;
using namespace saratoga;

/*
 **********************************************************************
 * UDP
 **********************************************************************
 */

namespace sarnet {




class ax25 : public udp
{
private:

	//AX25lib
	// AX25
	static bool ax25available;
	static char* ax25portcall;
	static int ax25slen;
	static char* ax25srcaddress;
	static struct full_sockaddr_ax25 ax25src;
	static int	ax25sock; 		// file descriptor

	static constexpr char* ax25port = "spacelink" ;
	static constexpr char* ax25multidestcall = "ALL";

	struct full_sockaddr_ax25 ax25dest;
	char *ax25destcall;
	int ax25dlen;

	struct sockaddr_storage	_sa; // sockaddr info and it is big enough to hold v4 & v6 info
	int	_fd; // file descriptor

	std::list<saratoga::buffer>	_bufax25; // Frames queued to send
	bool	_readytotx;	// Sets FD_SET() or FD_CLR() for tx
	timer_group::timer	*_delay;	// Used to implement a delay between sending frames

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


	static char* getax25srcaddr(){return ax25::ax25srcaddress; }
	static bool isax25available(){return ax25::ax25available;}
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
	ssize_t tx(char *buf, size_t buflen) {
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
	int send();

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


}
#endif /* AX25_H_ */
