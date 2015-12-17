/*

 Copyright (c) 2011, Charles Smith
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

#ifndef _PEERINFO_H
#define _PEERINFO_H

#include <iostream>
#include <cstring>
#include <string>
using namespace std;

#include "saratoga.h"
#include "sarflags.h"
#include "beacon.h"
#include "screen.h"
#include "timestamp.h"
#include "dirent.h"
#include "ip.h"

namespace saratoga
{
/*
 **********************************************************************
 * PEER INFORAMTION OBTAINED FROM BEACONS
 **********************************************************************
 */
 
class peerinfo
{
private:
	sarnet::ip	_ip;	// IP Address of the peer this is the index
	flag_t		_flags; // Flags set for this peer (by beacon)
	offset_t	_freespace;
	string		_eid; // The EID advertised
	bool		_ok;
//	saratoga::beacon	_current; // The current beacon
//	saratoga::beacon	_prev; // Last beacon Rx to compare
public:

	peerinfo(sarnet::ip *addr, saratoga::beacon *b);

	~peerinfo() { this->zap(); };

	void zap() {
		_ip.zap();
		_flags = 0;
		_freespace = 0;
		_eid.clear();
	};

	// What is the beacon eid
	string eid() { return _eid; };

	// What are the beacons flags
	flag_t	flags() { return _flags; };

	// What is the beacons advertised freespace
	offset_t freespace() { return _freespace; };

	// Whis is the ip address of the peer
	sarnet::ip ip() { return(_ip); };
	
	// These and only these flags can be changed during a session

	// tx willing on,off,capable
	enum f_txwilling	txwilling(); // What is it ?

	// rx willing on,off,capable
	enum f_rxwilling	rxwilling(); // What is it ?

	string straddr() { return(_ip.straddr()); };

	// Is the peer info all good
	bool	ok() { return(_ok); };

	// Print out the beacon details
	string	print();
};

// List of current peer information
// This is populated and amended when a beacon is received
// It is deleted when the peer is deleted from sarpeers
class peersinfo
{
private:
	const size_t	_max = 100; // Maximum # of peers
	std::list<saratoga::peerinfo>	_peers;
public:
	peersinfo() { };
	~peersinfo() { this->zap(); };

	saratoga::peerinfo	*add(sarnet::ip *addr, saratoga::beacon *b);
	void	remove(sarnet::ip *addr);

	void	zap() {
		for (std::list<saratoga::peerinfo>::iterator p = this->begin(); p != this->end(); p++)
			p->zap();
		while (! _peers.empty())
			_peers.pop_front();
	};

	std::list<saratoga::peerinfo>::iterator begin() { return(_peers.begin()); };
	std::list<saratoga::peerinfo>::iterator end() { return(_peers.end()); };

	saratoga::peerinfo	*match(sarnet::ip *addr); // Return ptr to peerinfo

	string print();
};

}; // Namespace saratoga

#endif // _PEERINFO_H

