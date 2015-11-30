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

#include <iostream>
#include <cstring>
#include <string>
#include <limits>

#include "saratoga.h"
#include "screen.h"
#include "globals.h"
#include "ip.h"
#include "sarflags.h"
#include "beacon.h"
#include "peerinfo.h"

using namespace std;

namespace saratoga 
{

peerinfo::peerinfo(sarnet::ip *addr, saratoga::beacon *b)
{
	_ip = *addr;
	_flags = b->flags();
	_freespace = b->freespace();
	_eid = b->eid();
	if ((addr == nullptr) || (b == nullptr))
		_ok = false;
	else
		_ok = true;
}

enum f_txwilling
peerinfo::txwilling()
{
	Ftxwilling	txwilling(_flags);
	return(txwilling.get());
}

enum f_rxwilling
peerinfo::rxwilling()
{
	Frxwilling	rxwilling(_flags);
	return(rxwilling.get());
}

saratoga::peerinfo *
peersinfo::add(sarnet::ip *addr, saratoga::beacon *b)
{
	peerinfo	*p;

	for (std::list<peerinfo>::iterator i = _peers.begin(); i != _peers.end(); i++)
	{
		if (i->straddr() == addr->straddr())
		{
			// Erase it  then re-add it with new info
			i->zap();
			_peers.erase(i);
			break;
		}
	}
	// Add it then return
	p = new saratoga::peerinfo(addr, b);
	string ipstr = addr->straddr();
	if (p->ok())
	{
		string eidstr = p->eid();
		_peers.push_back(*p);
		scr.debug(7, "peersinfo::add(): Adding peer %s %s", ipstr.c_str(), eidstr.c_str());
		return(p);
	}
	scr.error("peersinfo::add(): Cannot add peer %s", ipstr.c_str());
	return(nullptr);
}

string
peerinfo::print()
{
	string s = "";
	char	tmp[256];

	if (!this->ok())
		return("Invalid peer information");
	
	Fversion	version(_flags);
	Fdescriptor	descriptor(_flags);
	Fstream	stream(_flags);
	Ftxwilling	txwilling(_flags);
	Frxwilling	rxwilling(_flags);
	Fudptype	udptype(_flags);
	Ffreespace	advertise(_flags);
	// Ffreespaced	freespacedesc(_flags);

	string		version_str = "";
	string		descriptor_str = "";
	string		stream_str = "";
	string		txwilling_str = "";
	string		rxwilling_str = "";

	switch(version.get())
	{
	case F_VERSION_0:
		version_str = "V0";
		break;
	case F_VERSION_1:
		version_str = "V1";
	}

	switch(descriptor.get())
	{
	case F_DESCRIPTOR_16:
		descriptor_str = "16";
		break;
	case F_DESCRIPTOR_32:
		descriptor_str = "32";
		break;
	case F_DESCRIPTOR_64:
		descriptor_str = "64";
		break;
	case F_DESCRIPTOR_128:
#ifdef UINT128_T
		descriptor_str += "128";
#else
		descriptor_str += "128 not supported";
#endif
		break;
	}

	switch(stream.get())
	{
	case F_STREAMS_NO:
		stream_str = "No";
		break;
	case F_STREAMS_YES:
		stream_str = "Yes";
		break;
	}

	switch(txwilling.get())
	{
	case F_TXWILLING_NO:
		txwilling_str = "No";
		break;
	case F_TXWILLING_YES:
		txwilling_str = "Yes";
		break;
	case F_TXWILLING_CAPABLE:
		txwilling_str = "Cap";
		break;
	case F_TXWILLING_INVALID:
		txwilling_str = "???";
		break;
	}

	switch(rxwilling.get())
	{
	case F_RXWILLING_NO:
		rxwilling_str = "No";
		break;
	case F_RXWILLING_YES:
		rxwilling_str = "Yes";
		break;
	case F_RXWILLING_CAPABLE:
		rxwilling_str = "Cap";
		break;
	case F_RXWILLING_INVALID:
		rxwilling_str = "???";
		break;
	}

	string addr = _ip.straddr();
	if (advertise.get() == F_FREESPACE_YES)
		sprintf(tmp, "Peer=%-40.40s\n  EID=%-50.50s\n  Ver=%-4.4s Des=%-4.4s Streams=%-5.5s TxWill=%-5.5s RxWill=%-5.5s Free(MB)=%-12" PRIu64 "\n",
			addr.c_str(),
			_eid.c_str(),
			version_str.c_str(),
			descriptor_str.c_str(),
			stream_str.c_str(),
			txwilling_str.c_str(),
			rxwilling_str.c_str(),
			_freespace / 1000);
	else
		sprintf(tmp, "Peer=%-40.40s\n  EID=%-50.50s\n  Ver=%-4.4s Des=%-4.4s Streams=%-5.5s TxWill=%-5.5s RxWill=%-5.5s Free(MB)=%-12.12s\n",
			addr.c_str(),
			_eid.c_str(),
			version_str.c_str(),
			descriptor_str.c_str(),
			stream_str.c_str(),
			txwilling_str.c_str(),
			rxwilling_str.c_str(),
			"Not Advert");
	s = tmp;
	return(s);
}
string
peersinfo::print()
{
	string s = "";
	
	for (std::list<peerinfo>::iterator i = _peers.begin(); i != _peers.end(); i++)
		s += i->print();
	return(s);
}

}; // Namespace saratoga

