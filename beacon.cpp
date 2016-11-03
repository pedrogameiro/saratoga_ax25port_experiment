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

#include <cstring>
#include <iostream>
#include <limits>
#include <string>

#include "beacon.h"
#include "frame.h"
#include "globals.h"
#include "saratoga.h"
#include "sarflags.h"
#include "screen.h"

using namespace std;

namespace saratoga {

beacon::beacon(const enum f_descriptor& d, // Descriptor Size
               const enum f_stream& s,     // Are we a stream
               const enum f_txwilling& t,  // Can we transmit
               const enum f_rxwilling& r,  // Can we receive
               const string& eid)          // EID of the peer as a string
{
  uint32_t tmp_32;

  /*
   * These are the valid flags types applicable
   * to a beacon, set them
   */
  Fversion version = F_VERSION_1;
  Fframetype frametype = F_FRAMETYPE_BEACON; // Of course we are a BEACON
  Fdescriptor descriptor = d;
  Fstream stream = s;
  Ftxwilling txwilling = t;
  Frxwilling rxwilling = r;
  Fudptype udptype = F_UDPONLY;             // We only handle UDP at the moment
  Ffreespace freespace = F_FREESPACE_NO;    // No freespace to be advertised
  Ffreespaced freespaced = F_FREESPACED_16; // Set it to 16 bit for laughs

  /*
   * Turn on/off the bits appropriate in the
   * header flags field so our flags are set correctly
   */
  _badframe = false;

  // Set the flags += overloaded sets the appropriate flag
  _flags += version;
  _flags += frametype;
  _flags += descriptor;
  _flags += stream;
  _flags += txwilling;
  _flags += rxwilling;
  _flags += udptype;
  _flags += freespace;
  _flags += freespaced;

  // Copy the beacon info to the frame payload
  _freespace = 0;
  _eid = eid;

  _paylen = sizeof(flag_t) + _eid.length();
  _payload = new char[_paylen];

  char* bptr = _payload;
  tmp_32 = htonl((uint32_t)_flags.get());
  memcpy(bptr, &tmp_32, sizeof(flag_t));
  bptr += sizeof(flag_t);

  memcpy(bptr, eid.c_str(), eid.length());
}

/*
 * Given an eid and file system info assemble a beacon from the given flags
 */
beacon::beacon(const enum f_descriptor& d, // Descriptor Size
               const enum f_stream& s,     // Are we a stream
               const enum f_txwilling& t,  // Can we transmit
               const enum f_rxwilling& r,  // Can we receive
               sardir::fsinfo* fspace,     // How much freespace is available we
                                           // work out the descriptor
               const string& eid) // EID of the peer string binary or not
{
  uint16_t tmp_16;
  uint32_t tmp_32;
  uint64_t tmp_64;
#ifdef UINT128_T
  uint128_t tmp_128;
#endif
  /*
   * These are the valid flags types applicable
   * to a beacon, set them to defaults
   */
  Fversion version = F_VERSION_1;
  Fframetype frametype = F_FRAMETYPE_BEACON; // Of course we are a BEACON
  Fdescriptor descriptor = d;
  Fstream stream = s;
  Ftxwilling txwilling = t;
  Frxwilling rxwilling = r;
  Fudptype udptype = F_UDPONLY; // We only handle UDP at the moment
  Ffreespace freespace = F_FREESPACE_YES;
  Ffreespaced freespaced = fspace->freespaced();

  /*
   * Turn on/off the bits appropriate in the
   * header flags field so our flags are set correctly
   */
  _badframe = false;

  // Set the flags
  _flags += version;
  _flags += frametype;
  _flags += descriptor;
  _flags += stream;
  _flags += txwilling;
  _flags += rxwilling;
  _flags += udptype;
  _flags += freespace;
  _flags += freespaced;

  _freespace = (uint64_t)fspace->freespace();
  _eid = eid;

  _paylen = sizeof(flag_t);
  _paylen += freespaced.length();
  _paylen += _eid.length();
  _payload = new char[_paylen];
  char* bptr = _payload;

  // Copy flags
  tmp_32 = htonl((uint32_t)_flags.get());
  memcpy(bptr, &tmp_32, sizeof(flag_t));
  bptr += sizeof(flag_t);

  switch (freespaced.get()) {
    case F_FREESPACED_16:
      tmp_16 = htons((uint16_t)_freespace);
      memcpy(bptr, &tmp_16, sizeof(uint16_t));
      bptr += sizeof(uint16_t);
      break;
    case F_FREESPACED_32:
      tmp_32 = htonl((uint32_t)_freespace);
      memcpy(bptr, &tmp_32, sizeof(uint32_t));
      bptr += sizeof(uint32_t);
      break;
    case F_FREESPACED_64:
      tmp_64 = htonll((uint64_t)_freespace);
      memcpy(bptr, &tmp_64, sizeof(uint64_t));
      bptr += sizeof(uint64_t);
      break;
    case F_FREESPACED_128:
#ifdef UINT128_T
      tmp_128 = htonlll((uint128_t)_freespace);
      memcpy(bptr, &tmp_128, sizeof(uint128_t));
      bptr += sizeof(uint128_t);
#else
      scr.error("beacon(frame): Descriptor 128 Bit Size Not Supported\n");
      _badframe = true;
#endif
  }
  memcpy(bptr, eid.c_str(), eid.length());
}

/*
 * We have an input frame contents, get it into a beacon
 */
beacon::beacon(const char* payload, const size_t& len)
{
  size_t paylen = len;

  _badframe = false;
  _paylen = paylen;
  _payload = new char[_paylen];
  memcpy(_payload, payload, _paylen);

  // Copy flags
  _flags = (flag_t)ntohl(*(uint32_t*)payload);
  payload += sizeof(flag_t);
  paylen -= sizeof(flag_t);

  Fversion version = _flags.get();
  Fframetype frametype = _flags.get();
  // Check to see we have a beacon
  if (version.get() != F_VERSION_1) {
    scr.error("Beacon: Bad Saratoga Version");
    scr.error(_flags.print());
    _badframe = true;
    return;
  }
  if (frametype.get() != F_FRAMETYPE_BEACON) {
    scr.error("Beacon: Not a BEACON frame");
    scr.error(_flags.print());
    _badframe = true;
    return;
  }

  Ffreespace freespace = _flags.get();
  Ffreespaced freespaced = _flags.get();

  // If they advertise freespace then get it
  if (freespace.get() == F_FREESPACE_YES) {
    switch (freespaced.get()) {
      case F_FREESPACED_16:
        _freespace = (offset_t)ntohs(*(uint16_t*)payload);
        payload += sizeof(uint16_t);
        paylen -= sizeof(uint16_t);
        break;
      case F_FREESPACED_32:
        _freespace = (offset_t)ntohl(*(uint32_t*)payload);
        payload += sizeof(uint32_t);
        paylen -= sizeof(uint32_t);
        break;
      case F_FREESPACED_64:
        _freespace = (offset_t)ntohll(*(uint64_t*)payload);
        payload += sizeof(uint64_t);
        paylen -= sizeof(uint64_t);
        break;
      case F_FREESPACED_128:
#ifdef UINT128_T
        _freespace = (offset_t)ntohlll(*(uint128_t*)payload);
        payload += sizeof(uint128_t);
        paylen -= sizeof(uint128_t);
#else
        scr.error("beacon(frame): Descriptor 128 Bit Size Not Supported\n");
#endif
      default:
        _badframe = true;
        return;
    }
  }
  // And now the EID
  _eid.assign(payload, paylen);
}

/*
 * Print out the beacon flags and eid
 */
string
beacon::print()
{
  char tmp[128];
  string s;
  /*
   * These are the valid flags types applicable
   * to a beacon
   */
  if (_badframe) {
    s = "beacon::print() Bad BEACON Frame";
    return (s);
  }
  Fversion version = _flags.get();
  Fframetype frametype = _flags.get();
  Fdescriptor descriptor = _flags.get();
  Fstream stream = _flags.get();
  Ftxwilling txwilling = _flags.get();
  Frxwilling rxwilling = _flags.get();
  Fudptype udptype = _flags.get();
  Ffreespace freespace = _flags.get();
  Ffreespaced freespaced = _flags.get();

  s = frametype.print();
  s += printflags("BEACON FLAGS", this->flags());
  s += "    ";
  s += version.print();
  s += "\n    ";
  s += descriptor.print();
  s += "\n    ";
  s += stream.print();
  s += "\n    ";
  s += txwilling.print();
  s += "\n    ";
  s += rxwilling.print();
  s += "\n    ";
  s += udptype.print();
  s += "\n    ";
  if (freespace.get() == F_FREESPACE_YES) {
    s += "Freespace (";
    s += freespaced.print();
    sprintf(tmp, "): %" PRIu64 " kB", (uint64_t)_freespace);
    s += tmp;
  } else
    s += freespace.print();
  s += "\n    EID: ";

  // Handle the eid it is not necessarily an ascii string so if not print
  // it out in hex
  for (size_t i = 0; i < _eid.length(); i++) {
    if (!isprint(_eid[i])) {
      s += printhex(_eid.c_str(), _eid.length());
      s += '\n';
      return (s);
    }
  }
  s += _eid;
  s += '\n';
  return (s);
}

// These are the flags we can play with in beacons
enum f_txwilling
beacon::txwilling(const enum f_txwilling& t)
{
  Ftxwilling txwilling(t);
  _flags += txwilling;
  return txwilling.get();
}

enum f_rxwilling
beacon::rxwilling(const enum f_rxwilling& r)
{
  Frxwilling rxwilling(r);
  _flags += rxwilling;
  return rxwilling.get();
}

// What am I set to ?
enum f_txwilling
beacon::txwilling()
{
  Ftxwilling txwilling = _flags.get();
  return txwilling.get();
}

enum f_rxwilling
beacon::rxwilling()
{
  Frxwilling rxwilling = _flags.get();
  return rxwilling.get();
}

}; // Namespace saratoga
