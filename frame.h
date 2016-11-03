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

#ifndef _FRAME_H
#define _FRAME_H

#include <cstring>
#include <iostream>
#include <string>
using namespace std;

#include "ip.h"
#include "saratoga.h"
#include "sarflags.h"
#include "screen.h"
#include "timestamp.h"

// Max Data Frame Size - Worst case
// Framesize - IPv6Head - UDPHead - SarFlags - DataId - Offset - Timestamp
// 	8192      40        8            4       4        8         16
// 	1500      40        8            4       4        8         16

#ifdef JUMBO
const int MAXDATA = 8192 - 40 - 8 - 4 - 4 - 8 - 16;
#else
const int MAXDATA = 1500 - 40 - 8 - 4 - 4 - 8 - 16;
#endif

// Minimum Directory Entry Payload is DirFlags + Mtime + Ctime + Null Byte
const int MINDIRENT = 4 + 4 + 4 + 1;
// Minimum Beacon Payload is just the SarFlags
const int MINBEACON = 4;
// Minimum Request Payload is SarFlags + Session + Null Byte
const int MINREQUEST = 4 + 4 + 1;
// Minimum Metadata Payload is SarFlags + Session  + MinDirent
const int MINMETADATA = 4 + 4 + MINDIRENT;
// Minimum Data Payload is Sarflags + Session + MinOffset (we can have 0 data)
const int MINDATA = 4 + 4 + 2;
// Minimum Status is SarFlags + Session + Progress + InResponseto
const int MINSTATUS = 4 + 4 + 2 + 2;

namespace saratoga {

/*
 * A received or sent saratoga frame has flags and a payload
 * This is the master class for all of the frame types
 * BEACON, DATA, STATUS, REQUEST, METADATA
 */
class frame
{
public:
  virtual ~frame(){}; // Its all deleted in the sub-class

  /* What is the length of the payload in this frame */
  virtual size_t paylen() const { return this->paylen(); };

  /* Send me a pointer to the payload */
  virtual char* payload() const { return this->payload(); };

  /* What type of frame is this */
  virtual f_frametype type() { return this->type(); };

  // These functions are virtual and ALL code must be here
  // in the class declaration

  virtual bool badframe() { return this->badframe(); };

  /* This frames saratoga flags */
  virtual flag_t flags() { return this->flags(); };

  /* Send a frame */
  virtual ssize_t tx(sarnet::udp* sock) = 0;

  /* Receive a frame */
  virtual ssize_t rx() = 0;

  /* Print the details of the frame */
  virtual string print() { return this->print(); };
};

} // Namesapce saratoga

#endif // _FRAME_H
