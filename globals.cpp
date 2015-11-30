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
#include <vector>
#include <list>
#include <sys/types.h>
#include "screen.h"
#include "ip.h"
#include "fileio.h"
#include "sarflags.h"
#include "timestamp.h"
#include "cli.h"
#include "peerinfo.h"
#include "beacon.h"
#include "tran.h"

/*
 * Classes and variables used globally
 */
namespace saratoga
{

sarwin::screen	scr; // The curses output interface

cli_beacon	c_beacon;
cli_exit	c_exit;
cli_checksum	c_checksum;
cli_debug	c_debug;
cli_descriptor	c_descriptor;
cli_eid		c_eid;
cli_freespace	c_freespace;
cli_get		c_get;
cli_getrm	c_getrm;
cli_history	c_history;
cli_home	c_home;
cli_ls		c_ls;
cli_put		c_put;
cli_putrm	c_putrm;
cli_rm		c_rm;
cli_rmdir	c_rmdir;
cli_rx		c_rx;
cli_session	c_session;
cli_stream	c_stream;
cli_timer	c_timer;
cli_timestamp	c_timestamp;
cli_timezone	c_timezone;
cli_transfers	c_transfers;
cli_tx		c_tx;
cli_multicast	c_multicast;
cli_prompt	c_prompt;
cli_maxbuff	c_maxbuff;

/* iana assigned saratoga IP addresses & udp port number */
uint16_t sarport = 7542;
string if_loop = "127.0.0.1";
string if6_loop = "::1";
string if_mcast = "224.0.0.108";
string if6_mcast = "FF02:0:0:0:0:0:0:6c";

/* permanently opend sockets */
sarnet::udp*	v4out;
sarnet::udp*	v4mcastout;
sarnet::udp*	v6out;
sarnet::udp*	v6mcastout;

sarnet::udp*	v4in;
sarnet::udp*	v6in;
sarnet::udp*	v4loop;
sarnet::udp*	v6loop;
sarnet::udp*	v4mcastin;
sarnet::udp*	v6mcastin;

// Saratoga log file
sarfile::fileio*	sarlog;

// The current Zulu time - Used for various timers
saratoga::timestamp	curzulu;

// Dynamic List of current open udp sockets
sarnet::peers		sarpeers;
// Dynamic list of curent open local file descriptors
sarfile::files		sarfiles;
// Information on each peer (from beacons)
saratoga::peersinfo	sarpeersinfo;

// Dynamic List of current transfers in progress
saratoga::transfers	sartransfers;

// Beacon Timer every n secs
timer_group::timer	beacontimer(0);

// return the largest currently open fd - used by select()
int
maxfd()
{
	int lfilesfd = sarfiles.largestfd();
	int lpeersfd = sarpeers.largestfd();
	return (lpeersfd > lfilesfd) ? lpeersfd : lfilesfd;
}

// Convert an ascii string of len bytes to integer
// return 0 if is invalid
unsigned int
asciitoint(const char *c, const int len)
{
	string str = "";
	for (int i = 0; i < len; i++)
	{
		if (*c < '0' || *c > '9')
			return 0;
		str += c;
		c++;
	}
	return(atoi(str.c_str()));	
}

// Print out a binary buffer in hex
string
printhex(const char *buf, const size_t len)
{
	string s = "";
	// Format the hex string up so we have 16 chars per line
	// and make sure everyone know it is in hex
	if (len == 0)
		return s;
	for (size_t i = 0; i < len; i++)
	{
		if (i == 0)
			s += "0x:";
		if (i % 16 == 0 && i != 0)
			s += "\n            ";
		char	h[10];
		// Last hex char dones not have a :
		int hint = (int) buf[i];
		if ( i != len - 1)
			sprintf(h,"%02X:", hint);
		else
			sprintf(h,"%02X", hint);
		s += h;
	}
	return s;
}

// Print out a string in hex
string
printhex(const string buf)
{
	string s = "";
	// Format the hex string up so we have 16 chars per line
	// and make sure everyone know it is in hex
	if (buf.length() == 0)
		return s;
	for (size_t i = 0; i < buf.length(); i++)
	{
		if (i == 0)
			s += "0x:";
		if (i % 16 == 0 && i != 0)
			s += "\n            ";
		char	h[10];
		// Last hex char dones not have a :
		if ( i != buf.length() - 1)
			sprintf(h, "%02X:", (int) buf[i]);
		else
			sprintf(h, "%02X", (int) buf[i]);
		s += h;
	}
	return s;
}

// Split a string into a vector of strings
// delimiter between is space
void
splitargs(string s, std::vector<string> &x)
{
	string curarg = "";
	char	prevc = ' ';

	// Get rid of what was there previously
	if (x.size() > 0)		
		x.erase(x.begin(), x.end());
	for (size_t i = 0; i < s.size(); i++)
	{
		if (s[i] == ' ')
		{
			if (prevc == ' ')
				continue;
			if (curarg != "")
			{
				x.push_back(curarg);
				curarg = "";
			}
		}
		else
		{
			prevc = s[i];
			curarg += s[i];
		}
	}
	if (curarg != "")
		x.push_back(curarg);
}

}; // namespace saratoga

