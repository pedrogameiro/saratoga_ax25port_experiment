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

#ifndef _BEACON_H
#define _BEACON_H

#include <cstring>
#include <iostream>
#include <string>
using namespace std;

#include "dirent.h"
#include "frame.h"
#include "ip.h"
#include "saratoga.h"
#include "sarflags.h"
#include "screen.h"
#include "timestamp.h"

namespace saratoga {
/*
 **********************************************************************
 * BEACON
 **********************************************************************
 */

class beacon : public frame
{
private:
  Fflag _flags;        // Saratoga flags
  offset_t _freespace; // Advertised freespace
  string _eid;         // The EID advertised

  bool _badframe;
  char* _payload;
  size_t _paylen;

public:
  // This is how we assemble a local beacon
  // To get it ready for transmission
  // Create a beacon with no freespace identified
  beacon(const enum f_descriptor&, const enum f_stream&,
         const enum f_txwilling&, const enum f_rxwilling&,
         const string&); // The EID

  // This is how we assemble a local beacon
  // To get it ready for transmission
  // Create a beacon with freespace identified
  beacon(const enum f_descriptor&, const enum f_stream&,
         const enum f_txwilling&, const enum f_rxwilling&,
         sardir::fsinfo*, // The Freespace
         const string&);  // The EID

  // We have received a remote frame that is a BEACON
  // Get it into a beacon class
  beacon(const char*,    // The Frames Payload
         const size_t&); // Length of the payload

  ~beacon() { this->clear(); };

  void clear()
  {
    if (_paylen > 0)
      delete[] _payload; // Base class variable
    _paylen = 0;         // Base class variable
    _badframe = true;    // We are not a valid frame
    _flags = 0;          // Beacon variable
    _freespace = 0;      // Beacon variable
    _eid.clear();        // Beacon variable
  };

  // Copy Constructor
  beacon(const beacon& old)
  {
    if (old._paylen > 0) {
      _payload = new char[old._paylen];
      memcpy(_payload, old._payload, old._paylen);
    } else
      _payload = nullptr;
    _paylen = old._paylen;
    _flags = old._flags;
    _freespace = old._freespace;
    _eid = old._eid;
  }

  // Assignment operator
  beacon& operator=(const beacon& old)
  {
    if (old._paylen > 0) {
      _payload = new char[old._paylen];
      memcpy(_payload, old._payload, old._paylen);
    } else
      _payload = nullptr;
    _flags = old._flags;
    _freespace = old._freespace;
    _eid = old._eid;
    return (*this);
  }

  bool badframe() { return _badframe; };
  char* payload() { return _payload; };
  size_t paylen() { return _paylen; };

  // Yes I am a beacon
  f_frametype type() { return F_FRAMETYPE_BEACON; };

  // What is the beacon eid
  string eid() { return _eid; };

  // What is the beacons advertised freespace
  offset_t freespace() { return _freespace; };

  // What are the beacons flags
  flag_t flags() { return _flags.get(); };

  // These and only these flags can be changed during a session

  // Set tx or off return willing on,off,capable
  enum f_txwilling txwilling(const enum f_txwilling&); // Set
  enum f_txwilling txwilling();                        // What is it ?

  // Set rx or off return willing on,off,capable
  enum f_rxwilling rxwilling(const enum f_rxwilling&); // Set
  enum f_rxwilling rxwilling();                        // What is it ?

  // Transmit a beacon to a socket
  ssize_t tx(sarnet::udp* sock) { return (sock->tx(_payload, _paylen)); };

  // Receive a beacon
  ssize_t rx()
  {
    string s = "BEACON RX:";
    s += this->print();
    return (-1);
  };

  // Print out the beacon details
  string print();
};

}; // Namespace saratoga

#endif // _BEACON_H
