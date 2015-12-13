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

// Command Line Interpretor for saratoga

// #include <iostream>
#include <cstring>
#include <string>
#include <stdio.h>
#include <vector>
#include <sys/types.h>
#include "screen.h"
#include "ip.h"
#include "cli.h"
#include "saratoga.h"
#include "sarflags.h"
#include "timestamp.h"
#include "fileio.h"
#include "beacon.h"
#include "request.h"
#include "tran.h"
#include "globals.h"


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

#include <time.h>





using namespace std;

// A command has a name, a usage a help and a function to call


namespace saratoga
{

// Given a wakeup period see if the
// beacon timer has expired and if so return true
bool
cli_beacon::timerexpired()
{
	extern timestamp	curzulu; // The current time
	timestamp	*then = this->timer();
	timestamp	diff = curzulu - *then;

	// No timer so always false
	if (_secs == 0)
		return(false);
	// If our timer has expired then reset it to now
	if (diff.secs() >= _secs)
	{
		this->settimer(curzulu);
		return(true);
	}
	// Nope timer has not expired yet
	return(false);
}


/*
 * Send beacons out
 */
bool
cli_beacon::execute()
{
	string	s;

	string 	eidstr;

	frame*	f;
	sardir::fsinfo *fs = new sardir::fsinfo(".");

// AX25

	if(sarnet::ax25::isax25available()){
		// We always want the AX25 Address in the eid
		eidstr = sarnet::ax25::getax25srcaddr();
		eidstr += " ";
		eidstr += c_eid.eid();
		// Do we wish to advertise free space
		if (c_freespace.flag() == F_FREESPACE_YES)
		{
			f = new beacon(
				c_descriptor.flag(),
				c_stream.flag(),
				c_tx.flag(),
				c_rx.flag(),
				fs,
				eidstr);
		}
		else
		{
			f = new beacon(
			c_descriptor.flag(),
			c_stream.flag(),
			c_tx.flag(),
			c_rx.flag(),
			eidstr);
		}


		if (f->tx(ax25multi) < 0)
		 	scr.error("Cant queue a multicast ax25 beacon");
		else
			scr.msgout("cli_beacon::execute() Sending ax25 Multicast Beacon EID %s", eidstr.c_str());
		delete f;

	}





	// Assemble & Send Multicast IPv4 Beacon
	// The EID always contains the local IPv4 Address for v4 beacons
	if (this->v4flag())
	{
		// We always want the IP Address in the eid
		eidstr = v4out->straddr();
		eidstr += " ";
		eidstr += c_eid.eid();
		// Do we wish to advertise free space
		if (c_freespace.flag() == F_FREESPACE_YES)
		{
			f = new beacon(
				c_descriptor.flag(),
				c_stream.flag(),
				c_tx.flag(),
				c_rx.flag(),
				fs,
				eidstr);
		}
		else
		{
			f = new beacon(
			c_descriptor.flag(),
			c_stream.flag(),
			c_tx.flag(),
			c_rx.flag(),
			eidstr);
		}
		if (f->tx(v4mcastout) < 0)
		 	scr.error("Cant queue a multicast v4 beacon");
		else
			scr.msgout("cli_beacon::execute() Sending IPv4 Multicast Beacon EID %s", eidstr.c_str());
		delete f;
	}

	// Assemble & Send Multicast IPv6 Beacon 
	// The EID always contains the local IPv6 Address for v6 beacons
	if (this->v6flag())
	{
		eidstr = v6out->straddr();
		eidstr += " ";
		eidstr += c_eid.eid();
		// Do we wish to advertise free space
		if (c_freespace.flag() == F_FREESPACE_YES)
		{
			f = new beacon(
				c_descriptor.flag(),
				c_stream.flag(),
				c_tx.flag(),
				c_rx.flag(),
				fs,
				eidstr);
		}
		else
		{
			f = new beacon(
			c_descriptor.flag(),
			c_stream.flag(),
			c_tx.flag(),
			c_rx.flag(),
			eidstr);
		}
		if (f->tx(v6mcastout) < 0)
		 	scr.error("Cant queue a multicast v6 beacon");
		else
			scr.msgout("cli_beacon::execute() Sending IPv6 Multicast Beacon EID %s", eidstr.c_str());
		delete f;
	}

	// Assemble & Send Unicast Beacons to the current list of peers
	for (std::list<sarnet::ip>::iterator p = c_beacon.begin(); p != c_beacon.end(); p++)
	{
		string ipaddr = p->straddr();
		
		eidstr = ipaddr;
		eidstr += " ";
		eidstr += c_eid.eid();
		if (c_freespace.flag() == F_FREESPACE_YES)
		{
			f = new beacon(
				c_descriptor.flag(),
				c_stream.flag(),
				c_tx.flag(),
				c_rx.flag(),
				fs,
				eidstr);
		}
		else
		{
			f = new beacon(
			c_descriptor.flag(),
			c_stream.flag(),
			c_tx.flag(),
			c_rx.flag(),
			eidstr);
		}
		// Is our peer in the current list of open sockets
		// If not then create the new peer and open one to it
		sarnet::udp	*pm;

		if ((pm = sarpeers.match(ipaddr)) == nullptr)
		{
			sarpeers.add(ipaddr, sarport);
			if ((pm = sarpeers.match(ipaddr)) == nullptr)
			{
				scr.error("Can't create new socket to %s", ipaddr.c_str());
				continue;
			}
		}
		// Send the beacon to the unicast socket
		int plen;
		if ((plen = f->tx(pm)) >= 0)
			scr.msgout("cli_beacon::execute() Sending a Unicast Beacon to %s Length %d EID %s", 
				ipaddr.c_str(), plen, eidstr.c_str());
		else
			scr.error("Can't send unicast beacon to %s", ipaddr.c_str());
		delete f;
	}
	delete fs;
	return(true);
}

// Get a file send the REQUEST
bool
cli_get::execute()
{

	scr.debug(2, "cli_get::execute(): write some code!!!");
	this->ready(false);
	return(false);
}

// Get then remove a file send the REQUEST
bool
cli_getrm::execute()
{

	scr.debug(2, "cli_getrm::execute(): write some code!!!");
	this->ready(false);
	return(false);
}

// Put a file - Create the transfer and send the initial REQUEST
bool
cli_put::execute()
{
	frame	*f;

	session_t sess = c_session.set();
	string fname = c_put.fname();
	string localfname = c_put.localfname();
	sarnet::ip *ipaddr = c_put.peer();
	string ipstr = ipaddr->straddr();
	sarnet::udp	*pm;

	scr.debug(2, "cli_put::execute(): fname=%s ip=%s",
		fname.c_str(), ipstr.c_str());

	// Is our local file OK to read and there ?
	sarfile::fileio *locfp = new sarfile::fileio(localfname, sarfile::FILE_READ);
	if (!locfp->ok() || !locfp->isfile())
	{
		scr.error("Unable to open local file %s for transfer",
			localfname.c_str());
		this->ready(false);
		delete locfp;
		return(false);
	}
	delete locfp;
	
	// All is good locally so create the REQUEST to send
	f = new request(F_REQUEST_PUT, sess, fname);
	// Make sure we have generated a good REQUEST frame
	if (f->badframe())
	{
		scr.error("Badly formed REQUEST frame");
		delete f;
		this->ready(false);
		return(false);
	}

	// Is our peer in the current list of open sockets
	// If not then create the new peer and open a socket to it
	if ((pm = sarpeers.match(ipaddr)) == nullptr)
	{
		if ((pm = sarpeers.add(ipaddr, sarport)) == nullptr)
		{
			scr.error("cli_put::execute(): Can't create new socket to %s", 
				ipstr.c_str());
			delete f;
			this->ready(false);
			return(false);
		}
	}

	// Everything is in place so create the transfer
	// and add it to the current list
	// We are the initiator of the transfer
	saratoga::request *rp = (saratoga::request *) f;
	if (sartransfers.add(OUTBOUND, TO_SOCKET, rp, pm, localfname) != nullptr)
		scr.debug(3, "cli_put::execute(): Frame is OK and transfer is OK");
	else
	{
		scr.debug(3, "cli_put::execute(): Frame or transfer is bad");
		this->ready(false);
		delete f;
		return(false);
	}

	// We have created the transfer and the frame is good
	// Send out the REQUEST to the destination socket
	int plen;
	if ((plen = f->tx(pm)) > 0)
	{
		scr.msgout("cli_put::execute(): Tx PUT REQUEST to %s for %s Length %d", 
			ipstr.c_str(), fname.c_str(), plen);
		delete f;
		this->ready(true);
		return(true);
	}
	else
	{
		scr.error("Can't send PUT REQUEST to %s for %s Length %d", 
			ipstr.c_str(),
			fname.c_str(),
			plen);
		delete f;
		this->ready(false);
		return(false);
	}
}

// Put then remove a file send the REQUEST
bool
cli_putrm::execute()
{

	scr.debug(2, "cli_putrm::execute(): write some code!!!");
	this->ready(false);
	return(false);
}

// Remove a file send the REQUEST
bool
cli_rm::execute()
{

	scr.debug(2, "cli_rm::execute(): write some code!!!");
	this->ready(false);
	return(false);
}

// List the directory send the REQUEST
bool
cli_ls::execute()
{

	scr.debug(2, "cli_ls::execute(): write some code!!!");
	this->ready(false);
	return(false);
}

// Remove the directory send the REQUEST
bool
cli_rmdir::execute()
{

	scr.debug(2, "cli_rmdir::execute(): write some code!!!");
	this->ready(false);
	return(false);
}

}; // namespace saratoga

