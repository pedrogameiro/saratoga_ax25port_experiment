/*
 
 Copyright (c) 2012, Charles Smith
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

#include "saratoga.h"
#include "sarflags.h"
#include "timestamp.h"
#include "frame.h"
#include "checksum.h"
#include "metadata.h"
#include "globals.h"

using namespace std;

namespace saratoga
{

metadata::metadata( const enum f_descriptor des,
	const enum f_transfer tfr,
	const enum f_progress prog,
	const session_t	session,
	sarfile::fileio	*local)
{
	uint32_t	tmp_32;

	_badframe = false;
	_payload = nullptr;

	Fversion	version = F_VERSION_1;
	Fframetype	frametype = F_FRAMETYPE_METADATA;
	Fdescriptor	descriptor = des;
	Ftransfer	transfer = tfr; // What are we transferring
	Fprogress	progress = prog; // Are we in progress or terminating
	Fmetadata_udptype	metadata_udptype = F_UDPONLY; // We only do UDP
	Fcsumtype	csumtype = local->csum().csumtype();
	Fcsumlen	csumlen = local->csum().csumlen();
	
	_flags += version;
	_flags += frametype;
	_flags += descriptor;
	_flags += transfer;
	_flags += progress;
	_flags += metadata_udptype;
	_flags += csumtype;
	_flags += csumlen;

	_session = session;

	// Create the directory entry
	_dir = local->dir();
	if (_dir.baddir())
	{
		scr.error("metadata::metadata(): Bad diretory entry, not created");
		_badframe = true;
		return;
	}
	// Well there is no checksum but this keeps printing happy
	// and is consistent with the checksum superclass
	_csum = local->csum();
	// Work out the payload size
	_paylen = sizeof(flag_t) + sizeof(session_t);
	_paylen += _dir.paylen();
	// This of course should be 0 for no checksum
	_paylen += ((size_t) _csum.size() * 4); //# 32 bit values * 4

	// Now put the metadata into it ready for transmission
	_payload = new char[_paylen];
	char *payload = _payload;

	// Flags
	tmp_32 = htonl(_flags.get());
	memcpy(payload, &tmp_32,sizeof(flag_t));
	payload += sizeof(flag_t);

	// Session
	tmp_32 = htonl(_session);
	memcpy(payload, &tmp_32, sizeof(session_t));
	payload += sizeof(session_t);

	// Checksum
	if (_csum.csumtype() != checksums::CSUM_NONE)
	{
		for (size_t i = 0; i < _csum.size(); i++)
		{
			tmp_32 = (uint32_t) htonl(_csum.value(i));
			memcpy(payload, &tmp_32, sizeof(uint32_t));
			payload += sizeof(uint32_t);
		}
	}
	
	// Directory Entry
	memcpy(payload, _dir.payload(), _dir.paylen());
	scr.debug(7, "METADATA FRAME IS %s", this->print().c_str());
}

/*
 * Given a buffer and length, assemble the metadata
 * upon receipt of a METADATA
 */
metadata::metadata(char *payload,
	const size_t pl)
{
	size_t	paylen = pl;

	_badframe = false;
	if (payload == nullptr || paylen == 0)
	{
		scr.error("metadata(frame): Is NULL\n");
		_badframe = true;
		return;
	}
	// Assemble the frame info
	_paylen = paylen;
	_payload = new char[paylen];
	memcpy(_payload, payload, _paylen);

	// Grab the flags
	_flags = (flag_t) ntohl(*(uint32_t *) payload);
	payload += sizeof(flag_t);
	paylen -= sizeof(flag_t);

	Fversion	version = _flags.get();
	Fframetype	frametype = _flags.get();

	if (version.get() != F_VERSION_1)
	{
		scr.error("Metadata: Bad Saratoga Version");
		_badframe = true;
		return;
	}
	if (frametype.get() != F_FRAMETYPE_METADATA)
	{
		scr.error("Metadata: Not a METADATA frame");
		_badframe = true;
		return;
	}
	
	Fdescriptor	descriptor = _flags.get();
	Ftransfer	transfer = _flags.get();
	Fprogress	progress = _flags.get();
	Fcsumlen	csumlen = _flags.get();
	Fcsumtype	csumtype = _flags.get();

	// Session
	_session = (session_t) ntohl(*(uint32_t *) payload);
	payload += sizeof(session_t);
	paylen -= sizeof(session_t);

	// Optional checksum
	switch(csumtype.get())
	{
	case F_CSUM_NONE:
		_csum = checksums::checksum();
		break;
	case F_CSUM_CRC32:
		uint32_t cr;
		cr = ntohl(*(uint32_t *) payload);
		_csum = checksums::checksum(checksums::CSUM_CRC32, &cr);
		payload += sizeof(uint32_t);
		paylen -= sizeof(uint32_t);
		break;
	case F_CSUM_MD5:
		uint32_t md[4];
		for (int i = 0; i < 4; i++)
		{
			md[i] = ntohl(*(uint32_t *) payload);
			payload += sizeof(uint32_t);
			paylen -= sizeof(uint32_t);
		}
		_csum = checksums::checksum(checksums::CSUM_MD5, md);
		break;
	case F_CSUM_SHA1:
		uint32_t sh[5];
		for (int i = 0; i < 5; i++)
		{
			sh[i] = ntohl(*(uint32_t *) payload);
			payload += sizeof(uint32_t);
			paylen -= sizeof(uint32_t);
		}
		_csum = checksums::checksum(checksums::CSUM_SHA1, sh);
		break;
	default:
		scr.error("Metadata: Invalid Checksum Type Using none");
		_csum = checksums::checksum();
	}

	// Directory Entry
	_dir = sardir::dirent(payload, paylen);
}

enum f_descriptor
metadata::descriptor(enum f_descriptor d)
{
	Fdescriptor	descriptor = d;
	_flags += descriptor;
	return descriptor.get();
}

enum f_transfer
metadata::transfer(enum f_transfer t)
{
	Ftransfer	transfer = t;
	_flags += transfer;
	return transfer.get();
}

enum f_progress
metadata::progress(enum f_progress p)
{
	Fprogress	progress = p;
	_flags += progress;
	return progress.get();
}

enum f_udptype
metadata::metadata_udptype(enum f_udptype u)
{
	Fmetadata_udptype	metadata_udptype = u;
	_flags += metadata_udptype;
	return metadata_udptype.get();
}

enum f_csumtype
metadata::csumtype(enum f_csumtype c)
{
	Fcsumtype	csumtype = c;
	_flags += csumtype;
	return csumtype.get();
}

enum f_csumlen
metadata::csumlen(enum f_csumlen l)
{
	Fcsumlen	csumlen = l;
	_flags += csumlen;
	return csumlen.get();
}

// Get various flags applicable to metadata frames
enum f_descriptor
metadata::descriptor() {
	Fdescriptor d = _flags.get();
	return d.get();
}

enum f_transfer
metadata::transfer() {
	Ftransfer t = _flags.get();
	return t.get();
}

enum f_progress
metadata::progress() {
	Fprogress p = _flags.get();
	return p.get();
}

enum f_udptype
metadata::metadata_udptype() {
	Fmetadata_udptype u = _flags.get();
	return u.get();
}

enum f_csumtype
metadata::csumtype() {
	Fcsumtype c = _flags.get();
	return c.get();
}

enum f_csumlen
metadata::csumlen() {
	Fcsumlen l = _flags.get();
	return l.get();
}


/*
 * Print out the metadata
 *	flags
 * 	session
 * 	checksum (if required)
 * 	directory entry
 */
string
metadata::print()
{
	char	tmp[128];
	string	s;

	if (_badframe)
	{
		s = "metadata::print(): Bad METADATA frame";
		scr.error(s);
		return s;
	}
	 // These are the valid flags types applicable to a metadata
	Fversion	version = _flags.get();
	Fframetype	frametype = _flags.get();
	Fdescriptor	descriptor = _flags.get();
	Ftransfer	transfer = _flags.get();
	Fprogress	progress = _flags.get();
	Fcsumlen	csumlen = _flags.get();
	Fcsumtype	csumtype = _flags.get();
	Freqtstamp	reqtstamp = _flags.get();
	Freqstatus	reqstatus = _flags.get();

	s = frametype.print();
	s += printflags("METADATA FLAGS", this->flags());
	s += "    ";
	s += version.print();
	s += "\n    ";
	s += descriptor.print();
	s += "\n    ";
	s += transfer.print();
	s += "\n    ";
	s += reqtstamp.print();
	s += "\n    ";
	s += reqstatus.print();
	s += "\n    ";
	sprintf(tmp, 
		"Session: %" PRIu32 "", (uint32_t) _session);
	s += tmp;
	s += "\n    ";
	s += _csum.print();
	s += "\n    ";
	s += _dir.print();
	return(s);
}

}; // Namespace saratoga

