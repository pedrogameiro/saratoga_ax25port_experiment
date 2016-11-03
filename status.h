/*

 Copyright (c) 2011, Charles Smith
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

#ifndef _STATUS_H
#define _STATUS_H

#include <cstring>
#include <iostream>
#include <string>

#include "frame.h"
#include "holes.h"
#include "ip.h"
#include "saratoga.h"
#include "sarflags.h"
#include "screen.h"
#include "timestamp.h"

using namespace std;

namespace saratoga {
/*
 **********************************************************************
 * STATUS
 **********************************************************************
 */

class status : public frame
{
private:
  Fflag _flags;           // status's Flags
  session_t _session;     // The session ID
  timestamp _timestamp;   // If we have a timestamp what is it
  offset_t _progress;     // Progress offset
  offset_t _inresponseto; // Covering to offset
  offset_t _maxholes;     // Maximum # holes in a frame
  saratoga::holes _holes; // Holes for this frame
protected:
  bool _badframe; // Are we a good or bad data frame
  char* _payload; // Complete payload of frame
  size_t _paylen; // Length of payload
public:
  // This is how we assemble a local status
  // To get ready for transmission
  // No timestamp
  status(const enum f_descriptor, // Flags
         const enum f_metadatarecvd, const enum f_allholes,
         const enum f_reqholes, const enum f_errcode,
         const session_t,   // Session
         const offset_t,    // progress
         const offset_t,    // inresponseto
         saratoga::holes*); // the holes

  // With  the optional timestamp
  status(const enum f_descriptor, // Flags
         const enum f_metadatarecvd, const enum f_allholes,
         const enum f_reqholes, const enum f_errcode,
         const session_t,   // Session
         const timestamp&,  // Optional Timestamp
         const offset_t,    // progress
         const offset_t,    // inresponseto
         saratoga::holes*); // the holes

  // We have received a remote frame that is STATUS
  status(char*,         // Pointer to buffer received
         const size_t); // Total length of buffer

  ~status() { this->clear(); };

  void clear()
  {
    if (_paylen > 0)
      delete[] _payload; // From frame
    _paylen = 0;         // From frame
    _flags = 0;
    _session = 0;
    _timestamp = timestamp();
    _progress = 0;
    _inresponseto = 0;
    _holes.clear();
  };

  // Copy Constructor
  status(const status& old)
  {
    if (old._paylen > 0) {
      _payload = new char[old._paylen];
      memcpy(_payload, old._payload, old._paylen);
    } else
      _payload = nullptr;
    _paylen = old._paylen;
    _flags = old._flags;
    _session = old._session;
    _timestamp = old._timestamp;
    _progress = old._progress;
    _inresponseto = old._inresponseto;
    _maxholes = old._maxholes;
    _holes = old._holes;
    _badframe = old._badframe;
  }

  status& operator=(const status& old)
  {
    if (old._paylen > 0) {
      _payload = new char[old._paylen];
      memcpy(_payload, old._payload, old._paylen);
    } else
      _payload = nullptr;
    _paylen = old._paylen;
    _flags = old._flags;
    _session = old._session;
    _timestamp = old._timestamp;
    _progress = old._progress;
    _inresponseto = old._inresponseto;
    _maxholes = old._maxholes;
    _holes = old._holes;
    _badframe = old._badframe;
    return (*this);
  };

  bool badframe() { return _badframe; };
  size_t paylen() { return _paylen; };
  char* payload() { return _payload; };

  // Get various flags applicable to status
  enum f_descriptor descriptor();
  enum f_metadatarecvd metadatarecvd();
  enum f_errcode errcode();
  enum f_allholes allholes();
  enum f_reqholes reqholes();
  enum f_reqtstamp reqtstamp();

  // Set various flags applicable to status
  enum f_descriptor descriptor(enum f_descriptor);
  enum f_metadatarecvd metadatarecvd(enum f_metadatarecvd);
  enum f_errcode errcode(enum f_errcode);
  enum f_allholes allholes(enum f_allholes);
  enum f_reqholes reqholes(enum f_reqholes);
  enum f_reqtstamp reqtstamp(enum f_reqtstamp);

  // What is the session number for this transaction
  session_t session() { return _session; };

  timestamp tstamp() { return _timestamp; };

  // Progress and Inresponsto offsets
  offset_t progress() { return _progress; };
  offset_t progress(offset_t o)
  {
    _progress = o;
    return _progress;
  };
  offset_t inresponseto() { return _inresponseto; };
  offset_t inresponseto(offset_t o)
  {
    _inresponseto = o;
    return _inresponseto;
  };

  holes* holesptr() { return &_holes; };
  offset_t holecount() { return _holes.count(); };

  // The status frames flags
  flag_t flags() { return _flags.get(); };

  // Yes I am a status frame
  f_frametype type() { return F_FRAMETYPE_STATUS; };

  ssize_t tx(sarnet::udp* sock) { return (sock->tx(_payload, _paylen)); };

  ssize_t rx()
  {
    string s = "STATUS RX: ";
    s += this->print();
    cout << s << endl;
    return (-1);
  };

  string errprint();
  string print();
};

} // Namespace saratoga

#endif // _STATUS_H
