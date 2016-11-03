/*

 Copyright (c) 2015, Charles Smith
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification,
 are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
 this
      list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
 this
      list of conditions and the following disclaimer in the documentation
 and/or
      other materials provided with the distribution.
    * Neither the name of Vallona Networks nor the names of its contributors
      may be used to endorse or promote products derived from this software
 without
      specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 ADVISED
 OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#ifndef _REQUEST_H
#define _REQUEST_H

#include "dirent.h"
#include "frame.h"
#include "ip.h"
#include "saratoga.h"
#include "sarflags.h"
#include "screen.h"
#include "timestamp.h"
#include <cstring>
#include <iostream>
#include <string>

using namespace std;

namespace saratoga {

/*
 **********************************************************************
 * AUTH
 **********************************************************************
 */

// auth is not fully implemented in this version of saratoga
// We should do something like take the SHA1 of the file and
// the ip address. Then  encrypt it using public key.
// Put that in the buffer and send it and then
// decrpyt with the private key at the
// other end to ensure the request is a valid and not spoofed.
class auth
{
private:
  char* _auth;     // THe authentication buffer
  size_t _authlen; // Length of the buffer
public:
  // At the moment lets pass it hte local IP address and file name
  // What we do with it in the future will be in this constructor
  auth()
  {
    _auth = nullptr;
    _authlen = 0;
  };
  auth(const string& _fname);
  auth(char*, const size_t);

  // We have received a remote frame that is a REQUEST
  // Get it into a request class
  ~auth() { this->clear(); };

  void clear()
  {
    if (_authlen > 0)
      delete[] _auth; // Base class variable
    _authlen = 0;     // Base class variable
  };

  // Copy contructor
  auth(const saratoga::auth& old)
  {
    if (old._authlen > 0) {
      _auth = new char[old._authlen];
      memcpy(_auth, old._auth, old._authlen);
    } else
      _auth = nullptr;
    _authlen = old._authlen;
  };

  auth& operator=(const saratoga::auth& old)
  {
    if (old._authlen > 0) {
      _auth = new char[old._authlen];
      memcpy(_auth, old._auth, old._authlen);
    } else
      _auth = nullptr;
    _authlen = old._authlen;
    return (*this);
  };

  char* buf() { return _auth; };

  size_t length() { return _authlen; };

  // Print out the auth buf in hex
  string print();
};

/*
 **********************************************************************
 * REQUEST
 **********************************************************************
 */

class request : public frame
{
private:
  Fflag _flags;         // Saratoga flags
  session_t _session;   // The Session ID
  string _fname;        // File / Directory Name
  saratoga::auth _auth; // Everything after the fname '\0'
protected:
  bool _badframe; // Are we a good or bad data frame
  char* _payload; // Complete payload of frame
  size_t _paylen; // Length of payload
public:
  // This is how we assemble a local REQUEST
  // To get it ready for transmission
  request(f_requesttype req, // What tpye of request
          session_t sess,    // Unique session number
          string fname);     // File Name

  // We have received a remote frame that is a REQUEST
  // Get it into a request class
  request(char*,   // The Frames Payload
          size_t); // Length of the payload

  ~request() { this->clear(); };

  void clear()
  {
    if (_paylen > 0)
      delete[] _payload; // Base class variable
    _paylen = 0;         // Base class variable
    _badframe = true;    // We are not a valid frame
    _flags = 0;
    _session = 0;
    _fname.clear();
    _auth.clear();
  };

  // Copy constructor
  request(const request& old)
  {
    if (old._paylen > 0) {
      _payload = new char[old._paylen];
      memcpy(_payload, old._payload, old._paylen);
    } else
      _payload = nullptr;
    _paylen = old._paylen;
    _flags = old._flags;
    ;
    _session = old._session;
    _fname = old._fname;
    _auth = old._auth;
    _badframe = old._badframe;
  };

  request& operator=(const request& old)
  {
    if (old._paylen > 0) {
      _payload = new char[old._paylen];
      memcpy(_payload, old._payload, old._paylen);
    } else
      _payload = nullptr;
    _paylen = old._paylen;
    _flags = old._flags;
    ;
    _session = old._session;
    _fname = old._fname;
    _auth = old._auth;
    _badframe = old._badframe;
    return (*this);
  };

  bool badframe() { return _badframe; };
  size_t paylen() { return _paylen; };
  char* payload() { return _payload; };

  char* calcauth(string fname);

  // Yes I am a request
  f_frametype type() { return F_FRAMETYPE_REQUEST; };

  // F_REQUEST_NOACTION, GET, PUT, GETDELETE, PUTDELETE, DELETE, GETDIR
  f_requesttype reqtype()
  {
    Frequesttype f = _flags.get();
    return (f.get());
  };

  // What is the session #
  session_t session() { return _session; };
  session_t session(session_t s)
  {
    _session = s;
    return _session;
  };
  // What is the fname
  string fname() { return _fname; };
  string fname(string f)
  {
    _fname = f;
    return _fname;
  };

  // What is the auth
  char* authbuf() { return _auth.buf(); };
  size_t authlength() { return _auth.length(); };

  // What are the requests flags
  flag_t flags() { return _flags.get(); };

  // These and only these flags can be changed during a session

  // Transmit a request to a socket
  ssize_t tx(sarnet::udp* sock) { return (sock->tx(_payload, _paylen)); };

  ssize_t rx();

  // Print out the request details
  string print();
};

}; // Namespace saratoga

#endif // _REQUEST_H
