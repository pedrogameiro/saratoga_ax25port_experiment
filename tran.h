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

#ifndef _TRAN_H
#define _TRAN_H

#include <cstring>
#include <iostream>
#include <string>
using namespace std;

#include "data.h"
#include "fileio.h"
#include "frame.h"
#include "holes.h"
#include "ip.h"
#include "metadata.h"
#include "metadata.h"
#include "request.h"
#include "saratoga.h"
#include "sarflags.h"
#include "screen.h"
#include "status.h"
#include "sysinfo.h"
#include "timer.h"
#include "timestamp.h"

using namespace timer_group;

namespace saratoga {

/*
 * Are we receiving from a socket
 * or sending to a socket
 */
enum direction
{
  FROM_SOCKET = 0,
  TO_SOCKET = 1,
};

/*
 * Are we requesting a transfer or have we been requested to transfer
 * i.e. Did we create the REQUEST or did we receive the REQUEST
 */
enum requestor
{
  INBOUND = 0,
  OUTBOUND = 1
};

/*
 **********************************************************************
 * TRAN
 **********************************************************************
 */

/*
 * Data pertaining to each saratoga transfer
 */
class tran
{
private:
  bool _ready;          // Is this tran good or bad
  requestor _requestor; // Are we the requestor or have we been requested
  bool _rxstatus;       // Have we received a STATUS for the transfer
  bool _txstatus;       // Have we sent a STATUS for the transfer
  bool _metadatasent;   // Have we sent a METADATA for the transfer

  direction _dir; // To or from a socket

  // Timeouts
  timer_group::timer _transfertimer; // Transfer timer
  timer_group::timer _requesttimer;  // Request timer
  timer_group::timer _statustimer;   // Status timer

  session_t _session;      // The session ID
  offset_t _curprogress;   // Current transfer progress
  offset_t _inresponseto;  // What positon was this asked for in transfer
  holes _holes;            // Current list of holes
  holes _completed;        // Current list of completed / written buffers
  offset_t _offset;        // File offset for read or write
  sarnet::udp* _peer;      // Socket I am talking to
  sarfile::fileio* _local; // Local file I am reading or writing to
  timestamp _timestamp;    // Timestamp if we have one
  timestamp _lastrxtstamp; // Timestamp last received
  uint64_t _diskfree;      // Current free disk space in kilobytes
  bool _done;              // Is this transfer finished ?

  // THESE ARE ALL OF THE FLAGS APPLICABLE TO A TRANSACTION
  // They may and most do change during the life of a transaction
  Fversion _version;                   // Saratoga Version 1 default
  Fdescriptor _descriptor;             // Descriptor size
  Frequesttype _requesttype;           // What type of request
  Ftransfer _transfer;                 // What are we transferring
  Freqtstamp _reqtstamp;               // Do we transfer time stamps
  Fprogress _progress;                 // Are we in progress or terminated
  Ftxwilling _txwilling;               // Can we transmit at the moment
  Frxwilling _rxwilling;               // Can we receive at the moment
  Fstream _stream;                     // Can we stream
  Fudptype _udptype;                   // Are we udp or udplite
  Fmetadata_udptype _metadata_udptype; // Are we udp or udplite
  Fmetadatarecvd _metadatarecvd;       // Has Metadata been received
  Ffileordir _fileordir;               // Are we a file or directory
  Freqholes _reqholes;                 // Are holes requested or
  Fallholes _allholes;                 // Have we received all holes
  Freqstatus _reqstatus;               // SHould we request a staus
  Feod _eod;                           // End of data received
  Ffreespace _freespace;               // Do we advertise freespace
  Ffreespaced _freespaced;             // What is the freespace descriptor
  Fcsumtype _csumtype;                 // Checksums
  Fcsumlen _csumlen;                   // Checksum type
  Ferrcode _errcode;                   // Current error code
  enum t_timestamp _timetype;          // What is the timestamp format
public:
  inline direction dir() { return (_dir); };
  inline requestor req() { return (_requestor); };
  inline sarnet::udp* peer() { return (_peer); };
  inline sarfile::fileio* local() { return (_local); };
  inline string localfname() { return _local->fname(); };
  inline offset_t localflen() { return _local->flen(); };

  inline session_t session() { return (_session); };
  inline bool done() { return (_done); };
  inline offset_t offset() { return (_offset); };
  inline void offset(offset_t o) { _offset = o; };
  inline offset_t curprogress() { return _curprogress; };
  inline void curprogress(offset_t p) { _curprogress = p; };
  inline offset_t inresponseto() { return _inresponseto; };
  inline void inresponseto(offset_t i) { _inresponseto = i; };
  inline holes* holelist() { return &_holes; };
  inline holes* completedlist() { return &_completed; };

  // What values are set for the transfer flags
  inline enum f_version version() { return _version.get(); };
  inline enum f_descriptor descriptor() { return _descriptor.get(); };
  inline enum f_transfer transfer() { return _transfer.get(); };
  inline enum f_reqtstamp reqtstamp() { return _reqtstamp.get(); };
  inline enum f_progress progress() { return _progress.get(); };
  inline enum f_txwilling txwilling() { return _txwilling.get(); };
  inline enum f_rxwilling rxwilling() { return _rxwilling.get(); };
  inline enum f_stream stream() { return _stream.get(); };
  inline enum f_udptype udptype() { return _udptype.get(); };
  inline enum f_udptype metadata_udptype() { return _metadata_udptype.get(); };
  inline enum f_metadatarecvd metadatarecvd() { return _metadatarecvd.get(); };
  inline enum f_requesttype requesttype() { return _requesttype.get(); };
  inline enum f_fileordir fileordir() { return _fileordir.get(); };
  inline enum f_allholes allholes() { return _allholes.get(); };
  inline enum f_reqholes reqholes() { return _reqholes.get(); };
  inline enum f_reqstatus reqstatus() { return _reqstatus.get(); };
  inline enum f_eod eod() { return _eod.get(); };
  inline enum f_freespace freespace() { return _freespace.get(); };
  inline enum f_freespaced freespaced() { return _freespaced.get(); };
  inline enum f_csumtype csumtype() { return _csumtype.get(); };
  inline enum f_csumlen csumlen() { return _csumlen.get(); };
  inline enum f_errcode errcode() { return _errcode.get(); };

  // Reset the value of the transfer flags and return them
  inline void version(enum f_version x) { _version = x; };
  inline void descriptor(enum f_descriptor x) { _descriptor = x; };
  inline void transfer(enum f_transfer x) { _transfer = x; };
  inline void reqtstamp(enum f_reqtstamp x) { _reqtstamp = x; };
  inline void progress(enum f_progress x) { _progress = x; };
  inline void txwilling(enum f_txwilling x) { _txwilling = x; };
  inline void rxwilling(enum f_rxwilling x) { _rxwilling = x; };
  inline void stream(enum f_stream x) { _stream = x; };
  inline void udptype(enum f_udptype x) { _udptype = x; };
  inline void metadata_udptype(enum f_udptype x) { _metadata_udptype = x; };
  inline void metadatarecvd(enum f_metadatarecvd x) { _metadatarecvd = x; };
  inline void requesttype(enum f_requesttype x) { _requesttype = x; };
  inline void fileordir(enum f_fileordir x) { _fileordir = x; };
  inline void reqholes(enum f_reqholes x) { _reqholes = x; };
  inline void allholes(enum f_allholes x) { _allholes = x; };
  inline void reqstatus(enum f_reqstatus x) { _reqstatus = x; };
  inline void eod(enum f_eod x) { _eod = x; };
  inline void freespace(enum f_freespace x) { _freespace = x; };
  inline void freespaced(enum f_freespaced x) { _freespaced = x; };
  inline void csumtype(enum f_csumtype x) { _csumtype = x; };
  inline void csumlen(enum f_csumlen x) { _csumlen = x; };
  inline void errcode(enum f_errcode x) { _errcode = x; };

  // Has our timer elapsed
  inline bool transfer_expired() { return _transfertimer.elapsed(); };
  inline bool request_expired() { return _requesttimer.elapsed(); };
  inline bool status_expired() { return _statustimer.elapsed(); };

  // Reset the timer
  inline void transfer_reset() { _transfertimer.reset(); };
  inline void request_reset() { _requesttimer.reset(); };
  inline void status_reset() { _statustimer.reset(); };

  // Hole handlers
  inline void holes_clear() { _holes.clear(); };
  inline void holes_add(holes* h) { _holes += *h; };

  // Create a transfer from a received request
  tran(requestor, direction, saratoga::request*, sarnet::udp*, string);

  // Copy constructor
  tran(const tran&);

  ~tran()
  {
    extern sarwin::screen scr;
    scr.msg("Removing Transfer %" PRIu32 "", _session);
  };

  tran& operator=(const tran&);

  // We are equal if our session # and peer ip are the same
  inline bool operator==(const tran& t)
  {
    if (_session == t._session && *_peer == *(t._peer))
      return true;
    return false;
  }

  inline bool ready() { return (_ready); };
  inline bool ready(bool x)
  {
    _ready = x;
    return (x);
  };

  inline bool rxstatus() { return (_rxstatus); };
  inline bool rxstatus(bool x)
  {
    _rxstatus = x;
    return (x);
  };
  void zap();

  bool senddata(const char*, const ssize_t&);
  void senddata(std::list<saratoga::buffer>*);
  bool sendstatus();
  bool sendmetadata();

  // Apply all of the information contained in the received
  // frame to the transfer instance
  void applystatus(saratoga::status*);
  void applymetadata(saratoga::metadata*);
  void applydata(saratoga::data*);

  string print();
  string holes_print() { return _holes.print(); };
};

/*
 * List of multiple transfers
 */
class transfers
{
private:
  const size_t _max = 100; // Maximum # of transactions
  std::list<saratoga::tran> _transfers;

public:
  transfers(){};
  ~transfers() { _transfers.clear(); };

  // We have a REQUEST
  // create the transfer and add to list if not already there
  // Return pointer to it or NULL if bad
  saratoga::tran* add(saratoga::requestor, saratoga::direction,
                      saratoga::request*, sarnet::udp*, string);

  // Remove a transfer from the list
  void remove(tran* t) { _transfers.remove(*t); };

  void zap() { _transfers.clear(); };

  std::list<saratoga::tran>::iterator begin() { return (_transfers.begin()); };
  std::list<saratoga::tran>::iterator end() { return (_transfers.end()); };

  // Return ptr to matching socket transfer or NULL
  saratoga::tran* match(session_t, sarnet::udp*, enum direction);

  // Return ptr to matching transfer with local fd or NULL
  saratoga::tran* match(sarfile::fileio*);

  // Handle inbound traffic for a transfer and return a pointer
  // to the applicable transfer or NULL if it doesn't exist
  saratoga::tran* rxrequest(saratoga::request*, sarnet::udp*);
  saratoga::tran* rxmetadata(saratoga::metadata*, sarnet::udp*);
  saratoga::tran* rxdata(saratoga::data*, sarnet::udp*);
  saratoga::tran* rxstatus(saratoga::status*, sarnet::udp*);

  string print();
};

}; // Namespace saratoga

#endif // _TRAN_H
