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

#ifndef _DATA_H
#define _DATA_H

#include <iostream>
#include <cstring>
#include <string>
using namespace std;

#include "saratoga.h"
#include "sarflags.h"
#include "screen.h"
#include "timestamp.h"
#include "frame.h"

namespace saratoga
{
/*
 **********************************************************************
 * DATA
 **********************************************************************
 */

class data: public frame
{
private:
	Fflag		_flags;		// Data's Flags
        session_t	_session;	// THe session ID
        offset_t	_offset;	// How far into the transfer we are
        timestamp	_timestamp;// If we have a timestamp what is it
        char		*_dbuf;		// THe buffer holding the data
        size_t		_dbuflen;	// How long the buffer is

protected:
	bool		_badframe;	// Are we a good or bad data frame
	char		*_payload;	// Complete payload of frame
	size_t		_paylen;	// Length of payload
public:
	const static size_t	maxframesize = MAXDATA;	// Maximum size of a data frame

	// This is how we assemble a local data
	// To get ready for transmission
	// The optional timestamp is created within
	// this constructor. There is a data::timestamp()
	// to amend it if needed.
	data( const enum f_descriptor&,
		const enum f_transfer&,
		const enum f_reqstatus&,
		const enum f_eod&,
		const enum f_reqtstamp&, // If we want a timestamp
		const session_t&,	// Session NUmber
		const offset_t&,	// offset 
		const char *, 	// payload
		const size_t&);	// length of payload

	// We have received a remote frame that is DATA
	data(char *,	// Pointer to buffer received
		const size_t);	// Total length of buffer

	~data() { this->clear(); };

	void clear() {
		if (_paylen > 0) delete[] _payload;
		_paylen = 0;
		_timestamp = T_TSTAMP_32;
		_session = 0;
		_offset = 0;
		_dbuf = nullptr;
		_dbuflen = 0;
		_flags = 0;
		_badframe = true;
	};

	// Copy constructor
	data(const data& old) {
		if (old._paylen > 0)
		{
			_payload = new char[old._paylen];
			memcpy(_payload, old._payload, old._paylen);
		}
		else
			_payload = nullptr;
		_paylen = old._paylen;
		_flags = old._flags;
		_session = old._session;
		_offset = old._offset;

		Fdescriptor descriptor = _flags.get();	
		_dbuf = _payload + 
			sizeof(flag_t) + 
			sizeof(session_t) + 
			descriptor.length();
		Freqtstamp reqtstamp = _flags.get();
		if (reqtstamp.get() == F_TIMESTAMP_YES)
		{
			_timestamp = old._timestamp;
			_dbuf += _timestamp.length();
		}
		else
			_timestamp = T_TSTAMP_32;
		_dbuflen = old._dbuflen; 
	}

	// Assignment of existing data
	data& operator=(const data& old) {
		if (old._paylen > 0)
		{
			_payload = new char[old._paylen];
			memcpy(_payload, old._payload, old._paylen);
		}
		else
			_payload = nullptr;
		_paylen = old._paylen;
		_flags = old._flags;
		_session = old._session;
		_offset = old._offset;

		_dbuf = _payload + 
			sizeof(flag_t) + 
			sizeof(session_t);
		Fdescriptor descriptor = _flags.get();	
		_dbuf += descriptor.length();
		Freqtstamp reqtstamp = _flags.get();
		if (reqtstamp.get() == F_TIMESTAMP_YES)
		{
			_timestamp = old._timestamp;
			_dbuf += _timestamp.length();
		}
		else
			_timestamp = T_TSTAMP_32;
		_dbuflen = old._dbuflen; 
		return(*this); }

	bool	badframe() { return _badframe; };
	size_t	paylen() { return _paylen; };
	char	*payload() { return _payload; };

	// Get various flags applicable to data
	enum f_descriptor	descriptor();
	enum f_transfer		transfer();
	enum f_reqstatus		reqstatus();
	enum f_eod		eod();
	enum f_reqtstamp		reqtstamp();

	// Set various flags applicable to data
	enum f_descriptor	descriptor(f_descriptor);
	enum f_transfer		transfer(f_transfer);
	enum f_reqstatus		reqstatus(f_reqstatus);
	enum f_eod		eod(f_eod);
	enum f_reqtstamp		reqtstamp(f_reqtstamp);
	
	// What is the session number for this transaction
	session_t	session() { return _session; };

	// How far into the transfer are we and we can set it
	offset_t	offset() { return _offset; };
	offset_t	offset(offset_t o) { _offset = o; return _offset; };

	// The data frames flags
	flag_t	flags() { return _flags.get(); };

	// Yes I am a data frame
	f_frametype type() { return F_FRAMETYPE_DATA; };

	// What is the length of the data buffer in this frame
	char	*dbuf() { return _dbuf; };
	size_t	dbuflen() { return _dbuflen; };

	ssize_t rx() { return(-1); };

	ssize_t tx(sarnet::udp *sock) { return(sock->tx(_payload, _paylen)); };

	string	print();
};

} // Namespace saratoga

#endif // _DATA_H

