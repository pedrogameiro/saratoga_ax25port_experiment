/*
 
 Copyright (c) 2015, Charles Smith
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
#include "globals.h"
#include "frame.h"
#include "request.h"

using namespace std;

namespace saratoga
{

// We do something like take the SHA1 of the file and
// the EID. Then  encrypt it using public key. 
// Put that in the buffer and send it and then 
// decrpyt with the private key at the
// other end to ensure the transfer is good.
auth::auth(const string& fname)
{
	string	placeholder = fname;

	// Untill we write something based upon the placehoder
	_auth = (char *) 0;
	_authlen = 0;
	
//	scr.msg("Warning authentication field not supported in this saratoga version");
}

auth::auth(char *buf, const size_t len)
{
	_auth = new char[len];
	for (size_t i = 0; i < len; i++)
		_auth[i] = buf[i];
	_authlen = len;
}

/*
 * Print out the auth buffer in hex
 */
string
auth::print()
{
	string	s = "";
	
	s += "Authentication: ";
	s += printhex(_auth, _authlen);
	s += "\n";
	return(s);
}

// Create a request frame from scratch
request::request(enum f_requesttype reqtype,
	session_t	session,
	string		fname)
{
	uint32_t	tmp_32;

	Fversion	version = F_VERSION_1;
	Fframetype	frametype = F_FRAMETYPE_REQUEST;
	Fdescriptor	descriptor = c_descriptor.flag(); // Set decriptor size to global var
	Fstream		stream = F_STREAMS_NO;
	Fudptype	udptype = F_UDPONLY;
	Frequesttype	requesttype = reqtype;

	// We don;t implement auth in this version but when we do
	// it will all be inside the auth class and determined by what we key it by
	// at the moment the placeholder if the file name
	_auth = auth(fname);
	// Work out how big our frame has to be and allocate it
	_paylen = sizeof(flag_t) + sizeof(session_t) +
		fname.length() + 1 + _auth.length(); // Add 1 for the '\0' in filename
	_payload = new char[_paylen];
	char *rbufp = _payload;

	// Set and copy flags
	_badframe = false;
	
	_flags += version;
	_flags += frametype;
	_flags += descriptor;
	_flags += stream;
	_flags += udptype;
	_flags += requesttype;

	tmp_32 = htonl((uint32_t) _flags.get());
	memcpy(rbufp, &tmp_32, sizeof(flag_t));
	rbufp += sizeof(flag_t);

	// Set and copy session
	_session = session;
	tmp_32 = htonl(session);
	memcpy(rbufp, &tmp_32, sizeof(session_t));
	rbufp += sizeof(session_t);

	// Set and copy the fname with a trailing '\0'
	_fname = fname;
	memcpy(rbufp, fname.c_str(), fname.length());
	rbufp += fname.length();
	*rbufp = '\0';

	// Set and copy the authentication to the end
	if (_auth.length() > 0)
	{
		rbufp++;
		memcpy(rbufp, _auth.buf(), _auth.length());
	}
	scr.debug(6, "request::request(): Created REQUEST:");
	scr.debug(6, this->print());
}

/*
 * Given a buffer and length, assemble a request class
 * this is what we do when re receive a saratoga frame
 * so we do the ntoh's
 */
request::request(char *payload,
	size_t paylen)
{
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
	if (version.get() != F_VERSION_1)
	{
		scr.error("Request: Bad Saratoga Version");
		_badframe = true;
		return;
	}

	Fframetype	frametype = _flags.get();
	if (frametype.get() != F_FRAMETYPE_REQUEST)
	{
		scr.error("Request: Not a REQUEST frame");
		_badframe = true;
		return;
	}

	Fdescriptor descriptor = _flags.get();
	Fstream stream = _flags.get();
	Freqtstamp reqtstamp = _flags.get();
	Fudptype udptype = _flags.get();
	Frequesttype requesttype = _flags.get();
	
	// Assemble the request info

	// Session ID
	_session = (session_t) ntohl(*(uint32_t *) payload);
	payload += sizeof(session_t);
	paylen -= sizeof(session_t);

	_fname = "";
	if (paylen > 1024)
	{
		scr.error("Request: fname size exceeds 1024 bytes");
		_badframe = true;
		return;
	}
	while (paylen > 0)
	{
		if (*payload == '\0')
		{
			_fname += *payload;
			payload++;
			paylen--;
			break;
		}
		if (isprint(*payload))
		{
			_fname += *payload;
			payload++;
			paylen--;
		}
		else
		{
			scr.error("Request: invalid fname contains non printable character");
			_badframe = true;
			return;
		}
	}
	// What's left is the auth
	_auth = auth(payload, paylen);
	scr.debug(6, "request::request(): Read REQUEST:");
	scr.debug(6, this->print());
}


// Receive a request
ssize_t 
request::rx() 
{
	scr.debug(2, "RECEIVED: %s", this->print().c_str());
	return(-1);
}

/*
 * Print out the request flags
 */
string
request::print()
{
	char	tmp[2048];
	string	s = "";
	/*
	 * These are the valid flags types applicable
	 * to a REQUEST
	 */
	if (_badframe)
	{
		s = "Bad Frame: Invalid REQUEST";
		return(s);
	}
	Fversion	version = _flags.get();
	Fframetype	frametype = _flags.get();
	Fdescriptor	descriptor = _flags.get();
	Fstream		stream = _flags.get();
	Fudptype	udptype = _flags.get();
	Frequesttype	requesttype = _flags.get();


	s += frametype.print();
	s += printflags("REQUEST FLAGS", this->flags());
	s += "    ";
	s += version.print();
	s += "\n    ";
	s += descriptor.print();
	s += "\n    ";
	s += stream.print();
	s += "\n    ";
	s += udptype.print();
	s += "\n    ";
	s += requesttype.print();
	s += "\n    ";
	sprintf(tmp, "Session: %" PRIu32 "", (uint32_t) _session);
	s += tmp;
	s += "\n    ";
	sprintf(tmp, "Filename: %s", _fname.c_str());
	s += tmp;
	s += "\n    ";

	if (this->authlength() > 0)
	{
		s += "Authentication: ";
		s += printhex(this->authbuf(), this->authlength());
		s += "\n";
	}

	return(s);
}

} // Namespace saratoga

