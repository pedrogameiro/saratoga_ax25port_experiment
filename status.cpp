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

#include "screen.h"
#include "saratoga.h"
#include "sarflags.h"
#include "timestamp.h"
#include "frame.h"
#include "holes.h"
#include "status.h"
#include "globals.h"

using namespace std;

namespace saratoga
{

// Create a status frame from scratch
// with no timestamp
status::status( const enum f_descriptor des,
	const enum f_metadatarecvd md,
	const enum f_allholes ah,
	const enum f_reqholes	ho,
	const enum f_errcode	st,	
	const session_t	session,
	const offset_t	progress,
	const offset_t	inresponseto,
	saratoga::holes	*h)
{
	uint16_t	tmp_16;
	uint32_t	tmp_32;
	uint64_t	tmp_64;
#ifdef UINT128_T
	uint128_t	tmp_128;
#endif

	_badframe = false;

	int	holecount = 0;
	if (h != nullptr)
		holecount = h->count();

	Fversion	version = F_VERSION_1;
	Fframetype	frametype = F_FRAMETYPE_STATUS;
	Fdescriptor	descriptor = des;
	Freqtstamp	reqtstamp = F_TIMESTAMP_NO;
	Fmetadatarecvd	metadatarecvd = md;
	Fallholes	allholes = ah;
	Freqholes	freqholes = ho;
	Ferrcode	errcode = st;
	
	// Work out how big our frame has to be and allocate it
	size_t fsize = sizeof(flag_t) + sizeof(session_t);
	fsize += descriptor.length(); // Offset
	
	fsize += (descriptor.length() * 2); // progress, inresponseto
	fsize += (descriptor.length() * 2 * holecount); // and the holes if any

	_payload = new char[fsize];
	_paylen = fsize;
	char	*sbufp = _payload;

	// Set and copy flags
	_flags += version;
	_flags += frametype;
	_flags += descriptor;
	_flags += reqtstamp;
	_flags += metadatarecvd;
	_flags += allholes;
	_flags += errcode;

	tmp_32 = htonl(_flags.get());
	memcpy(sbufp, &tmp_32, sizeof(flag_t));
	sbufp += sizeof(flag_t);

	// Set and copy session
	_session = session;
	tmp_32 = htonl(session);
	memcpy(sbufp, &tmp_32, sizeof(session_t));
	sbufp += sizeof(session_t);

	// We dont use it but make sure its set to current Zulu
	_timestamp = timestamp(); 

	_progress = progress;
	_inresponseto = inresponseto;
	switch(descriptor.get())
	{
	case F_DESCRIPTOR_16:
		tmp_16 = htons((uint16_t) _progress);
		memcpy(sbufp, &tmp_16, sizeof(uint16_t));
		sbufp += sizeof(uint16_t);
		tmp_16 = htons((uint16_t) _inresponseto);
		memcpy(sbufp, &tmp_16, sizeof(uint16_t));
		sbufp += sizeof(uint16_t);
		break;
	case F_DESCRIPTOR_32:
		tmp_32 = htonl((uint32_t) _progress);
		memcpy(sbufp, &tmp_32, sizeof(uint32_t));
		sbufp += sizeof(uint32_t);
		tmp_32 = htonl((uint32_t) _inresponseto);
		memcpy(sbufp, &tmp_32, sizeof(uint32_t));
		sbufp += sizeof(uint32_t);
		break;
	case F_DESCRIPTOR_64:
		tmp_64 = htonll(_progress);
		memcpy(sbufp, &tmp_64, sizeof(uint64_t));
		sbufp += sizeof(uint64_t);
		tmp_64 = htonll(_inresponseto);
		memcpy(sbufp, &tmp_64, sizeof(uint64_t));
		sbufp += sizeof(uint64_t);
		break;
	case F_DESCRIPTOR_128:
	#ifdef UINT128_T
		tmp_128 = htonlll(_progress);
		memcpy(sbufp, &tmp_128, sizeof(uint128_t));
		sbufp += sizeof(uint128_t);
		tmp_128 = htonlll(_inresponseto);
		memcpy(sbufp, &tmp_128, sizeof(uint128_t));
		sbufp += sizeof(uint128_t);
		break;
	#else
		_badframe = true;
		scr.error("status(): Descriptor 128 Bit Size Not Supported\n");
		return;
	#endif
	default:
		_badframe = true;
		scr.error("status(): Invalid Descriptor Size");
		return;
	}

	// And the holes if any
	for (std::list<saratoga::hole>::iterator i = h->first(); i != h->last(); i++)
	{
		_holes.add(i->starts(), i->length());
		switch(descriptor.get())
		{
		case F_DESCRIPTOR_16:
			tmp_16 = htons((uint16_t) i->starts());
			memcpy(sbufp, &tmp_16, sizeof(uint16_t));
			sbufp += sizeof(uint16_t);
			tmp_16 = htons((uint16_t) i->ends());
			memcpy(sbufp, &tmp_16, sizeof(uint16_t));
			sbufp += sizeof(uint16_t);
			break;
		case F_DESCRIPTOR_32:
			tmp_32 = htonl((uint32_t) i->starts());
			memcpy(sbufp, &tmp_32, sizeof(uint32_t));
			sbufp += sizeof(uint32_t);
			tmp_32 = htonl((uint32_t) i->ends());
			memcpy(sbufp, &tmp_32, sizeof(uint32_t));
			sbufp += sizeof(uint32_t);
			break;
		case F_DESCRIPTOR_64:
			tmp_64 = htonll(i->starts());
			memcpy(sbufp, &tmp_64, sizeof(uint64_t));
			sbufp += sizeof(uint64_t);
			tmp_64 = htonll(i->ends());
			memcpy(sbufp, &tmp_64, sizeof(uint64_t));
			sbufp += sizeof(uint64_t);
			break;
		case F_DESCRIPTOR_128:
		#ifdef UINT128_T
			tmp_128 = htonlll(i->starts());
			memcpy(sbufp, &tmp_128, sizeof(uint128_t));
			sbufp += sizeof(uint128_t);
			tmp_128 = htonlll(i->ends());
			memcpy(sbufp, &tmp_128, sizeof(uint128_t));
			sbufp += sizeof(uint128_t);
			break;
		#else
			scr.error("status(): Descriptor 128 Bit Size Not Supported\n");
			_badframe = true;
			return;
		#endif
		default:
			scr.error("status(): Invalid Descriptor Size");
			_badframe = true;
			return;
		}
	}
}

// Create a status frame from scratch
// with a timestamp
status::status( const enum f_descriptor des,
	const enum f_metadatarecvd md,
	const enum f_allholes ah,
	const enum f_reqholes	ho,
	const enum f_errcode	st,	
	const session_t	session,
	const timestamp	&tstamp,
	const offset_t	progress,
	const offset_t	inresponseto,
	saratoga::holes	*h)
{
	uint16_t	tmp_16;
	uint32_t	tmp_32;
	uint64_t	tmp_64;
#ifdef UINT128_T
	uint128_t	tmp_128;
#endif

	timestamp ts = tstamp;

	_badframe = false;

	int	holecount = 0;
	if (h != nullptr)
		holecount = h->count();

	Fversion	version = F_VERSION_1;
	Fframetype	frametype = F_FRAMETYPE_STATUS;
	Fdescriptor	descriptor = des;
	Freqtstamp	reqtstamp = F_TIMESTAMP_YES;
	Fmetadatarecvd	metadatarecvd = md;
	Fallholes	allholes = ah;
	Freqholes	freqholes = ho;
	Ferrcode	errcode = st;
	
	// Work out how big our frame has to be and allocate it
	size_t fsize = sizeof(flag_t) + sizeof(session_t);
	fsize += descriptor.length(); // Offset
	fsize += ts.length();
	fsize += (descriptor.length() * 2); // progress, inresponseto
	fsize += (descriptor.length() * 2 * holecount); // and the holes if any

	_payload = new char[fsize];
	_paylen = fsize;
	char	*sbufp = _payload;

	// Set and copy flags
	_flags += version;
	_flags += frametype;
	_flags += descriptor;
	_flags += reqtstamp;
	_flags += metadatarecvd;
	_flags += allholes;
	_flags += errcode;

	tmp_32 = htonl(_flags.get());
	memcpy(sbufp, &tmp_32, sizeof(flag_t));
	sbufp += sizeof(flag_t);

	// Set and copy session
	_session = session;
	tmp_32 = htonl(session);
	memcpy(sbufp, &tmp_32, sizeof(session_t));
	sbufp += sizeof(session_t);

	// Set and copy timestamp
	_timestamp = tstamp;
	memcpy(sbufp, _timestamp.hton(), _timestamp.length());
	sbufp += _timestamp.length();

	_progress = progress;
	_inresponseto = inresponseto;
	switch(descriptor.get())
	{
	case F_DESCRIPTOR_16:
		tmp_16 = htons((uint16_t) _progress);
		memcpy(sbufp, &tmp_16, sizeof(uint16_t));
		sbufp += sizeof(uint16_t);
		tmp_16 = htons((uint16_t) _inresponseto);
		memcpy(sbufp, &tmp_16, sizeof(uint16_t));
		sbufp += sizeof(uint16_t);
		break;
	case F_DESCRIPTOR_32:
		tmp_32 = htonl((uint32_t) _progress);
		memcpy(sbufp, &tmp_32, sizeof(uint32_t));
		sbufp += sizeof(uint32_t);
		tmp_32 = htonl((uint32_t) _inresponseto);
		memcpy(sbufp, &tmp_32, sizeof(uint32_t));
		sbufp += sizeof(uint32_t);
		break;
	case F_DESCRIPTOR_64:
		tmp_64 = htonll(_progress);
		memcpy(sbufp, &tmp_64, sizeof(uint64_t));
		sbufp += sizeof(uint64_t);
		tmp_64 = htonll(_inresponseto);
		memcpy(sbufp, &tmp_64, sizeof(uint64_t));
		sbufp += sizeof(uint64_t);
		break;
	case F_DESCRIPTOR_128:
	#ifdef UINT128_T
		tmp_128 = htonlll(_progress);
		memcpy(sbufp, &tmp_128, sizeof(uint128_t));
		sbufp += sizeof(uint128_t);
		tmp_128 = htonlll(_inresponseto);
		memcpy(sbufp, &tmp_128, sizeof(uint128_t));
		sbufp += sizeof(uint128_t);
		break;
	#else
		_badframe = true;
		scr.error("status(): Descriptor 128 Bit Size Not Supported\n");
		return;
	#endif
	default:
		_badframe = true;
		scr.error("status(): Invalid Descriptor Size");
		return;
	}

	// And the holes if any
	for (std::list<saratoga::hole>::iterator i = h->first(); i != h->last(); i++)
	{
		_holes.add(i->starts(), i->length());
		switch(descriptor.get())
		{
		case F_DESCRIPTOR_16:
			tmp_16 = htons((uint16_t) i->starts());
			memcpy(sbufp, &tmp_16, sizeof(uint16_t));
			sbufp += sizeof(uint16_t);
			tmp_16 = htons((uint16_t) i->ends());
			memcpy(sbufp, &tmp_16, sizeof(uint16_t));
			sbufp += sizeof(uint16_t);
			break;
		case F_DESCRIPTOR_32:
			tmp_32 = htonl((uint32_t) i->starts());
			memcpy(sbufp, &tmp_32, sizeof(uint32_t));
			sbufp += sizeof(uint32_t);
			tmp_32 = htonl((uint32_t) i->ends());
			memcpy(sbufp, &tmp_32, sizeof(uint32_t));
			sbufp += sizeof(uint32_t);
			break;
		case F_DESCRIPTOR_64:
			tmp_64 = htonll(i->starts());
			memcpy(sbufp, &tmp_64, sizeof(uint64_t));
			sbufp += sizeof(uint64_t);
			tmp_64 = htonll(i->ends());
			memcpy(sbufp, &tmp_64, sizeof(uint64_t));
			sbufp += sizeof(uint64_t);
			break;
		case F_DESCRIPTOR_128:
		#ifdef UINT128_T
			tmp_128 = htonlll(i->starts());
			memcpy(sbufp, &tmp_128, sizeof(uint128_t));
			sbufp += sizeof(uint128_t);
			tmp_128 = htonlll(i->ends());
			memcpy(sbufp, &tmp_128, sizeof(uint128_t));
			sbufp += sizeof(uint128_t);
			break;
		#else
			scr.error("status(): Descriptor 128 Bit Size Not Supported\n");
			_badframe = true;
			return;
		#endif
		default:
			scr.error("status(): Invalid Descriptor Size");
			_badframe = true;
			return;
		}
	}
}

/*
 * Given a buffer and length, assemble the status
 */
status::status(char *payload, const size_t pl)
{
	offset_t	tmp1;
	offset_t	tmp2;

	size_t	paylen = pl;

	// Copy the frame info
	_badframe = false;
	_paylen = paylen;
	_payload = new char[_paylen];
	memcpy(_payload, payload, paylen);

	// Get the frame info
	// flags and payload

	// Assemble the flags info
	_flags = (flag_t) ntohl(*(uint32_t *) payload);
	payload += sizeof(flag_t);
	paylen -= sizeof(flag_t);

	Fversion	version = _flags.get();
	Fframetype	frametype = _flags.get();

	if (version.get() != F_VERSION_1)
	{
		scr.error("Status: Bad Saratoga Version");
		_badframe = true;
		return;
	}
	if (frametype.get() != F_FRAMETYPE_STATUS)
	{
		scr.error("Status: Not s STATUS frame");
		_badframe = true;
		return;
	}

	Fdescriptor descriptor = _flags.get();
	Freqtstamp reqtstamp = _flags.get();
	Fmetadatarecvd	metadatarecvd = _flags.get();
	Fallholes	allholes = _flags.get();
	Ferrcode	errcode = _flags.get();
	
	// Assemble the status info

	// Session ID
	_session = (session_t) ntohl(*(uint32_t *) payload);
	payload += sizeof(session_t);
	paylen -= sizeof(session_t);

	// Optional timestamp on status frame
	if (reqtstamp.get() == F_TIMESTAMP_YES)
	{
		_timestamp = timestamp(payload);
		payload += _timestamp.length();
		paylen -= _timestamp.length();
	}
	else
		_timestamp = timestamp(); 

	// Progress & Inresponseto 
	switch(descriptor.get())
	{
	case F_DESCRIPTOR_16:
		_progress = (offset_t) ntohs(*(uint16_t *) payload);
		payload += sizeof(uint16_t);
		paylen -= sizeof(uint16_t);
		_inresponseto = (offset_t) ntohs(*(uint16_t *) payload);
		payload += sizeof(uint16_t);
		paylen -= sizeof(uint16_t);
		break;
	case F_DESCRIPTOR_32:
		_progress = (offset_t) ntohl(*(uint32_t *) payload);
		payload += sizeof(uint32_t);
		paylen -= sizeof(uint32_t);
		_inresponseto = (offset_t) ntohl(*(uint32_t *) payload);
		payload += sizeof(uint32_t);
		paylen -= sizeof(uint32_t);
		break;
	case F_DESCRIPTOR_64:
		_progress = (offset_t) ntohll(*(uint64_t *) payload);
		payload += sizeof(uint64_t);
		paylen -= sizeof(uint64_t);
		_inresponseto = (offset_t) ntohll(*(uint64_t *) payload);
		payload += sizeof(uint64_t);
		paylen -= sizeof(uint64_t);
		break;
	case F_DESCRIPTOR_128:
#ifdef UINT128_T
		_progress = (offset_t) ntohlll(*(uint128_t *) payload);
		payload += sizeof(uint128_t);
		paylen -= sizeof(uint128_t);
		_inresponseto = (offset_t) ntohlll(*(uint128_t *) payload);
		payload += sizeof(uint128_t);
		paylen -= sizeof(uint128_t);
		break;
#else
		scr.error("status(frame): Descriptor 128 Bit Size Not Supported\n");
		_badframe = true;
		return;
#endif
	default:
		_badframe = true;
		return;
	}

	// Anything left are holes
	tmp1 = 0;
	tmp2 = 0;
	int holecnt = paylen / (descriptor.length() * 2);
	for (int i = 0; i < holecnt; i++)
	{
		switch(descriptor.get())
		{
		case F_DESCRIPTOR_16:
			tmp1 = (offset_t) ntohs(*(uint16_t *) payload);
			payload += sizeof(uint16_t);
			paylen -= sizeof(uint16_t);
			tmp2 = (offset_t) ntohs(*(uint16_t *) payload);
			payload += sizeof(uint16_t);
			paylen -= sizeof(uint16_t);
			break;
		case F_DESCRIPTOR_32:
			tmp1 = (offset_t) ntohl(*(uint32_t *) payload);
			payload += sizeof(uint32_t);
			paylen -= sizeof(uint32_t);
			tmp2 = (offset_t) ntohl(*(uint32_t *) payload);
			payload += sizeof(uint32_t);
			paylen -= sizeof(uint32_t);
			break;
		case F_DESCRIPTOR_64:
			tmp1 = (offset_t) ntohll(*(uint64_t *) payload);
			payload += sizeof(uint64_t);
			paylen -= sizeof(uint64_t);
			tmp2 = (offset_t) ntohll(*(uint64_t *) payload);
			payload += sizeof(uint64_t);
			paylen -= sizeof(uint64_t);
			break;
		case F_DESCRIPTOR_128:
	#ifdef UINT128_T
			tmp1 = (offset_t) ntohlll(*(uint128_t *) payload);
			payload += sizeof(uint128_t);
			paylen -= sizeof(uint128_t);
			tmp2 = (offset_t) ntohlll(*(uint128_t *) payload);
			payload += sizeof(uint128_t);
			paylen -= sizeof(uint128_t);
			break;
	#else
			scr.error("status(frame): Descriptor 128 Bit Size Not Supported\n");
			_badframe = true;
			return;
	#endif
		default:
			_badframe = true;
			return;
		}
		_holes.add(tmp1, tmp2);
	}
}

string
status::errprint()
{	
	Ferrcode	errcode = _flags.get();
	return errcode.print(); 
}

/*
 * Print out the status flags & holes
 */
string
status::print()
{
	char	tmp[128];
	string	s;

	if (_badframe)
	{
		s = "status::print(): Bad STATUS Frame";
		scr.error(s);
		return(s);
	}
	/*
	 * These are the valid flags types applicable
	 * to a status
	 */
	Fversion	version = _flags.get();
	Fframetype	frametype = _flags.get();
	Fdescriptor	descriptor = _flags.get();
	Freqtstamp 	reqtstamp = _flags.get();
	Fmetadatarecvd	metadatarecvd = _flags.get();
	Fallholes	allholes = _flags.get();
	Ferrcode	errcode = _flags.get();
	
	s = frametype.print();
	s += printflags("STATUS FLAGS", this->flags());
	s += "    ";
	s += version.print();
	s += "\n    ";
	s += descriptor.print();
	s += "\n    ";
	s += reqtstamp.print();
	s += "\n    ";
	s += metadatarecvd.print();
	s += "\n    ";
	s += allholes.print();
	s += "\n    ";
	s += errcode.print();
	s += "\n    ";
	sprintf(tmp, 
		"Session: %" PRIu32 "", (uint32_t) _session);
	s += tmp;
	if (reqtstamp.get() == F_TIMESTAMP_YES)
	{
		s += "\n    ";
		s += _timestamp.asctime();
	}
	s += "\n    ";
	sprintf(tmp, 
		"Progress: %" PRIu64 "", (uint64_t) _progress);
	s += tmp;
	s += "\n    ";
	sprintf(tmp, 
		"In Response To: %" PRIu64 "", (uint64_t) _inresponseto);
	s += tmp;
	int holenumb = 0;
	for (std::list<saratoga::hole>::iterator i = _holes.first(); i != _holes.last(); i++)
	{
		holenumb++;
		sprintf(tmp, 
			"Hole[%d]:%" PRIu64 " to %" PRIu64 "", 
				holenumb, 
				(uint64_t) i->starts(), 
				(uint64_t) i->ends());
		s += tmp;
		s += "\n    ";
	}
	if (holenumb == 0)
		s += "\n    No Holes";
	return(s);
}

// Set various flags applicable to status frames
enum f_descriptor
status::descriptor(enum f_descriptor d)
{
	Fdescriptor	descriptor = d;
	_flags += descriptor;
	return descriptor.get();
}

enum f_metadatarecvd
status::metadatarecvd(enum f_metadatarecvd f)
{
	Fmetadatarecvd	metadatarecvd = f;
	_flags += metadatarecvd;
	return metadatarecvd.get();
}

enum f_allholes
status::allholes(enum f_allholes h)
{
	Fallholes	allholes = h;
	_flags += allholes;
	return allholes.get();
}

enum f_reqholes
status::reqholes(enum f_reqholes h)
{
	Freqholes	fh = h;
	_flags += fh;
	return fh.get();
}

enum f_reqtstamp
status::reqtstamp(enum f_reqtstamp t)
{
	Freqtstamp	reqtstamp = t;
	_flags += reqtstamp;
	return reqtstamp.get();
}

enum f_errcode
status::errcode(enum f_errcode t)
{
	Ferrcode	errcode = t;
	_flags += errcode;
	return errcode.get();
}

// Get various flags applicable to status frames
enum f_descriptor
status::descriptor() {
	Fdescriptor t = _flags.get();
	return t.get();
}

enum f_metadatarecvd
status::metadatarecvd() {
	Fmetadatarecvd t = _flags.get();
	return t.get();
}

enum f_errcode
status::errcode() {
	Ferrcode s = _flags.get();
	return s.get();
}

enum f_reqholes
status::reqholes() {
	Freqholes e = _flags.get();
	return e.get();
}

enum f_allholes
status::allholes() {
	Fallholes e = _flags.get();
	return e.get();
}

enum f_reqtstamp
status::reqtstamp() {
	Freqtstamp t = _flags.get();
	return t.get();
}

}; // Namespace saratoga
