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

#include <iostream>
#include <cstring>
#include <string>
#include <limits>
#include <sys/statvfs.h>
#include "saratoga.h"
#include "sarflags.h"
#include "screen.h"
#include "timestamp.h"
#include "checksum.h"
#include "frame.h"
#include "beacon.h"
#include "data.h"
#include "dirent.h"
#include "metadata.h"
#include "request.h"
#include "ip.h"
#include "holes.h"
#include "tran.h"
#include "globals.h"
#include "timer.h"

using namespace std;

namespace saratoga
{

// We have a request. Create the transfer
tran::tran(saratoga::requestor inorout, 
	saratoga::direction dir, 
	saratoga::request *req, 
	sarnet::udp *sock,
	string localfname)
{
	string		s = "";

	// Prime the timers with the currently set cli variables
	_transfertimer = timer_group::timer(c_timer.transfer());
	_requesttimer = timer_group::timer(c_timer.request());
	_statustimer = timer_group::timer(c_timer.status());

	_requestor = inorout;

	if (sock == nullptr)
	{
		scr.error("Cannot initiate transfer - No return socket");
		return;
	}
	_peer = sock;

	_session = req->session();
	_curprogress = 0;
	_inresponseto = 0;
	
	// Grab the flags we need	
	_version = req->flags();
	_descriptor = req->flags();
	_stream = req->flags();
	_udptype = req->flags();
	_requesttype = req->flags();
	_errcode = F_ERRCODE_SUCCESS;
	_progress = F_PROGRESS_INPROG; // For METADATA we are in progress

	// Do we want to transmit timestamps for this transfer
	_reqtstamp = c_timestamp.flag();
	_timetype = c_timestamp.ttype();
	// Set these to now for the moment
	_timestamp = timestamp(); // Last timestmap transmitted in DATA or STATUS
	_lastrxtstamp = timestamp(); // Last rx timestamp in DATA or STATUS

	_offset = 0; // We are at the start of our transfer
	_holes.clear();
	_completed.clear();
	_done = false;
	
	_dir = dir;
	switch(dir)
	{
	case TO_SOCKET:
		// We are opening a local file and SENDING it out the socket
		// to a remote peer. Make sure it exists first
		if (sarfile::fexists(localfname))
		{
			// It exists so Open up the local file for reading
			_local = new sarfile::fileio(localfname, sarfile::FILE_READ);
			if (_local->ok() && _local->isfile())
			{
				// Add it to the current list of open files for select() to poll
				string	locinfo = _local->print();
				string	peerinfo = _peer->print();
				scr.msg("Requesting transfer of %s to %s",
					locinfo.c_str(),
					peerinfo.c_str());
				break;
			}
			scr.error("Cannot Initiate Transfer. Local %s is not a file",
				localfname.c_str());
			delete _local;
		}
		else
		{
			scr.error("Cannot Initiate Transfer. Unable to open local file %s",
				localfname.c_str());
		}
		goto badtran;
	case FROM_SOCKET:
		// We are creating a local file and RECEIVING it in the socket
		// from a remote peer
		// Create the local file for writing to
		if (sarfile::fexists(localfname))
		{
			scr.error("Local file %s already exists cannot create",
				localfname.c_str());
			_errcode = F_ERRCODE_INUSE;
			goto badtran;
		}
		_local = new sarfile::fileio(localfname, sarfile::FILE_WRITE);
		if (_local->ok() && _local->isfile())
		{
			string	locinfo = _local->print();
			string	peerinfo = _peer->print();
			scr.msg("Requesting transfer to %s from %s",
				locinfo.c_str(),
				peerinfo.c_str());
			break;
		}
		scr.error("Cannot Initiate Transfer. Unable to create local file %s",
			localfname.c_str());
		delete _local;
		_errcode = F_ERRCODE_INUSE;
		goto badtran;
	}
	// We are a good transfer as we can open/create local file
	scr.debug(3, "tran::tran(): Adding Local File to sarfiles");
	sarfiles.add(_local);
	_session = req->session();
	_rxstatus = false;
	_txstatus = false;
	_metadatarecvd = F_METADATARECVD_NO;
	// As the last thing set we are ready to transfer data
	_ready = true;
	return;

// We can;t create/open the local file so we are a bad transfer
// send a STATUS back with the error
badtran:
	scr.debug(3, "tran::tran(): Cannot add  Local File to sarfiles");
	_local = nullptr;
	_session = req->session();
	_rxstatus = false;
	_txstatus = true;
	_metadatarecvd = F_METADATARECVD_NO;
	_ready = false;
	_done = true;
	return;
}

// Copy constructor
tran::tran(const tran& t)
{
	_ready = t._ready;
	_requestor = t._requestor;
	_rxstatus = t._rxstatus;
	_txstatus = t._txstatus;
	_metadatarecvd = t._metadatarecvd;
	_dir = t._dir;	
	_transfertimer = t._transfertimer;
	_requesttimer = t._requesttimer;
	_statustimer = t._statustimer;
        _session = t._session;
	_curprogress = t._curprogress;
	_inresponseto = t._inresponseto;
	_offset = t._offset;

	_timetype = t._timetype;
	_timestamp = t._timestamp;
	_lastrxtstamp = t._lastrxtstamp;
	_holes = t._holes;
	_completed = t._completed;
	// These are pointers for _peer, _local
	// Make sure that is so by having the operator= behave appropriately for them
	_peer = t._peer;
	_local = t._local;

	_diskfree = t._diskfree;
	// THESE ARE ALL OF THE FLAGS APPLICABLE TO A TRANSACTION	
	_version = t._version;
	_descriptor = t._descriptor;
	_requesttype = t._requesttype;
	_transfer = t._transfer;
	_reqtstamp = t._reqtstamp;
	_progress = t._progress;
	_txwilling = t._txwilling;
	_rxwilling = t._rxwilling;
	_stream = t._stream;
	_udptype = t._udptype;
	_metadata_udptype = t._metadata_udptype;
	_metadatarecvd = t._metadatarecvd;
	_fileordir = t._fileordir;
	_reqholes = t._reqholes;
	_allholes = t._allholes;
	_reqstatus = t._reqstatus;
	_eod = t._eod;
	_freespace = t._freespace;
	_freespaced = t._freespaced;
	_csumtype = t._csumtype;
	_csumlen = t._csumlen;
	_errcode = t._errcode;
	_done = t._done;
}

tran&
tran::operator =(const tran& t)
{
	_ready = t._ready;
	_requestor = t._requestor;
	_rxstatus = t._rxstatus;
	_txstatus = t._txstatus;
	_metadatarecvd = t._metadatarecvd;
	_dir = t._dir;	
	_transfertimer = t._transfertimer;
	_requesttimer = t._requesttimer;
	_statustimer = t._statustimer;
        _session = t._session;
	_curprogress = t._curprogress;
	_inresponseto = t._inresponseto;
	_offset = t._offset;

	_timetype = t._timetype;
	_timestamp = t._timestamp;
	_lastrxtstamp = t._lastrxtstamp;
	_holes = t._holes;
	_completed = t._completed;
	// These are pointers for _peer, _local
	// Make sure that is so by having the operator= behave appropriately for them
	_peer = t._peer;
	_local = t._local;

	_diskfree = t._diskfree;
	// THESE ARE ALL OF THE FLAGS APPLICABLE TO A TRANSACTION	
	_version = t._version;
	_descriptor = t._descriptor;
	_requesttype = t._requesttype;
	_transfer = t._transfer;
	_reqtstamp = t._reqtstamp;
	_progress = t._progress;
	_txwilling = t._txwilling;
	_rxwilling = t._rxwilling;
	_stream = t._stream;
	_udptype = t._udptype;
	_metadata_udptype = t._metadata_udptype;
	_metadatarecvd = t._metadatarecvd;
	_fileordir = t._fileordir;
	_reqholes = t._reqholes;
	_allholes = t._allholes;
	_reqstatus = t._reqstatus;
	_eod = t._eod;
	_freespace = t._freespace;
	_freespaced = t._freespaced;
	_csumtype = t._csumtype;
	_csumlen = t._csumlen;
	_errcode = t._errcode;
	_done = t._done;
	return(*this);
}

// Clean up the transfer
void
tran::zap()
{
	_ready = false;
	// Close down the local file and remove it from sarfiles list
	if (_local && _local->fd() > 2)
	{
		delete _local;
	}
	// Remove the remaining holes 
	_holes.clear();
	_completed.clear();
}

string
tran::print()
{
	char	tmp[2048];
	string s;
	string from;
	string to;

	if (this->req() == INBOUND)
		s = "Inbound ";
	else // OUTBOUND
		s = "Outbound ";
	if (this->ready())
		s += "started ";
	else
		s += "not started ";
	if (this->dir() == FROM_SOCKET)
	{
		from = _peer->print();
		to = _local->print();
	}
	else // TO_SOCKET
	{
		to = _peer->print();
		from = _local->print();
	}
	sprintf(tmp, "session %" PRIu32 " from %s to %s\n",
		_session, from.c_str(), to.c_str());
	s += tmp;
	return s;
}

bool
tran::sendstatus()
{
	saratoga::status *s;

	scr.debug(2, "Assembling STATUS for transfer");
	// Se current timestamp if we have enabled it in command line
	// the timestamp type we are using.
	if (c_timestamp.flag() == F_TIMESTAMP_YES)
	{
		timestamp	ts(c_timestamp.ttype());

		s = new status(this->descriptor(),
			this->metadatarecvd(),
			this->allholes(),
			this->reqholes(),
			this->errcode(),
			this->session(),
			ts,
			this->curprogress(),
			this->inresponseto(),
			this->holelist());
	}
	else
	{
		s = new status(this->descriptor(),
			this->metadatarecvd(),
			this->allholes(),
			this->reqholes(),
			this->errcode(),
			this->session(),
			this->curprogress(),
			this->inresponseto(),
			this->holelist());
	}
	if (s->badframe() || (s->tx(this->peer()) != (ssize_t) s->paylen()))
	{
		scr.error("tran::sendstatus(): Bad STATUS frame",
			s->paylen());
		delete s;
		return(false);
	}
	// scr.msgout("tran::sendstatus(): %s", s->print().c_str());
	delete s;
	_txstatus = true;
	_statustimer.reset(); // We have sent it so reset the timer
	return(true);
}

bool
tran::sendmetadata()
{
	saratoga::metadata *m;

	scr.debug(2, "Assembling METADATA for transfer of %s",
		this->localfname().c_str());
		m = new metadata(this->descriptor(),
			this->transfer(),
			this->progress(),
			this->session(),
			this->local());
	scr.debug(7, "Assembled METADATA is %s", m->print().c_str());
	if (m->badframe() || (m->tx(this->peer()) != (ssize_t) m->paylen()))
	{
		scr.error("tran::sendmetadata(): Bad METADATA frame");
		delete m;
		return(false);
	}
	scr.msgout("tran::sendmetadata(): METADATA Sent");
	delete m;
	return true;
}

// We have a buffer, convert it into data frames(s) and send
// Send multiple frames if the len > data::maxframesize
void
tran::senddata(std::list<saratoga::buffer> *bufs)
{
	size_t framesize = data::maxframesize;
bufloop:
	while (! bufs->empty())
	{
		saratoga::buffer *b = &(bufs->front());
		ssize_t remainder = b->len();
		offset_t	offset = b->offset();
		size_t framecount = b->len() / framesize;
		char *buf = b->buf();
		while (framecount)
		{
			saratoga::data	*d = new data(this->descriptor(),
				this->transfer(),
				this->reqstatus(),
				this->eod(),
				this->reqtstamp(),
				this->session(),
				offset,
				buf,
				framesize);
			if (d->badframe() || d->tx(this->peer()) != (ssize_t) d->paylen())
			{
				scr.error("tran::senddata(): Bad DATA frame");
				delete d;
				bufs->pop_front();
				goto bufloop;
			}
			else
				scr.msgout("Sent DATA Frame: Length=%d Offset=%" PRIu64 " Frame# %d",
					framesize, offset, framecount);
			scr.debug(7, "tran::senddata(): Full Frame %s", 
				d->print().c_str());
			delete d;
			framecount--;
			buf += data::maxframesize;
			remainder -= data::maxframesize;
			// Increment the file offset
			offset += framesize;
		}
		if (remainder)
		{
			saratoga::data	*d = new data(this->descriptor(),
				this->transfer(),
				this->reqstatus(),
				this->eod(),
				this->reqtstamp(),
				this->session(),
				offset,
				buf,
				remainder);
			if (d->badframe() || d->tx(this->peer()) != (ssize_t) d->paylen())
			{
				scr.error("tran::senddata(): Bad DATA frame");
				delete d;
				bufs->pop_front();
				goto bufloop;
			}
			else
				scr.msgout("Sent Remaining DATA Frame: Length=%d Offset=%" PRIu64 "",
					remainder, offset);
			scr.debug(7, "tran::senddata(): Remainder Frame %s", 
				d->print().c_str());
			scr.debug(7, "Sleeping for 10 Seconds");
			delete d;
			offset += remainder;
		}
		_offset = offset;
		bufs->pop_front();
	}
}

// We have received a request. Add the transfer to the list
// return pointer to it or NULL if can't create the transfer
saratoga::tran *
transfers::add(saratoga::requestor inorout, direction dir, saratoga::request *req, sarnet::udp *sock, string localfname)
{
//	sysinfo		df("/dev/sda1");
//	_diskfree = df.diskfree();

	string socstr = sock->print();
	saratoga::tran *t = new saratoga::tran(inorout, dir, req, sock, localfname);
	if (t->ready())
	{
		if (dir == FROM_SOCKET)
			scr.debug(3, "transfers::add(): INBOUND %" PRIu32 " Added transfer from %s to %s",
				(uint32_t) req->session(), 
				socstr.c_str(), 
				localfname.c_str());
		else // TO_SOCKET
			scr.debug(3, "transfers::add(): OUTBOUND %" PRIu32 " Added transfer from %s to %s", 
				(uint32_t) req->session(), localfname.c_str(), socstr.c_str());
		_transfers.push_back(*t);
		scr.debug(2, "%s", sartransfers.print().c_str());
		return(sartransfers.match(req->session(), sock, dir));
	}
	if (dir == FROM_SOCKET)
	{
		scr.error("INBOUND %" PRIu32 " Unable to add transfer from %s to %s",
			(uint32_t) req->session(), 
			socstr.c_str(), 
			localfname.c_str());
		// Send the STATUS error code back to the other end
		// Most likely we can't vreate the local file
		t->sendstatus();
	}	
	else // TO_SOCKET
		scr.error("OUTBOUND %" PRIu32 " Unable to add transfer from %s to %s", 
			(uint32_t) req->session(), 
			localfname.c_str(), 
			socstr.c_str());
	delete t;
	scr.error("transfers.add(): Transfer not ready Deleteted tran");
	return(nullptr);
}

// Return pointer to the transfer which has a match for  the sesssion and the peer ip 
saratoga::tran *
transfers::match(session_t sess, sarnet::udp *addr, direction dir)
{
	string dirprint;

	(dir == TO_SOCKET) ? dirprint = "TO SOCKET" : dirprint = "FROM SOCKET";

	for (std::list<saratoga::tran>::iterator t = sartransfers.begin(); t != sartransfers.end(); t++)
	{
		sarnet::udp 	*peer = t->peer();
		sarnet::ip	paip(peer->straddr());
		sarnet::ip	adip(addr->straddr());

		if ((t->session() == sess) && (paip == adip))
		{
			scr.debug(7, "transfers::match(): Found Transfer %s Session %" PRIu32 "  %s",
				dirprint.c_str(), (uint32_t) sess, addr->straddr().c_str());
			return(&(*t));
		}
	}
	scr.error("transfers::match(): NO MATCH FOUND for Transfer %s Session %" PRIu32 "  %s",
		dirprint.c_str(), (uint32_t) sess, addr->straddr().c_str());
	return(nullptr);
}

// Return pointer to transfer which has a matching local file
saratoga::tran *
transfers::match(sarfile::fileio *localfile)
{
	for (std::list<saratoga::tran>::iterator t = sartransfers.begin(); t != sartransfers.end(); t++)
		if (t->local()->fd() == localfile->fd())
			return(&(*t));
	return(nullptr);
}

// Handle received REQUEST frames
saratoga::tran *
transfers::rxrequest(saratoga::request *req, sarnet::udp *sock)
{
	saratoga::tran *t;
	string sockinfo = sock->print();

	scr.debug(5, "transfers::rxrequest(): RX REQUEST from %s", sockinfo.c_str());
	// Find if the transfer is already there
	if ((t = this->match(req->session(), sock, FROM_SOCKET)) != nullptr)
	{
		scr.error("Transfer session %" PRIu32 " for %s already exists, ignoring this",
			(uint32_t) req->session(),
			sockinfo.c_str());
		// But lets send a STATUS back in any case
		t->sendstatus();
		return(t);
	}

	// OK create the new transfer
	string localfname = c_home.dir() + "/" + req->fname();
	if ((t = sartransfers.add(INBOUND, FROM_SOCKET, req, sock, localfname)) == nullptr)
	{
		// Apply all of the appropriate flags to the transfer class
		scr.debug(3, "transfers::rxrequest(): Cannot add transfer session %" PRIu32 " for %s",
			(uint32_t) req->session(),
			sockinfo.c_str());
		return(nullptr);
	}
	// Send back the STATUS to say we have initiiated the transfer OK
	t->sendstatus();
	scr.debug(7, req->print());
	// We have received a frame (good or bad) for this transfer reset the timer
	return(t);
}

// These are what we check & apply to a transfer when recieve a METADATA
/* 
 sarflags:
	enum f_descriptor
	enum f_transfer
	enum f_progress
	enum f_udptype
	enum f_csumlen
	enum f_csumtype
 values:
	session_t		session 
	checksums::checksum	*csum;
	sardir::dirent		*_dirent;
 */
void
tran::applymetadata(saratoga::metadata *met)
{

	if (met->descriptor() != this->descriptor())
	{
		scr.error("applymetadata:: Transfer Descriptor mismatch");
		_errcode = F_ERRCODE_BADDESC;
		return;
	}
	if (met->transfer() != this->transfer())
	{
		scr.error("applymetadata:: Transfer Type mismatch");
		_errcode = F_ERRCODE_BADFLAG;
		return;
	}
	_progress = met->progress();
	if (met->progress() == F_PROGRESS_TERMINATED)
	{
		scr.error("applymetadata:: Transfer Shutdown");
		_errcode = F_ERRCODE_SHUTDOWN;
		return;
	}
	_metadata_udptype = met->metadata_udptype();
	if (met->metadata_udptype() == F_UDPLITE)
	{
		scr.error("applymetadata:: UDPLite not supported");
		// We only do UDP not UDP Lite at the moment
		_metadata_udptype = F_UDPONLY;
		_errcode = F_ERRCODE_BADFLAG;
		return;
	}
	_csumtype = met->csumtype();
	_csumlen = met->csumlen();
	// Check the session number
	if (met->session() != this->session())
	{
		scr.error("applymetadata: Session Number mismatch %" PRIu32 " != %" PRIu32 "",
			met->session(),
			this->session());
		_errcode = F_ERRCODE_NOID;
		return;
	}
	// Set the local files metadata equivalent to the received METADATA
	_local->setdir(met->dir());
	_errcode = F_ERRCODE_SUCCESS;
	_metadatarecvd = F_METADATARECVD_YES; // We have received a valid METADATA
	return;
}

// Handle received METADATA frames
saratoga::tran *
transfers::rxmetadata(saratoga::metadata *met, sarnet::udp *sock)
{
	saratoga::tran *t;
	string sockinfo = sock->print();

	scr.debug(5, "transfers::rxmetadata(): RX METADATA from %s", sockinfo.c_str());
	// Find what transfer this metadata is applicable to
	if ((t = this->match(met->session(), sock, FROM_SOCKET)) == nullptr)
	{
		scr.error("Transfer session %" PRIu32 " for %s does not exist, discarding frame",
			(uint32_t) met->session(),
			sockinfo.c_str());
		return(nullptr);
	}
	// Apply all of the appropriate flags to the transfer class
	scr.debug(7, "transfers::rxmetadata(): Found transfer session %" PRIu32 " for %s",
		(uint32_t) met->session(),
		sockinfo.c_str());
	scr.debug(7, met->print());
	t->applymetadata(met);
	return(t);
}

// These are the things we check & apply to a transfer when recieve a DATA
/* 
 sarflags:
	enum f_descriptor
	enum f_transfer
	enum f_reqstatus
	enum f_reqtstamp
	enum f_eod
 values:
	session_t	session
	offset_t	offset
	size_t		paylen
	char		*payload
 */
void
tran::applydata(saratoga::data *dat)
{

	if (dat->descriptor() != this->descriptor())
	{
		scr.error("applydata:: Transfer Descriptor mismatch");
		_errcode = F_ERRCODE_BADDESC;
		return;
	}
	if (dat->transfer() != this->transfer())
	{
		scr.error("applydata:: Transfer Type mismatch");
		_errcode = F_ERRCODE_BADFLAG;
		return;
	}
	if (dat->reqtstamp() != this->reqtstamp())
	{
		scr.error("applydata:: Timestamps not enabled mismatch");
		_errcode = F_ERRCODE_BADFLAG;
		return;
	}
	_reqstatus = dat->reqstatus();
	_eod = dat->eod();
	// Check the session number
	if (dat->session() != this->session())
	{
		scr.error("applydata: Session Number mismatch %" PRIu32 " != %" PRIu32 "",
			dat->session(),
			_session);
		_errcode = F_ERRCODE_NOID;
		return;
	}

	// Seek to the local file offset position to write to
//	scr.debug(2, "applydata:: Will Seek to %" PRIu64 " and write %" PRIu64 " bytes to %s",
//		dat->offset(),
//		dat->dbuflen(),
//		_local->fname().c_str());
	_local->fwrite(dat->dbuf(), dat->dbuflen(), dat->offset());
	hole databuf(dat->offset(), dat->dbuflen());
	// Remove the hole if it is within our current list of holes
	_holes -= databuf;
	// Add the buffer to our list of completed holes
	_completed += databuf;

	std::list<hole>::iterator firstcompleted = _completed.first();
	if ( _holes.count() == 0)
	{
		// We have no holes, have received our METADATA 
		// and the remaining completed is the size of our file
		// We are done
		if ( _completed.count() == 1 && 
			this->metadatarecvd() == F_METADATARECVD_YES &&
			firstcompleted->starts() == 0 && 
			firstcompleted->ends() == _local->filesize())
		{
			scr.msg("Successfully completed transfer of session %" PRIu32 "", 
				this->session());
			_offset = _local->filesize();
			_curprogress = _local->filesize();
			_done = true;
			_errcode = F_ERRCODE_SUCCESS;
			return;
		}
		// Our offset is at the end of our first completed buffer
		_curprogress = firstcompleted->ends();
	}
	else // We have multiple holes (holes.count() > 0)
	{
		std::list<hole>::iterator firsthole = _holes.first();
		(firsthole->starts() == 0) ?  _curprogress = 0 : _curprogress = firstcompleted->ends();
	}
	
	// Add a hole from the current offset to the beginning of the next completed - 1
	// We dont add a hole to the end
	for (std::list<hole>::iterator i = _completed.first(); i != _completed.last(); i++)
	{
		offset_t startofhole = dat->offset() + dat->dbuflen();
		offset_t endofhole = 0;
		if (i->starts() > startofhole)
		{
			endofhole = i->starts() - 1;
			hole newhole(startofhole, endofhole);
			_holes += newhole;
		}
	}
	_errcode = F_ERRCODE_SUCCESS;
	return;
}

// Handle received DATA frames
saratoga::tran *
transfers::rxdata(saratoga::data *dat, sarnet::udp *sock)
{
	saratoga::tran *t;
	string sockinfo = sock->print();

	scr.debug(5, "transfers::rxdata(): RX DATA from %s", sockinfo.c_str());
	// Find what transfer this data is applicable to
	if ((t = this->match(dat->session(), sock, FROM_SOCKET)) == nullptr)
	{
		scr.error("Transfer session %" PRIu32 " for %s does not exist",
			(uint32_t) dat->session(),
			sockinfo.c_str());
		return(nullptr);
	}
	char* payload = dat->payload();
	size_t paylen = dat->paylen();
	// Apply all of the appropriate flags to the transfer class
	scr.debug(9, "transfers::rxdata(): Found transfer session %" PRIu32 " for %s",
		(uint32_t) dat->session(),
		sockinfo.c_str());
	scr.debug(7, dat->print());
	t->applydata(dat);
	return(t);
}

// These are what we check & apply to a transfer when recieve a STATUS
/* 
 sarflags:
	enum f_descriptor
	enum f_reqtstamp
	enum f_metadatarecvd
	enum f_allholes
	enum f_reqholes
	enum f_errcode
 values:
	bool		rxstatus - true
	session_t	session - Done
	timestamp	timestamp - Done
	offset_t	curprogress - Done
	offset_t	inresponseto - Done
	holes		*sholes - Done
 */
// Apply the STATUS to the transfer
void
tran::applystatus(saratoga::status *sta)
{
	// If we have received an error code then return it jump back and rx it	
	_errcode = sta->errcode();
	if (_errcode.get() != F_ERRCODE_SUCCESS)
	{
		scr.error("rxstatus: Status Error code received");
		_rxstatus = false;
		_ready = false;
		return;
	}

	// Descriptor mismatch
	if (sta->descriptor() != this->descriptor())
	{
		scr.error("transfers::rxstatus(): Transfer Descriptor mismatch");
		_rxstatus = false;
		_ready = false;
		_errcode = F_ERRCODE_BADDESC;
		return;
	}
	// Session # mismatch
	if (sta->session() != this->session())
	{
		scr.error("transfers::rxstatus(): Session Number mismatch %" PRIu32 " != %" PRIu32 "",
			sta->session(),
			this->session());
		_rxstatus = false;
		_ready = false;
		_errcode = F_ERRCODE_NOID;
		return;
	}

	// If the remote end has not received a METADATA and
	// we are the end sending the local file then send the METADATA
	// for the local file
	_metadatarecvd = sta->metadatarecvd();
	if ((_metadatarecvd.get() == F_METADATARECVD_NO) &&
		(_local->rorw() == sarfile::FILE_READ))
	{
		scr.debug(5, "transfers::rxstatus(): Send a METADATA");
		this->sendmetadata();
	}
		
	_allholes = sta->allholes();
	_reqholes = sta->reqholes();
	_curprogress = sta->progress();
	_inresponseto = sta->inresponseto();

	// Copy the timestmap if we have one
	if (sta->reqtstamp() == F_TIMESTAMP_YES)
	{
		scr.debug(2, "removing lastrxtstamp %s", 
			_lastrxtstamp.asctime().c_str());
		_lastrxtstamp = sta->tstamp();
	}
	// Add the holes from this status into the transfer holes
	if (sta->holesptr() != nullptr)
		_holes += *(sta->holesptr());
	// All is good there are no errors here
	_rxstatus = true; // We have received a valid status frame
	_ready = true; // We are a good status so ready to receive data
	_statustimer.reset(); // We have one so reset the status timer
	_errcode = F_ERRCODE_SUCCESS;
}

// Handle received STATUS frames and update the transfer variables
// Return back poiner to tran or nullptr if can't find one
saratoga::tran *
transfers::rxstatus(saratoga::status *sta, sarnet::udp *sock)
{
	saratoga::tran *t;
	string sockinfo = sock->print();

	scr.debug(5, "transfers::rxstatus(): RX STATUS from %s", sockinfo.c_str());
	// Find what transfer this status is applicable to
	if ((t = this->match(sta->session(), sock, FROM_SOCKET)) == nullptr)
	{
		scr.error("Transfer session %" PRIu32 " for %s does not exist",
			(uint32_t) sta->session(),
			sockinfo.c_str());
		return(nullptr);
	}
	// Apply all of the appropriate flags to the transfer class
	scr.debug(3, "transfers::rxstatus(): Found transfer session %" PRIu32 " for %s",
		(uint32_t) sta->session(),
		sockinfo.c_str());
	// scr.debug(6, sta->print());
	t->applystatus(sta);
	return t;
}

string
transfers::print()
{
	string s = "";

	if (_transfers.empty())
		return("No curent transfers in progress");
	for (std::list<tran>::iterator i = _transfers.begin(); i != _transfers.end(); i++)
		s += i->print();
	return(s);
}

}; // namespace saratoga

