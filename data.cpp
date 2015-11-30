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
#include "data.h"
#include "globals.h"

using namespace std;

namespace saratoga
{

// Create a data frame from scratch
data::data( const enum f_descriptor& des,
	const enum f_transfer& tfr,
	const enum f_reqstatus& stat,
	const enum f_eod& eodf,
	const enum f_reqtstamp& ts,
	const session_t&	session,
	const offset_t& 	offset,
	const char 		*dbuf, 
	const size_t& 		dblen)
{
	uint16_t	tmp_16;
	uint32_t	tmp_32;
	uint64_t	tmp_64;
#ifdef UINT128_T
	uint128_t	tmp_128;
#endif


	Fversion	version = F_VERSION_1;
	Fframetype	frametype = F_FRAMETYPE_DATA;
	Fdescriptor	descriptor = des;
	Ftransfer	transfer = tfr;
	Freqstatus	reqstatus = stat;
	Feod		eod = eodf;
	Freqtstamp	reqtstamp = ts;
	
	// Work out how big our frame has to be allocate it
	size_t fsize = sizeof(flag_t) + sizeof(session_t); // Flag + Session
	fsize += descriptor.length(); // Offset
	if (reqtstamp.get() == F_TIMESTAMP_YES) // Optional timestamp
	{
		// Create the timestamp based upon the current
		// command set for the timestamp type
		_timestamp = timestamp(c_timestamp.ttype());
		fsize += _timestamp.length();
	}
	else // Default place holder
		_timestamp = timestamp(T_TSTAMP_32);
	fsize += dblen;
	_payload = new char[fsize];
	_paylen = fsize;
	char	*dbufp = _payload;

	// Set and copy flags
	_badframe = false;
	
	_flags += version;
	_flags += frametype;
	_flags += descriptor;
	_flags += transfer;
	_flags += reqtstamp;
	_flags += reqstatus;
	_flags += eod;

	tmp_32 = htonl(_flags.get());
	memcpy(dbufp, &tmp_32, sizeof(flag_t));
	dbufp += sizeof(flag_t);

	// Set and copy session
	_session = session;
	tmp_32 = htonl(session);
	memcpy(dbufp, &tmp_32, sizeof(session_t));
	dbufp += sizeof(session_t);

	// Set and copy offset
	_offset = offset;
	switch(descriptor.get())
	{
	case F_DESCRIPTOR_16:
		tmp_16 = htons(_offset);
		memcpy(dbufp, &tmp_16, sizeof(uint16_t));
		break;
	case F_DESCRIPTOR_32:
		tmp_32 = htonl(_offset);
		memcpy(dbufp, &tmp_32, sizeof(uint32_t));
		break;
	case F_DESCRIPTOR_64:
		tmp_64 = htonll(_offset);
		memcpy(dbufp, &tmp_64, sizeof(uint64_t));
		break;
	case F_DESCRIPTOR_128:
	#ifdef UINT128_T
		tmp_128 = htonlll(_offset);
		memcpy(dbufp, &tmp_128, sizeof(uint128_t));
		break;
	#else
		scr.error("data(payload, len): Descriptor 128 Bit Size Not Supported\n");
		_badframe = true;
		return;
	#endif
	default:
		scr.error("data(payload, len): Invalid Descriptor Size");
		_badframe = true;
		return;
	}
	dbufp += descriptor.length();

	// Set and copy the timestamp
	// If we have a timestamp then lets make it by default ZULU time
	if (reqtstamp.get() == F_TIMESTAMP_YES)
	{
		memcpy(dbufp, _timestamp.hton(), _timestamp.length());
		dbufp += _timestamp.length();
	}

	// And finally the actual data buffer
	memcpy(dbufp, dbuf, dblen);
	_dbuf = dbufp; // No need to copy it it's in the frame just point to it
	_dbuflen = dblen;
}

/*
 * Given a buffer and length, assemble the data
 * this is what we do when re receive a saratoga frame
 * so we do the ntoh's
 */
data::data(char *payload,
	size_t paylen)
{
	_badframe = false;
	_paylen = paylen;
	_payload = new char[_paylen];
	memcpy(_payload, payload, paylen);

	// Get the frame info
	// flags and payload

	// Assemble the flags info
	_flags = ntohl(*(uint32_t *) payload);
	payload += sizeof(flag_t);
	paylen -= sizeof(flag_t);
	Fversion	version = _flags.get();
	Fframetype	frametype = _flags.get();

	if (version.get() != F_VERSION_1)
	{
		scr.error("Data: Bad Saratoga Version");
		_badframe = true;
		return;
	}
	if (frametype.get() != F_FRAMETYPE_DATA)
	{
		scr.error("Data: Not a DATA frame");
		_badframe = true;
		return;
	}
	Fdescriptor descriptor = _flags.get();
	Freqtstamp reqtstamp = _flags.get();
	
	// Assemble the data info

	// Session ID
	_session = ntohl(*(uint32_t *) payload);
	payload += sizeof(session_t);
	paylen -= sizeof(session_t);

	// Offset into file/stream
	switch(descriptor.get())
	{
	case F_DESCRIPTOR_16:
		_offset = (offset_t) ntohs(*(uint16_t *) payload);
		break;
	case F_DESCRIPTOR_32:
		_offset = (offset_t) ntohl(*(uint32_t *) payload);
		break;
	case F_DESCRIPTOR_64:
		_offset = (offset_t) ntohll(*(uint64_t *) payload);
		break;
	case F_DESCRIPTOR_128:
#ifdef UINT128_T
		_offset = (offset_t) ntohlll(*(uint128_t *) payload);
		break;
#else
		scr.error("data(frame): Descriptor 128 Bit Size Not Supported\n");
		_badframe = true;
		return;
#endif
	default:
		_badframe = true;
		return;
	}
	payload += descriptor.length();
	paylen -= descriptor.length();

	// Optional timestamp on data frame
	if (reqtstamp.get() == F_TIMESTAMP_YES)
	{
		_timestamp = timestamp(payload);
		payload += _timestamp.length();
		paylen -= _timestamp.length();
	}
	else
		_timestamp = timestamp(); // Place holder

	_dbuflen = paylen;
	// We dont need to copy dbuf we just need to point to it in the payload
	_dbuf = payload;
}

/*
 * Print out the data flags, timestamp buffer length and
 */
string
data::print()
{
	char	tmp[128];
	string	s;
	/*
	 * These are the valid flags types applicable
	 * to a data
	 */
	if (_badframe)
	{
		s = "data::print():: Bad DATA Frame";
		scr.error(s);
		return(s);
	}
	Fversion	version = _flags.get();
	Fframetype	frametype = _flags.get();
	Fdescriptor	descriptor = _flags.get();
	Ftransfer	transfer = _flags.get();
	Freqtstamp	reqtstamp = _flags.get();
	Freqstatus	reqstatus = _flags.get();
	Feod		eod = _flags.get();

	s = frametype.print();
	s += printflags("DATA FLAGS", this->flags());
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
	s += eod.print();
	s += "\n    ";

	sprintf(tmp, 
		"Session: %" PRIu32 "", (uint32_t) _session);
	s += tmp;
	s += "\n    ";
	sprintf(tmp, 
		"Offset: %" PRIu64 "", (uint64_t) _offset);
	s += tmp;
	if (reqtstamp.get() == F_TIMESTAMP_YES)
	{
		s += "\n    ";
		s += _timestamp.asctime();
	}
	s += "\n    ";
	sprintf(tmp, "Buffer Length: %lu", _dbuflen);
	s += tmp;
	return(s);
}
// Set various flags applicable to data frames

enum f_descriptor
data::descriptor(enum f_descriptor d)
{
	Fdescriptor	descriptor = d;
	_flags += descriptor;
	return descriptor.get();
}

enum f_transfer
data::transfer(enum f_transfer t)
{
	Ftransfer	transfer = t;
	_flags += transfer;
	return transfer.get();
}

enum f_reqstatus
data::reqstatus(enum f_reqstatus t)
{
	Freqstatus	reqstatus = t;
	_flags += reqstatus;
	return reqstatus.get();
}

enum f_eod
data::eod(enum f_eod t)
{
	Feod	eod = t;
	_flags += eod;
	return eod.get();
}

enum f_reqtstamp
data::reqtstamp(enum f_reqtstamp t)
{
	Freqtstamp	reqtstamp = t;
	_flags += reqtstamp;
	return reqtstamp.get();
}

// Get various flags applicable to data frames
enum f_descriptor
data::descriptor() {
	Fdescriptor t = _flags.get();
	return t.get();
}

enum f_transfer
data::transfer() {
	Ftransfer t = _flags.get();
	return t.get();
}

enum f_reqstatus
data::reqstatus() {
	Freqstatus s = _flags.get();
	return s.get();
}

enum f_eod
data::eod() {
	Feod e = _flags.get();
	return e.get();
}

enum f_reqtstamp
data::reqtstamp() {
	Freqtstamp t = _flags.get();
	return t.get();
}

} // Namespace saratoga
