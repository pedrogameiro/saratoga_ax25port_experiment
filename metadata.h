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

#ifndef _METADATA_H
#define _METADATA_H

#include <cstring>
#include <iostream>
#include <string>
using namespace std;

#include "checksum.h"
#include "dirent.h"
#include "fileio.h"
#include "frame.h"
#include "saratoga.h"
#include "sarflags.h"
#include "screen.h"
#include "timestamp.h"

namespace saratoga {
/*
 **********************************************************************
 * METADATA
 **********************************************************************
 */

class metadata : public frame
{
private:
  Fflag _flags;              // Frame Flags
  session_t _session;        // Session #
  checksums::checksum _csum; // What the Checksum is
  sardir::dirent _dir;       // What's in the directory entry
protected:
  bool _badframe; // Are we a good or bad data frame
  char* _payload; // Complete payload of frame
  size_t _paylen; // Length of payload
public:
  // This is how we assemble a local metadata
  // To get ready for transmission

  metadata(const enum f_descriptor, const enum f_transfer,
           const enum f_progress,
           const session_t,   // Session ID
           sarfile::fileio*); // File info

  // We have received a remote frame that is METADATA
  metadata(char*,         // Pointer to buffer received
           const size_t); // Total length of buffer

  ~metadata() { this->clear(); };

  void clear()
  {
    if (_paylen > 0)
      delete[] _payload; // Delete frames payload
    _csum.clear();       // Delete the checksum
    _dir.clear();        // Delete the directory entry
    _paylen = 0;         // Set it's length to 0
    _session = 0;
    _flags = 0;
  };

  // Copy Constructor
  metadata(const metadata& old)
  {
    if (old._paylen > 0) {
      _payload = new char[old._paylen];
      memcpy(_payload, old._payload, old._paylen);
    } else
      _payload = nullptr;
    _paylen = old._paylen;
    _flags = old._flags;
    _session = old._session;
    _csum = old._csum;
    _dir = old._dir;
    _badframe = old._badframe;
  };

  metadata& operator=(const metadata& old)
  {
    if (old._paylen > 0) {
      _payload = new char[old._paylen];
      memcpy(_payload, old._payload, old._paylen);
    } else
      _payload = nullptr;
    _paylen = old._paylen;
    _flags = old._flags;
    _session = old._session;
    _csum = old._csum;
    _dir = old._dir;
    _badframe = old._badframe;
    return (*this);
  };

  bool badframe() { return _badframe; };
  size_t paylen() { return _paylen; };
  char* payload() { return _payload; };

  flag_t flags() { return _flags.get(); };

  // Get various flags applicable to metadata
  enum f_descriptor descriptor();
  enum f_transfer transfer();
  enum f_progress progress();
  enum f_udptype metadata_udptype();
  enum f_csumlen csumlen();
  enum f_csumtype csumtype();

  // Set various flags applicable to metadata
  enum f_descriptor descriptor(f_descriptor);
  enum f_transfer transfer(f_transfer);
  enum f_progress progress(f_progress);
  enum f_udptype metadata_udptype(f_udptype);
  enum f_csumlen csumlen(f_csumlen);
  enum f_csumtype csumtype(f_csumtype);

  // Yes I am a METADATA frame
  f_frametype type() { return F_FRAMETYPE_METADATA; };

  // What is the session number for this transaction
  session_t session() { return _session; };

  // The checksum
  checksums::checksum checksum() { return _csum; };

  // The directory entry
  sardir::dirent dir() { return _dir; };

  // The number of bytes in the checksum
  size_t bytes();

  ssize_t tx(sarnet::udp* sock) { return (sock->tx(_payload, _paylen)); };

  ssize_t rx()
  {
    string s = "METADATA RX: ";
    s += this->print();
    cout << s << endl;
    return (-1);
  };

  string print();
};

} // Namespace saratoga

#endif // _METADATA_H
