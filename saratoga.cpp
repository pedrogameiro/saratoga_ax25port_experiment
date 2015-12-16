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

#include <cstring>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/select.h>
#include <ctype.h>
#include <signal.h>
#include "screen.h"
#include "ip.h"
#include "saratoga.h"
#include "sarflags.h"
#include "timestamp.h"
#include "cli.h"
#include "globals.h"

#include "frame.h"
#include "beacon.h"
#include "request.h"
#include "data.h"
#include "metadata.h"
#include "status.h"



#include "holes.h"

using namespace std;

namespace saratoga
{

bool	resizeset = false;

// Work out what frame type we have read and handle it
// If the # if fd's change then return true so we know in our mainloop
// to redo the select()
bool
readhandler(sarnet::ip ip, char *buf, size_t len)
{
	flag_t	flags;
	sarnet::udp *sock; // Where we want to create a socket to for writing
	sarnet::ip	*ipaddr; // Source IP address 
	saratoga::tran	*t; // The applicable transfer a frame is received for

	// What is the source IP of this frame
	// Add it into our list of peers and open a socket to the peer (to)
	// if not already there
	ipaddr = &ip;
	string from = ipaddr->straddr();
	//ipaddr = new sarnet::ip(from);
	if ((sock = sarpeers.match(ipaddr)) == nullptr)
	{
		sarpeers.add(ipaddr, sarport);
		// Make SURE we have the socket in the list and it is open
		if ((sock = sarpeers.match(ipaddr)) == nullptr)
		{
			string socks = sock->print();
			scr.error("Readhandler: Cannot establish socket to %s", 
				socks.c_str());
			return false;
		}
	}

	if (len <= 4)
	{
		scr.error("Bad Frame: len %d", (int) len);
		return false;
	}

	memcpy(&flags, buf, sizeof(flag_t));
	flags = (flag_t) ntohl(flags);

	Fversion	version(flags);
	Fframetype	frametype(flags);
	Fdescriptor	descriptor(flags);

	if (version.get() != F_VERSION_1)
	{
		scr.error("Bad Frame: Not Saratoga Version 1");
		return false;
	}

	switch(frametype.get())
	{
	case F_FRAMETYPE_BEACON:
		saratoga::beacon *b;
		b = new saratoga::beacon(buf, len); // Decode the beacon frame
		if (b->badframe())
			scr.error("Rx malformed BEACON from %s", from.c_str());
		else
		{
			scr.msgin("Rx BEACON from %s", from.c_str());
			sarpeersinfo.add(ipaddr, b); // Get the beacon info from it
			scr.debug(7, b->print());
		}
		delete b;
		return false;
		break;
	case F_FRAMETYPE_REQUEST:
		saratoga::request *r;
		r = new request(buf, len);
		if (r->badframe())
			scr.error("Rx malformed REQUEST from %s", from.c_str());
		else
		{
			scr.msgin("Rx REQUEST from %s", from.c_str());
			if ((t = sartransfers.rxrequest(r, sock)) == nullptr)
				scr.error("Bad REQUEST cannot create transfer");
			else
			{
				t->transfer_reset();
				t->request_reset();
				t->sendstatus();
			}
			scr.debug(7, r->print());
		}
		delete r;
		return true;
		break;
	case F_FRAMETYPE_METADATA:
		saratoga::metadata *m;
		m = new metadata(buf, len);
		if (m->badframe())
			scr.error("Rx malformed METADATA from %s", from.c_str());
		else
		{
			scr.msgin("Rx METATDATA from %s", from.c_str());
			if ((t = sartransfers.rxmetadata(m, sock)) == nullptr)
				scr.error("Bad METADATA no such transfer");
			else
				t->sendstatus();
			scr.debug(7, m->print());
		}
		delete m;
		return false;
		break;
	case F_FRAMETYPE_DATA:
		saratoga::data *d;
		d = new data(buf, len);
		if (d->badframe())
			scr.error("Rx malformed DATA from %s", from.c_str());
		else
		{
			scr.msgin("Rx DATA from %s Length=%d Offset=%" PRIu64 "", 
				from.c_str(), d->dbuflen(), d->offset());

			if ((t = sartransfers.rxdata(d, sock)) == nullptr)
				scr.error("Bad DATA no such transfer");
			else
			{
				if (t->status_expired())
					t->sendstatus();
			}
			scr.debug(7, d->print());
		}
		delete d;
		return false;
		break;
 	case F_FRAMETYPE_STATUS:
		saratoga::status *s;
 		s = new status(buf, len);
		if (s->badframe())
		{
			scr.error("Rx malformed STATUS from %s", from.c_str());
			delete s;
			return false;
		}
		else
		{
			if ((t = sartransfers.rxstatus(s, sock)) == nullptr)
			{
				scr.error("Bad STATUS no such transfer");
				delete s;
				return false;
			}
			else
			{
				// Reset the STATUS timer
				t->status_reset();
		 		scr.msgin("Rx STATUS from %s ERRCODE=%s", 
					from.c_str(), s->errprint().c_str());
				// THIS IS WHERE WE HANDLE ALL OF THE STATUS
				// ERROR CODES
				if (s->errcode() != F_ERRCODE_SUCCESS)
				{
					// Lets just tell us for the moment
					scr.error("Received STATUS ERROR %s Removing transfer: %s",
						s->errprint().c_str(),
						t->print().c_str());
					sartransfers.remove(t);
					delete s;
					return true;
				}
				else
				{
					// All is done we have received the metadata and we have no holes
					// and the progress is the size of the file
					if (t->metadatarecvd() == F_METADATARECVD_YES &&
						s->holecount() == 0 &&
						s->progress() == t->localflen())
					{
						scr.msg("Received STATUS and completed transfer %" PRIu32 " from %s",
							t->session(), from.c_str());
						t->sendstatus(); // Send a status back to the other end to close its tfr
						// transfer is done so remove it
						sartransfers.remove(t);
						delete s;
						return true;
					}
					// We have holes, add them to the transfer for processing
					if (s->holecount() > 0)
					{
						if (s->allholes() == F_ALLHOLES_YES)
						{
							// All of the holes are in this STATUS frame for the transfer
							// Clear the existing holes (if any)
							// MMMMMMMMMM lets think about this some more!!!!!!!
							t->holes_clear();
							// Now Add the new holes that are in our STATUS frame
							t->holes_add(s->holesptr());
							scr.msg("Received all holes in STATUS transfer %" PRIu32 ": %s",
								t->session(), t->holes_print().c_str());
						}
						else
						{
							// The holes are to be added to the transfers current hole list
							t->holes_add(s->holesptr());
							scr.msg("Added more holes from STATUS transfer %" PRIu32 ": %s",
								t->session(), t->holes_print().c_str());
						}
					}
				}
			}
			scr.debug(7, s->print());
		}
		delete s;
		return false;
		break;
	default:
		scr.error("Rx Invalid Saratoga Frame Type from %s", from.c_str());
		break;
	}
	delete ipaddr;
	return false;
}

}; // namespace saratoga

// *************************************************************************************

using namespace saratoga;

// Command line completion
// Given a current command try and complete it
// as a <TAB> has been hit
// return the completed part of the command
string
completearg(string s)
{

	saratoga::cmds		c;
	string		ret = "";

	std::vector<string> arglist; // The current argument list

	splitargs(s, arglist);
	if (arglist.size() != 1)
		return "";
	string cmd = c.cmatch(arglist[0]);
	for ( unsigned int pos = s.length(); pos < cmd.length(); pos++)
		ret += cmd[pos];
	saratoga::scr.debug(2, "completearg(): Given <%s> Command <%s> Return <%s>",
		s.c_str(), cmd.c_str(), ret.c_str());	
	return(ret); 
}

// Handle ? command line completion
void
help(string s)
{
	saratoga::cmds	clist;
	std::vector<string> arglist;
	std::vector<string> matches;

	// Give me list of all commands
	if (s.length() == 0)
	{
		saratoga::cmd	c;
		saratoga::scr.std('\n');
		c.cmd_help();
		return;
	}
	// Give me usage on what matches current input s
	saratoga::scr.debug(2, "help(): Help for <%s>", s.c_str());
	splitargs(s, arglist);
	string smatches = clist.cmdmatches(arglist[0]);
	splitargs(smatches, matches);
	if (matches.size() > 0)
	{
		saratoga::scr.std('\n');
		// Loop around and show usage for each matching command
		for (unsigned int i = 0; i < matches.size(); i++)
		{
			saratoga::scr.std(clist.printusage(matches[i]));
			saratoga::scr.debug(2, "help(): Arglist[%d] is %s match for %s", i, arglist[0].c_str(), arglist[0].c_str());
		}
	}
	else
		saratoga::scr.std('\n');
		
}

// Initialise saratoga. Open the sockets
void
initialise(string logname, string confname)
{
	timestamp	curtime;

	// Read in the saratoga.conf configuration file
	readconf(confname);

	// Log file name
	if (logname == "")
		logname = "./saratoga.log";

	// Open the log file for writing
	sarlog = new sarfile::fileio(logname,  sarfile::FILE_WRITE);
	if (!sarlog->ok())
		saratoga::scr.fatal("Cannot open saratoga log file %s", logname.c_str());

	// Start time of this log
	char	str[128];
	sprintf(str, "\n####################SARATOGA STARTED####################\n%s", 
		curtime.printshort().c_str());
	saratoga::scr.msg(str);

	sarnet::netifaces	interfaces;
	saratoga::scr.debug(4, "initialise(): %s", interfaces.print().c_str());
	
	sarnet::netiface *firstv4iface = interfaces.first(AF_INET);
	sarnet::ip	*fv4 = new sarnet::ip(firstv4iface->ifip());
	saratoga::scr.msg("First IPv4 Physical Interface is %s", fv4->print().c_str());
	
	sarnet::netiface *firstv6iface = interfaces.first(AF_INET6);
	sarnet::ip	*fv6 = new sarnet::ip(firstv6iface->ifip());
	saratoga::scr.msg("First IPv6 Physical Interface is %s", fv6->print().c_str());
	
	if (fv4->straddr() == if_loop || fv6->straddr() == if6_loop)
	{
		saratoga::scr.msg("Saratoga must have a physical interface to run not just a loopback");
		sarlog->fflush();
		saratoga::scr.fatal("Saratoga requires a physical interface");
		sleep(3);
	}

	sarnet::ax25::initax25();

	// IPv4 listening input socket
	v4in = new sarnet::udp(AF_INET, sarport);
	if (v4in->fd() < 0)
		saratoga::scr.fatal("Can't bind ipv4 saratoga socket for listening\n");
	// IPv6 listening input socket
	v6in = new sarnet::udp(AF_INET6, sarport);
	if (v6in->fd() < 0)
		saratoga::scr.fatal("Can't bind ipv6 saratoga socket for listening\n");
	// IPv4 output socket we write to
	v4out = new	sarnet::udp(fv4->straddr(), sarport);
	if (v4out->fd() < 0)
		saratoga::scr.fatal("Can't bind to ipv4 saratoga local ip\n");
	// IPv6 output socket we write to
	v6out = new	sarnet::udp(fv6->straddr(), sarport);
	if (v6out->fd() < 0)
		saratoga::scr.fatal("Can't bind to ipv6 saratoga local ip\n");
	// IPv4 loopback we can write to
	v4loop = new	sarnet::udp(if_loop, sarport);
	if (v4loop->fd() < 0)
		saratoga::scr.fatal("Can't bind to ipv4 saratoga loopback ip\n");
	// IPv6 loopback we can write to
	v6loop  = new	sarnet::udp(if6_loop, sarport);
	if (v6loop->fd() < 0)
		saratoga::scr.fatal("Can't bind to ipv6 saratoga loopback ip\n");

	// Bind the output IPv4 multicast to the IPv4 output socket
	v4mcastout = new sarnet::udp(sarnet::MCAST_OUT, v4out, if_mcast, sarport);
	if (v4mcastout->fd() < 0)
	{
		saratoga::scr.error("Can't bind to ipv4 saratoga multicast out\n");
		saratoga::c_multicast.off();
	}
	else
		saratoga::c_multicast.on();

	// Bind the output IPv6 multicast to the IPv6 output socket
	v6mcastout  = new sarnet::udp(sarnet::MCAST_OUT, v6out, if6_mcast, sarport);
	if (v6mcastout->fd() < 0)
	{
		saratoga::scr.error("Can't bind to ipv6 saratoga multicast out\n");
		saratoga::c_multicast.off();
	}
	else
		saratoga::c_multicast.on();

	v4mcastin = new	sarnet::udp(sarnet::MCAST_IN, v4in, if_mcast, sarport);
	if (v4mcastin->fd() < 0)
		saratoga::scr.fatal("Can't bind to ipv4 saratoga multicast in\n");
	
	v6mcastin  = new sarnet::udp(sarnet::MCAST_IN, v6in, if6_mcast, sarport);
	if (v6mcastin->fd() < 0)
		saratoga::scr.fatal("Can't bind to ipv6 saratoga multicast in\n");

	// Print them all out
	saratoga::scr.msg("IPv4:\n Input %s", v4in->print().c_str());
	saratoga::scr.msg(" Output %s", v4out->print().c_str());
	saratoga::scr.msg(" Loopback %s", v4loop->print().c_str());
	if (c_multicast.state() == true)
	{
		saratoga::scr.msg(" Mcast In %s", v4mcastin->print().c_str());
		saratoga::scr.msg(" Mcast Out %s", v4mcastout->print().c_str());
	}
	saratoga::scr.msg("IPv6:\n Input %s", v6in->print().c_str());
	saratoga::scr.msg(" Output %s", v6out->print().c_str());
	saratoga::scr.msg(" Loopback %s", v6loop->print().c_str());
	if (c_multicast.state() == true)
	{
		saratoga::scr.msg(" Mcast In %s", v6mcastin->print().c_str());
		saratoga::scr.msg(" Mcast Out %s", v6mcastout->print().c_str());
	}
	sarlog->fflush(); // Make sure our log file is flushed
	saratoga::scr.msg("initialise(): %s", interfaces.print().c_str());
}

// Resize of screen signal caught. Flag for a redraw
void
resize(int sig)
{
	resizeset = true;
}

void
finalise(string config_file)
{
	// End time of this log
	timestamp	curtime;
	char	str[128];
	sprintf(str, "\n########################################################\n%s\n", 
		curtime.asctime().c_str());
	saratoga::scr.msg(str);
	sarlog->fwrite(str, strlen(str));
	sprintf(str, "####################SARATOGA ENDED OK###################\n");
	saratoga::scr.msg(str);

	// Write out the configuration file updating session
	writeconf(config_file);
	sarlog->fflush();
	sarlog->clear();
	sleep(5);
}

void
usage(string s)
{
	saratoga::scr.msg("\n");
	saratoga::scr.msg("usage: %s [-p <port>] [-l <logfile>] [-c <conffile>]", s.c_str());
}

// FLagged if the # of open file or peers change
// This then causes the and fd count to be 
// recalculated for select()
bool
fdchange()
{
	if (sarpeers.fdchange() || sarfiles.fdchange()) 
	{
		saratoga::scr.debug(2, "Change in # of fd's");
		sarpeers.fdchange(false); 
		sarfiles.fdchange(false); 
		return(true);
	}
	return(false);
}

//#ifdef SARATOGA
int 
main(int argc, char **argv)
{
	int		opt; // Command line options

	string args = "";	// Command line entry args
	string addargs = "";	// Add to args complete an arg
	std::vector<string> arglist; // List of arg words
	string logname = "../saratoga.log"; // Default log file name
	string confname = "../saratoga.conf"; // Default config file name


	// Handle command line input args
	// to set udp port log file and config file names
	// usage: saratoga [-p <	port>] [-l <logfile] [-c <conffile>]

	while ((opt = getopt(argc, argv, "l:c:p:")) != -1)
	{
		switch(opt)
		{
		case 'l':
			logname = optarg;
			break;
		case 'c':
			confname = optarg;
			break;
		case 'p':
			sarport = (int) strtol(optarg, NULL, 10);
			break;
		default:
			usage(argv[0]);
			sleep(5);
			exit(1);
		}
	}

	// Catch screen resizes
	signal(SIGWINCH, resize);

	fd_set	crfd;
	fd_set	cwfd;
	saratoga::cmds c;
	struct timespec		wakeup;
	int	inkey;
	int	selval;		// Value returned by pselect
	char	buf[9000];	// Current frame input buffer
	int	sz;		// Current frame size
	
	initialise(logname, confname);

	// Wake up every 5 seconds in select if required
	wakeup.tv_sec = 5; 
	wakeup.tv_nsec = 0; 

	// Initial Prompt
	saratoga::scr.prompt();
	saratoga::scr.std("Press ? for help");
	saratoga::scr.prompt();

	// Get set to read and write
	FD_ZERO(&crfd);
	FD_ZERO(&cwfd);

// We have four cases where we want to immediatly rerun the 
// FD_XXX's and select() loop due to the number of fd's changing
// These cases "may" be, which  is determined by sarpeers.fdchange())
//	1) when a command line has been entered initiating a session
//	2) when a new frame has been received e.g. request, beacon,
// 	   metadata that requires a new socket or file to be opened.
//	3) When a session has been finished or closed resulting in
//	   a file closure. - We do this in cleanup() - not written yet.
//	4) When there has been some sort of timeout on a transfer or
//	   overrun - not written yet
// Yes and when one of those cases happen we immediatley goto mainloop:
// I have tried so far to not have to do an setjmp's. So far so good!
// sarpeers.fdchange(true) & sarfiles.fdchange(true) are the triggers
// and they are all over the place.

static const offset_t TIMER_GRANULARITY = 1000;
	// # Of times we have gone around mainloop
	// Reset curzulu every TIMER_GRANULARITY iterations for timers
	// and set current Zulu time for timers
	offset_t loopcounter = 0;
	curzulu.setzulu();

	while (1)
	{
mainloop:
		// Has our signal handler caught a screen resize ?
		if (resizeset)
		{
			saratoga::scr.resize();
			resizeset = false;
			args = "";
			saratoga::scr.prompt();
		}

		// Get set to read and write
		FD_ZERO(&crfd);
		FD_ZERO(&cwfd);

		FD_SET(STDIN_FILENO, &crfd);
		FD_SET(v4in->fd(), &crfd);
		v4in->ready(true);
		FD_SET(v6in->fd(), &crfd);
		v6in->ready(true);
		FD_SET(v4mcastin->fd(), &crfd);
		v4mcastin->ready(true);
		FD_SET(v6mcastin->fd(), &crfd);
		v6mcastin->ready(true);

		// AX25
		if(sarnet::ax25::ax25available){
			FD_SET(ax25multiin->fd(), &crfd);
			ax25multiin->ready(true);
			(ax25multiout->ready()) ? FD_SET(ax25multiout->fd(), &cwfd) : FD_CLR(ax25multiout->fd(), &cwfd);

		}

		(sarlog->ready()) ? FD_SET(sarlog->fd(), &cwfd) : FD_CLR(sarlog->fd(), &cwfd);	
		(v4out->ready()) ? FD_SET(v4out->fd(), &cwfd) : FD_CLR(v4out->fd(), &cwfd);
		(v6out->ready()) ? FD_SET(v6out->fd(), &cwfd) : FD_CLR(v6out->fd(), &cwfd);
		(v4mcastout->ready()) ? FD_SET(v4mcastout->fd(), &cwfd) : FD_CLR(v4mcastout->fd(), &cwfd);
		(v6mcastout->ready()) ? FD_SET(v6mcastout->fd(), &cwfd) : FD_CLR(v6mcastout->fd(), &cwfd);
		(v4loop->ready()) ? FD_SET(v4loop->fd(), &cwfd) : FD_CLR(v4loop->fd(), &cwfd);
		(v6loop->ready()) ? FD_SET(v6loop->fd(), &cwfd) : FD_CLR(v6loop->fd(), &cwfd);

		// Are we ready to write to peers
		for (std::list<sarnet::udp>::iterator p = sarpeers.begin(); p != sarpeers.end(); p++)
			(p->ready()) ? FD_SET(p->fd(), &cwfd) : FD_CLR(p->fd(), &cwfd);
		
		// Are we ready to read or write to the local files
		for (std::list<saratoga::tran>::iterator tr = sartransfers.begin(); tr != sartransfers.end(); tr++)
		{
			sarfile::fileio *f = &(*tr->local());
			bool ready = f->ready();
			if (!ready)
				continue;
			enum sarfile::rorw mode = f->rorw();
			switch(mode)
			{
			case sarfile::FILE_WRITE:
			case sarfile::FILE_EXCL:
				(ready) ?  FD_SET(f->fd(), &cwfd) : FD_CLR(f->fd(), &cwfd);
				break;
			case sarfile::FILE_READ:
				(ready) ?  FD_SET(f->fd(), &crfd) : FD_CLR(f->fd(), &crfd);
				break;
			default:
				saratoga::scr.error("main(): Cannot FD_SET/FD_CLR Undefined mode for %s",
					f->print().c_str());
			}
		}

		selval = pselect(saratoga::maxfd() + 1, &crfd, &cwfd, nullptr, &wakeup, nullptr);
		switch(selval)
		{
		case -1:
			saratoga::scr.perror(errno, "Failure in select");
			goto mainloop;
		case 0: // Timeout expired just go around again
			saratoga::scr.debug(9, "main(): Select Timeout");
			loopcounter = 0;
			curzulu.setzulu(); // Reset Zulu time again
			break;
		default:
			saratoga::scr.debug(9, "main(): Number of FD Sets Changed %d", selval);
			// Reget Zulu Time every 1000 iterations
			// We don't want to waste CPU cycles getting the current time too often
			if (++loopcounter > TIMER_GRANULARITY)
			{
				saratoga::scr.debug(7, "main(): Reset curzulu Time");
				loopcounter = 0;
				curzulu.setzulu();
			}
			break;
		}

		// Send beacon's
		if (saratoga::c_beacon.ready() && beacontimer.timedout())
		{
			if (saratoga::c_beacon.execute()) 
			{ 
				if (fdchange())
					goto mainloop;
			}
			else
				saratoga::scr.error("Could not send beacon");
			beacontimer.reset(); //alterado //TODO Ele nunca chega aqui!??

		}

		// Handle al of the c_xxxxxx these are the results of
		// command line inputs

		// We wish to exit (exit
		if (saratoga::c_exit.flag() >= 0)
		{
			saratoga::scr.msg("Exit Flag Set Bye!!");
			goto closedown;
		}

		// The xxxxx.execute() functions initiiate the outbound transfers 
		// by sending out REQUEST's
		// All these commands may change the # of fd's so we have to jump back
		// to the top of our main loop to recalc maxfd() and select()

		// Put a file
		if (saratoga::c_put.ready())
		{
			saratoga::scr.debug(7, "main(): Before Put execute");
			if (saratoga::c_put.execute()) 
			{
				saratoga::scr.debug(7, "main(): After Put execute");
				saratoga::c_put.ready(FALSE);
				if (fdchange())
					goto mainloop;
			}
			else
				saratoga::scr.error("Could not send put");
		}
		
		// Put then remove a file
		if (saratoga::c_putrm.ready())
		{
			if (saratoga::c_putrm.execute()) 
			{
				saratoga::c_putrm.ready(FALSE);
				if (fdchange())
					goto mainloop;
			}
			else
				saratoga::scr.error("Could not send putrm");
		}

		// Get a file
		if (saratoga::c_get.ready())
		{
			if (saratoga::c_get.execute()) 
			{
				saratoga::c_get.ready(FALSE);
				if (fdchange())
					goto mainloop;
			}
			else
				saratoga::scr.error("Could not send get");
		}

		// Get then remove file
		if (saratoga::c_getrm.ready())
		{
			if (saratoga::c_getrm.execute()) 
			{
				saratoga::c_getrm.ready(FALSE);
				if (fdchange())
					goto mainloop;
			}
			else
				saratoga::scr.error("Could not send getrm");
		}

		// Remove a file
		if (saratoga::c_rm.ready())
		{
			if (saratoga::c_rm.execute()) 
			{
				saratoga::c_rm.ready(FALSE);
				if (fdchange())
					goto mainloop;
			}
			else
				saratoga::scr.error("Could not send rm");
		}

		// Get directory listing
		if (saratoga::c_ls.ready())
		{
			if (saratoga::c_ls.execute()) 
			{
				saratoga::c_ls.ready(FALSE);
				if (fdchange())
					goto mainloop;
			}
			else
				saratoga::scr.error("Could not send ls");
		}


		// Remove directory
		if (saratoga::c_rmdir.ready())
		{
			if (saratoga::c_rmdir.execute()) 
			{
				saratoga::c_rmdir.ready(FALSE);
				if (fdchange())
					goto mainloop;
			}
			else
				saratoga::scr.error("Could not send rmdir");
		}

		// CLI Inputs to stdin i.e. Keyboard input
		if (FD_ISSET(STDIN_FILENO, &crfd))
		{
			inkey = getch(); 
			switch(inkey)
			{
			// Process the input args
			case ERR:
				break;

			case KEY_RESIZE:
				break;

			case KEY_ENTER:
			case CHAR_CR:
				saratoga::scr.moveeol(args);
				saratoga::scr.std('\n');
				// Handle !, !!, !n command repeat
				if (args[0] == '!')
				{	
					// "!!" - repeat last command
					if (args.size() == 2 && args[1] == '!')
					{
						saratoga::scr.prompt();
						args = saratoga::scr.uarrow();
						break;
					}
					// "!n" repat command # n
					if (args.size() == 2 || args.size() == 3) 
					{
						unsigned int n = asciitoint(&args[1], args.size() - 1);
						saratoga::scr.msg(c_history.print());
						saratoga::scr.prompt();
						args = c_history.get(n);
						saratoga::scr.stdnonl(args);
						break;
					}
				}
				c_history.push(args);
				splitargs(args, arglist);
				// Nothing there just hit cr
				if (arglist.size() < 1)
				{
					saratoga::scr.prompt();
					args = "";
					break; // goto mainloop;
				}
				// We have something
				// Check & RUN the command with the arglist
				if (c.cmdstr(arglist[0]) == "" || !(c.runcmd(arglist)))
				{
					string instr = arglist[0] + ": invalid command";
					saratoga::scr.info(instr);
					saratoga::scr.prompt();
					args = "";
					// If # fd's changed due to command line inputs
					// then immediatley rerun select 
					if (fdchange())
						goto mainloop;
					break;
				}
				saratoga::scr.prompt();
				args = "";
				break;

			// Quit
			case CHAR_CTL_C:
			case CHAR_CTL_D:
				saratoga::scr.msg("Read EOF Goodbye!!");
				goto closedown;

			// Process a backspace
			case CHAR_DELETE:
			case KEY_BACKSPACE:
				args = saratoga::scr.backspace(args);
				break; // goto mainloop;

			// Tabs try and complete the current command line word
			case CHAR_TAB:
				addargs = completearg(args);
				if (addargs != "" )
				{
					args += addargs;
					saratoga::scr.stdnonl(addargs);
					addargs = "";
				}
				break;
		
			// Up arrow hit to repeat last command
			case KEY_UP:
				// Copy previous command to this command 
				args = saratoga::scr.uarrow();
				break;

			// Left arrow move left to edit existing command
			case KEY_LEFT:
				saratoga::scr.larrow(args);
				break;

			// Right Arrow move right to edit existing command
			case KEY_RIGHT:
				saratoga::scr.rarrow(args);
				break;

			// Down arrow - Erase the command
			case KEY_DOWN:
				saratoga::scr.darrow();
				args = "";
				break;

			// Help within a command
			case CHAR_QUESTION:
				help(args);
				saratoga::scr.prompt();
				saratoga::scr.stdnonl(args);
				break;

			// Typed in a character add it to command
			default:
				if (isprint(inkey))
				{
					char ckey = (char) inkey;
					args = saratoga::scr.insertkey(args, ckey);
					saratoga::scr.debug(9, "main(): Key <%d> <%c> entered",
						inkey, ckey);
				}
				else
					saratoga::scr.error("Invalid character <%d> entered", inkey);
				break;
			}
		}

		// Now go through all of the open fd's and handle I/O on them if they are ready
	
		// Handle V4 Input frames
		if (FD_ISSET(v4in->fd(), &crfd))
		{
			sarnet::ip	*from = new sarnet::ip();
			sz = v4in->rx(buf, from);
			string	s = from->straddr();
			saratoga::scr.debug(7, "main(): v4in Read %d bytes from %s",
				sz, s.c_str());
			if (saratoga::readhandler(from, buf, sz) && fdchange()){
				delete from;
				goto mainloop;
			}else
				delete from;

		}
		// Handle V4 Input Multicast frames
		if (FD_ISSET(v4mcastin->fd(), &crfd))
		{
			sarnet::ip	*from = new sarnet::ip();
			sz = v4mcastin->rx(buf, from);
			string	s = from->straddr();
			saratoga::scr.debug(7, "main(): v4mcastin Read %d bytes from %s",
				sz, s.c_str());
			if (saratoga::readhandler(from, buf, sz) && fdchange()){
				delete from;
				goto mainloop;
			}else
				delete from;
		}
		// Handle AX25 Input Multicast frames
		//if (FD_ISSET(v4mcastin->fd(), &crfd))
		if(sarnet::ax25::ax25available && FD_ISSET(ax25multiout->fd(), &crfd))
		{
			sarnet::ip *from = new sarnet::ip();
			sz = ax25multiout->rx(buf,from);
			string	s = from->straddr();
			saratoga::scr.debug(7, "main(): ax25castin Read %d bytes from %s",
				sz, s.c_str());
			if (saratoga::readhandler(from, buf, sz) && fdchange()){
				delete from;
				goto mainloop;
			}else
				delete from;
		}
		// Handle V6 Input frames
		if (FD_ISSET(v6in->fd(), &crfd))
		{
			sarnet::ip	*from = new sarnet::ip();
			sz = v6in->rx(buf, from);
			string	s = from->straddr();
			saratoga::scr.debug(7, "main(): v6in Read %d bytes from %s",
				sz, s.c_str());
			if (saratoga::readhandler(from, buf, sz) && fdchange()){
				delete from;
				goto mainloop;
			}else
				delete from;
		}
		// Handle V6 Input Multicast frames
		if (FD_ISSET(v6mcastin->fd(), &crfd))
		{
			sarnet::ip	*from = new sarnet::ip();
			sz = v6mcastin->rx(buf, from);
			string	s = from->straddr();
			saratoga::scr.debug(7, "main(): v6mcastin Read %d bytes from %s",
				sz, s.c_str());
			if (saratoga::readhandler(from, buf, sz) && fdchange()){
				delete from;
				goto mainloop;
			}else
				delete from;
		}



		// Multicast Outputs
		if (c_multicast.state() == true)
		{
			// Send AX25 Multicast stuff.
			if (sarnet::ax25::ax25available && FD_ISSET(ax25multiout->fd(), &cwfd)){

				if ((sz = ax25multiout->send()) > 0)
					saratoga::scr.debug(7, "main(): ax25multiout Wrote %d bytes", sz);
				else
					ax25multiout->ready(false);
			}

			// Handle V4 Multicast Output frames
			if (FD_ISSET(v4mcastout->fd(), &cwfd))
			{
				if ((sz = v4mcastout->send()) > 0)
					saratoga::scr.debug(7, "main(): v4mcastout Wrote %d bytes", sz);
				else
					v4mcastout->ready(false);
			}

			// Handle V6 Multicast Output frames
			if (FD_ISSET(v6mcastout->fd(), &cwfd))
			{
				if ((sz =  v6mcastout->send()) > 0)
					saratoga::scr.debug(7, "main(): v6mcastout Wrote %d bytes", sz);
				else
					v6mcastout->ready(false);
			}
		}

		// Handle V4 Loopback Output frames
		if (FD_ISSET(v4loop->fd(), &cwfd))
		{
			if ((sz = v4loop->send()) > 0)
				saratoga::scr.debug(7, "main(): v4loop Wrote %d bytes", sz);
			else
				v4loop->ready(false);
		}

		// Handle V6 Loopback Output frames
		if (FD_ISSET(v6loop->fd(), &cwfd))
		{
			if ((sz = v6loop->send()) > 0)
				saratoga::scr.debug(7, "main(): v6loop Wrote %d bytes", sz);
			else
				v6loop->ready(false);
		}

		// Handle V4 Output frames - Yes to ourself
		if (FD_ISSET(v4out->fd(), &cwfd))
		{
			if ((sz = v4out->send()) > 0)
				saratoga::scr.debug(7, "main(): v4out Wrote %d bytes", sz);
			else
				v4out->ready(false);
		}

		// Handle V6 Output frames - Yes to ourself
		if (FD_ISSET(v6out->fd(), &cwfd))
		{
			if ((sz = v6out->send()) > 0)
				saratoga::scr.debug(7, "main(): v6out Wrote %d bytes", sz);
			else
				v6out->ready(false);
		}

		// Send frames out to our current peers as required
		for (std::list<sarnet::udp>::iterator p = sarpeers.begin(); p != sarpeers.end(); p++)
		{
			if (FD_ISSET(p->fd(), &cwfd))
			{
				if ((sz = p->send()) > 0)
				{
					saratoga::scr.debug(7, "main(): Wrote %d bytes to peer %s",
						sz, p->straddr().c_str());
				}
				else
					p->ready(false);
			}
		}

		// Write out log file information
		if (FD_ISSET(sarlog->fd(), &cwfd))
			sarlog->fflush();

		// Write to and read from currently open files as required
		for (std::list<saratoga::tran>::iterator tr = sartransfers.begin(); tr != sartransfers.end(); tr++)
		{
			sarfile::fileio *f = &(*tr->local());
			saratoga::tran *t = sartransfers.match(&(*f));
			if (t == nullptr)
				continue;
			saratoga::scr.debug(7, "main(): Looking at %s", f->print().c_str());
			// If our status timer has expired then send one
			if (t != nullptr && t->ready() && t->status_expired())
				t->sendstatus();
			switch(f->rorw())
			{
			case sarfile::FILE_WRITE:
			case sarfile::FILE_EXCL:
				// Write to local file
				if (FD_ISSET(f->fd(), &cwfd))
				{
					saratoga::scr.msg("Ready to flush %s", f->print().c_str());
					f->fflush();
				}
				break;
			case sarfile::FILE_READ:
				// Read from local file
				if (FD_ISSET(f->fd(), &crfd))
				{
					if ((t == nullptr) || !t->ready())
					{
						// We aren;t ready to send data yet
						// We havn't got a status frame
						f->ready(false);
						saratoga::scr.debug(2, "main(): Not ready to send data yet");
						break;
					}
					else
						f->ready(true);
					// Read maximum buffer we can from local file 
					if ((sz = f->read(c_maxbuff.get())) > 0)
					{
						saratoga::tran *t;
						saratoga::scr.debug(7, "main(): Read %d bytes from file %s",
							sz, f->fname().c_str());
						if ((t = sartransfers.match(&(*f))) != nullptr)
							t->senddata(f->buffers());
						else
							saratoga::scr.error("main(): No matching transfer for fd %d file %s",
								f->fd(), f->fname().c_str());
					}
					else
						f->ready(false); // EOF
				}
				break;
			default:
				saratoga::scr.error("main(): Cannot FD_ISSET Undefined mode for %s",
					f->print().c_str());
			}
		}
	} // END OF THE MAIN LOOP

closedown:
	// Clear all the sockets we are writing to
	for (std::list<sarnet::udp>::iterator p = sarpeers.begin(); p != sarpeers.end(); p++)
		saratoga::scr.msg("Removing Peer %s", p->print().c_str());
	// Crear all the local files we are reading or writing too
	for (std::list<sarfile::fileio>::iterator f = sarfiles.begin(); f != sarfiles.end(); f++)
	{
		saratoga::scr.msg("Flushing File %s", f->print().c_str());
		f->fflush();
	}

	sartransfers.zap();

	finalise(confname);
	return(saratoga::c_exit.flag());
}

/*
 * This is the Saratoga transfer protocol Vallona implementation,
 * under development by Charles Smith.
 *
 */

//#endif
