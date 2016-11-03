/*

 Copyright (c) 2012, Charles Smith
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

/*
 * Handle the Saratoga Header and Saratoga Directory Entry Sflags Fields
 * We can set them, get them, print them
 *
 * Basic principle is to have an enum that holds the values of each particular
 * flag and then a class to handle variables & functions attaining to that flag.
 *
 * There is lots of bit manipulation going on here.
 * The saratoga header uses a 32 bit unsigned integer (uint32_t) flags field.
 * The saratoga directory header uses a 16 bit unsigned integer (uint16_t) flags
 * field.
 */

/*
 **********************************************************************************************************
 */

#ifndef _SARFLAGS_H
#define _SARFLAGS_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS // So we can use STD_u64 in printf's
#endif
#include <inttypes.h>
#include <string>
using namespace std;

#include "saratoga.h"

namespace saratoga {

/*
 * Saratoga Sflag Header Field Format - 32 bit unsigned integer (flag_t)
 *
 *             111111 11112222 22222233
 *  01234567 89012345 67890123 45678901
 * +--------+--------+--------+--------+
 * |        |        |        |        |
 * +--------+--------+--------+--------+
 *
 *
 * BEACON FRAME FLAGS
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
 * |0|0|1|-> Version 1 - f_version
 * | | | |0|0|0|0|0|-> Beacon Frame - f_frametype
 * | | | | | | | | |X|X|-> Descriptor - f_descriptor
 * | | | | | | | | | | |0|-> Undefined used to be Bundles
 * | | | | | | | | | | | |X|-> Streaming - f_stream
 * | | | | | | | | | | | | |X|X| | |-> Tx Willing - f_txwilling
 * | | | | | | | | | | | | | | |X|X|-> Rx Willing - f_rxwilling
 * | | | | | | | | | | | | | | | | |X|-> UDP Lite - f_udptype
 * | | | | | | | | | | | | | | | | | |X|-> Freespace Advertise - f_freespace
 * | | | | | | | | | | | | | | | | | | |X|X|-> Freespace Descriptor -
 f_freespaced
 * | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *
 *******************************************************************

 * REQUEST FRAME FLAGS
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
 * |0|0|1|-> Version 1 - f_version
 * | | | |0|0|0|0|1|-> Request Frame - f_frametype
 * | | | | | | | | |X|X|-> Descriptor - f_descriptor
 * | | | | | | | | | | |0|-> Undefined used to be Bundles
 * | | | | | | | | | | | |X|-> Streams - f_stream
 * | | | | | | | | | | | | | | | | |X|-> UDP Lite - f_udptype
 * | | | | | | | | | | | | | | | | | | | | | | | | |X|X|X|X|X|X|X|X|->
 f_requesttype
 * | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *
 *******************************************************************

 * METADATA FRAME FLAGS
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
 * |0|0|1|-> Version 1 - f_version
 * | | | |0|0|0|1|0|-> Metadata Frame - f_frametype
 * | | | | | | | | |X|X|-> Descriptor - f_descriptor
 * | | | | | | | | | | |X|X|-> Type of Transfer - f_transfer
 * | | | | | | | | | | | | |X|-> Transfer in Progress - f_progress
 * | | | | | | | | | | | | | |X|-> Reliability - f_udptype
 * | | | | | | | | | | | | | | | | | | | | | | | | |X|X|X|X|-> Checksum Length -
 f_csumlen
 * | | | | | | | | | | | | | | | | | | | | | | | | | | | | |X|X|X|X|-> Checksum
 Type - f_csumtype
 * | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *
 *******************************************************************

 * DATA FRAME FLAGS
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
 * |0|0|1|-> Version 1 - f_version
 * | | | |0|0|0|1|1|-> Data Frame - f_frametype
 * | | | | | | | | |X|X|-> Descriptor - f_descriptor
 * | | | | | | | | | | |X|X|-> Type of Transfer - f_transfer
 * | | | | | | | | | | | | |X|-> Timestamps - f_reqtstamp
 * | | | | | | | | | | | | | | | |X|-> Request Status - f_reqstatus
 * | | | | | | | | | | | | | | | | |X|-> End of Data - f_eod
 * | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *
 *******************************************************************

 * STATUS FRAME FLAGS
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
 * |0|0|1|-> Version 1 - f_version
 * | | | |0|0|1|0|0|-> Status Frame - f_frametype
 * | | | | | | | | |X|X|-> Descriptor - f_descriptor
 * | | | | | | | | | | | | |X|-> Timestamp - f_reqtstamp
 * | | | | | | | | | | | | | |X|->Metadata Received - f_metadatarecvd
 * | | | | | | | | | | | | | | |X|-> All Holes - f_allholes
 * | | | | | | | | | | | | | | | |X|-> Holes Requested or Sent - f_reqholes
 * | | | | | | | | | | | | | | | | | | | | | | | | |X|X|X|X|X|X|X|X|-> Error
 Code - f_errcode
 * | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *
 *******************************************************************

 *
 * Saratoga Dflag Header Field Format - 16 bit unsigned integer (dflag_t)
 *
 *             1
 *  01234567 89012345
 * +--------+--------+
 * |        |        |
 * +--------+--------+
 *
 * DIRECTORY ENTRY FLAGS
 *  0                   1
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
 * | | | | | | | | | | | | | | | | |
 * |1|-> Bit 0 is always set
 * | | | | | | |X|X|-> Dirent Properties - d_properties
 * | | | | | | | | |X|X|-> Dirent Descriptor - d_descriptor
 * | | | | | | | | | | |0|-> Dirent File
 * | | | | | | | | | | | | | | | | |
 *  0                   1
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
 *
 *******************************************************************

 *
 * Saratoga Tflag Header Field Format - 8 bit unsigned integer (tflag_t)
 *
 *  01234567
 * +--------+
 * |        |
 * +--------+
 *
 * TIMESTAMP FLAGS
 *  0 1 2 3 4 5 6 7
 * | | | | | | | | |
 * | | | | | |X|X|X|-> Timestamp Type - t_timestamp
 * | | | | | | | | |
 *  0 1 2 3 4 5 6 7
 *
 *******************************************************************
 */

// We do these as templates so that they are flexible about what types
// and therefore size of the flag so this is portable for any future
// flags whether they be any size integer value

// How many bits to shift across to get the flag
template <typename T>
inline T
SHIFT(const T& bits, const T& msb)
{
  return (32 - bits - msb);
}

// How many bits long is the flag
template <typename T>
inline T
MASK(const T& bits)
{
  return ((1 << bits) - 1);
}

// Getting & Setting Flag bits
class Sflag
{
protected:
  flag_t shift; // Number of bits to shift
  flag_t set;   // The set mask for the bit field covered
public:
  inline Sflag(const flag_t s, const flag_t m)
  {
    shift = s;
    set = m << s;
  };
  inline ~Sflag()
  {
    shift = 0;
    set = 0;
  };
  inline flag_t fget(flag_t f) { return ((f & set) >> shift); };
  inline flag_t fset(flag_t c, flag_t f)
  {
    return (((c) & ~set) | ((f) << shift));
  };
};

/*
 * Saratoga Version Number - Bits 0-1
 * BEACON,REQUEST,METADATA,DATA,STATUS
 */
enum f_version
{
  F_VERSION_0 = 0x00,
  F_VERSION_1 = 0x01
};

class Fversion : private Sflag
{
protected:
  static const flag_t bits = 3;
  static const flag_t msb = 0;
  enum f_version version;

public:
  // Constructors set the version.
  // The default is Saratoga V1
  inline Fversion()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    version = F_VERSION_1;
  };
  // We want to expicitly set the Version
  inline Fversion(enum f_version v)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    version = v;
  };
  // We have a saratoga 32 bit flag set version to what it holds
  inline Fversion(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    version = (enum f_version)fget(f);
  };

  Fversion& operator=(f_version v)
  {
    version = v;
    return (*this);
  };
  Fversion& operator=(flag_t f)
  {
    version = (enum f_version)fget(f);
    return (*this);
  };

  // What is the version
  inline enum f_version get() { return (version); };
  inline enum f_version get(flag_t f) { return ((enum f_version)fget(f)); };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, version);
    return f;
  };

  // Print out the current saratoga version
  string print();
};

/*
 * Saratoga Frame Type - Bits 3-7
 *  BEACON,REQUEST,METADATA,DATA,STATUS
 */
enum f_frametype
{
  F_FRAMETYPE_BEACON = 0x00,
  F_FRAMETYPE_REQUEST = 0x01,
  F_FRAMETYPE_METADATA = 0x02,
  F_FRAMETYPE_DATA = 0x03,
  F_FRAMETYPE_STATUS = 0x04
};

class Fframetype : private Sflag
{
protected:
  static const flag_t bits = 5;
  static const flag_t msb = 3;
  enum f_frametype frametype;

public:
  // Constructor set the frametype.
  Fframetype()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    frametype = F_FRAMETYPE_BEACON;
  };
  Fframetype(enum f_frametype f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    frametype = f;
  };
  Fframetype(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    frametype = (enum f_frametype)fget(f);
  };

  Fframetype& operator=(f_frametype f)
  {
    frametype = f;
    return (*this);
  };
  Fframetype& operator=(flag_t f)
  {
    frametype = (enum f_frametype)fget(f);
    return (*this);
  };

  // What is the frametype
  enum f_frametype get() { return (frametype); };
  enum f_frametype get(flag_t f) { return ((enum f_frametype)fget(f)); };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, frametype);
    return f;
  };

  // Print out the current frametype
  string print();
};

/*
 * Saratoga Descriptor Size - Bits 8-9
 *  BEACON,REQUEST,METADATA,DATA,STATUS
 */
enum f_descriptor
{
  F_DESCRIPTOR_16 = 0x00,
  F_DESCRIPTOR_32 = 0x01,
  F_DESCRIPTOR_64 = 0x02,
  F_DESCRIPTOR_128 = 0x03
};

class Fdescriptor : private Sflag
{
protected:
  static const flag_t bits = 2;
  static const flag_t msb = 8;
  enum f_descriptor descriptor;

public:
  // Constructor set the descriptor.
  Fdescriptor()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    descriptor = F_DESCRIPTOR_16;
  };
  Fdescriptor(enum f_descriptor f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    descriptor = f;
  };
  Fdescriptor(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    descriptor = (enum f_descriptor)fget(f);
  };

  Fdescriptor& operator=(f_descriptor f)
  {
    descriptor = f;
    return (*this);
  };
  Fdescriptor& operator=(flag_t f)
  {
    descriptor = (enum f_descriptor)fget(f);
    return (*this);
  };

  // What is the descriptor
  enum f_descriptor get() { return (descriptor); };
  enum f_descriptor get(flag_t f) { return ((enum f_descriptor)fget(f)); };

  // Number of bytes in descriptor
  size_t length();

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, descriptor);
    return f;
  };

  // Print out the current descriptor
  string print();
};

/*
 * Streams support - Bit 11
 *  BEACON,REQUEST
 */
enum f_stream
{
  F_STREAMS_NO = 0x00,
  F_STREAMS_YES = 0x01
};

class Fstream : private Sflag
{
protected:
  static const flag_t bits = 1;
  static const flag_t msb = 11;
  enum f_stream stream;

public:
  // Constructor set the stream.
  Fstream()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    stream = F_STREAMS_NO;
  };
  Fstream(enum f_stream f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    stream = f;
  };
  Fstream(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    stream = (enum f_stream)(fget(f));
  };

  Fstream& operator=(f_stream f)
  {
    stream = f;
    return (*this);
  };
  Fstream& operator=(flag_t f)
  {
    stream = (enum f_stream)fget(f);
    return (*this);
  };

  // What is the stream
  enum f_stream get() { return (stream); };
  enum f_stream get(flag_t f) { return ((enum f_stream)fget(f)); };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, stream);
    return f;
  };

  // Print out the current stream
  string print();
};

/*
 * Transfer Type - Bits 10-11
 *  METADATA,DATA
 */
enum f_transfer
{
  F_TRANSFER_FILE = 0x00,
  F_TRANSFER_DIR = 0x01,
  F_TRANSFER_BUNDLE = 0x02,
  F_TRANSFER_STREAM = 0x03
};

class Ftransfer : private Sflag
{
protected:
  static const flag_t bits = 2;
  static const flag_t msb = 10;
  enum f_transfer transfer;

public:
  // Constructor set the transfer.
  Ftransfer()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    transfer = F_TRANSFER_FILE;
  };
  Ftransfer(enum f_transfer f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    transfer = f;
  };
  Ftransfer(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    transfer = (enum f_transfer)(fget(f));
  };

  Ftransfer& operator=(f_transfer f)
  {
    transfer = f;
    return (*this);
  };
  Ftransfer& operator=(flag_t f)
  {
    transfer = (enum f_transfer)fget(f);
    return (*this);
  };

  // What is the transfer
  enum f_transfer get() { return (transfer); };
  enum f_transfer get(flag_t f) { return ((enum f_transfer)fget(f)); };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, transfer);
    return f;
  };

  // Print out the current transfer
  string print();
};

/*
 * Timestamp/Nonce - Bit 12
 *  DATA,STATUS
 */
enum f_reqtstamp
{
  F_TIMESTAMP_NO = 0x00,
  F_TIMESTAMP_YES = 0x01
};

class Freqtstamp : private Sflag
{
protected:
  static const flag_t bits = 1;
  static const flag_t msb = 12;
  enum f_reqtstamp reqtstamp;

public:
  // Constructor set the tstamp.
  Freqtstamp()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    reqtstamp = F_TIMESTAMP_NO;
  };
  Freqtstamp(enum f_reqtstamp f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    reqtstamp = f;
  };
  Freqtstamp(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    reqtstamp = (enum f_reqtstamp)(fget(f));
  };

  Freqtstamp& operator=(f_reqtstamp f)
  {
    reqtstamp = f;
    return (*this);
  };
  Freqtstamp& operator=(flag_t f)
  {
    reqtstamp = (enum f_reqtstamp)fget(f);
    return (*this);
  };

  // What is the tstamp
  enum f_reqtstamp get() { return (reqtstamp); };
  enum f_reqtstamp get(flag_t f) { return ((enum f_reqtstamp)fget(f)); };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, reqtstamp);
    return f;
  };

  // Print out the current tstamp
  string print();
};

/*
 * Tranfer Progress - Bit 12
 *  METADATA
 */
enum f_progress
{
  F_PROGRESS_INPROG = 0x00,
  F_PROGRESS_TERMINATED = 0x01
};

class Fprogress : private Sflag
{
protected:
  static const flag_t bits = 1;
  static const flag_t msb = 12;
  enum f_progress progress;

public:
  // Constructor set the progress.
  Fprogress()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    progress = F_PROGRESS_TERMINATED;
  };
  Fprogress(enum f_progress f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    progress = f;
  };
  Fprogress(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    progress = (enum f_progress)(fget(f));
  };

  Fprogress& operator=(f_progress f)
  {
    progress = f;
    return (*this);
  };
  Fprogress& operator=(flag_t f)
  {
    progress = (enum f_progress)fget(f);
    return (*this);
  };

  // What is the progress
  enum f_progress get() { return (progress); };
  enum f_progress get(flag_t f) { return ((enum f_progress)fget(f)); };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, progress);
    return f;
  };

  // Print out the current progress
  string print();
};

/*
 * Transmitter Willingness - Bits 12-13
 *  BEACON,REQUEST
 */
enum f_txwilling
{
  F_TXWILLING_NO = 0x00,
  F_TXWILLING_INVALID = 0x01,
  F_TXWILLING_CAPABLE = 0x02,
  F_TXWILLING_YES = 0x03
};

class Ftxwilling : private Sflag
{
protected:
  static const flag_t bits = 2;
  static const flag_t msb = 12;
  enum f_txwilling txwilling;

public:
  // Constructor set the txwilling.
  Ftxwilling()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    txwilling = F_TXWILLING_INVALID;
  };
  Ftxwilling(enum f_txwilling f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    txwilling = f;
  };
  Ftxwilling(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    txwilling = (enum f_txwilling)(fget(f));
  };

  Ftxwilling& operator=(f_txwilling f)
  {
    txwilling = f;
    return (*this);
  };
  Ftxwilling& operator=(flag_t f)
  {
    txwilling = (enum f_txwilling)fget(f);
    return (*this);
  };

  // What is the txwilling
  enum f_txwilling get() { return (txwilling); };
  enum f_txwilling get(flag_t f) { return ((enum f_txwilling)fget(f)); };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, txwilling);
    return f;
  };

  // Print out the current txwilling
  string print();
};

/*
 * UDP & UDP-Lite Support - Bit 13
 * METADATA
 * This is defined also as bit 16 in BEACON & REQUEST
 * but the flag tpyes are the same and do the same thing just in a different
 * place
 */
enum f_udptype
{
  F_UDPONLY = 0x00,
  F_UDPLITE = 0x01
};

class Fmetadata_udptype : private Sflag
{
protected:
  static const flag_t bits = 1;
  static const flag_t msb = 13;
  enum f_udptype metadata_udptype;

public:
  // Constructor set the udptype.
  Fmetadata_udptype()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    metadata_udptype = F_UDPONLY;
  };
  Fmetadata_udptype(enum f_udptype f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    metadata_udptype = f;
  };
  Fmetadata_udptype(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    metadata_udptype = (enum f_udptype)(fget(f));
  };

  Fmetadata_udptype& operator=(f_udptype f)
  {
    metadata_udptype = f;
    return (*this);
  };
  Fmetadata_udptype& operator=(flag_t f)
  {
    metadata_udptype = (enum f_udptype)fget(f);
    return (*this);
  };

  // What is the udptype
  enum f_udptype get() { return (metadata_udptype); };
  enum f_udptype get(flag_t f) { return ((enum f_udptype)fget(f)); };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, metadata_udptype);
    return f;
  };

  // Print out the current udptype
  string print();
};

/*
 * Metadata Received - Bit 13
 *  STATUS
 */
enum f_metadatarecvd
{
  F_METADATARECVD_YES = 0x00,
  F_METADATARECVD_NO = 0x01
};

class Fmetadatarecvd : private Sflag
{
protected:
  static const flag_t bits = 1;
  static const flag_t msb = 13;
  enum f_metadatarecvd metadatarecvd;

public:
  // Constructor set the metadatarecvd.
  Fmetadatarecvd()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    metadatarecvd = F_METADATARECVD_NO;
  };
  Fmetadatarecvd(enum f_metadatarecvd f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    metadatarecvd = f;
  };
  Fmetadatarecvd(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    metadatarecvd = (enum f_metadatarecvd)(fget(f));
  };

  Fmetadatarecvd& operator=(f_metadatarecvd f)
  {
    metadatarecvd = f;
    return (*this);
  };
  Fmetadatarecvd& operator=(flag_t f)
  {
    metadatarecvd = (enum f_metadatarecvd)fget(f);
    return (*this);
  };

  // What is the metadatarecvd
  enum f_metadatarecvd get() { return (metadatarecvd); };
  enum f_metadatarecvd get(flag_t f)
  {
    return ((enum f_metadatarecvd)fget(f));
  };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, metadatarecvd);
    return f;
  };

  // Print out the current metadatarecvd
  string print();
};

/*
 * All Holes in this packet - Bit 14
 *  STATUS
 */
enum f_allholes
{
  F_ALLHOLES_YES = 0x00,
  F_ALLHOLES_NO = 0x01
};

class Fallholes : private Sflag
{
protected:
  static const flag_t bits = 1;
  static const flag_t msb = 14;
  enum f_allholes allholes;

public:
  // Constructor set the allholes.
  Fallholes()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    allholes = F_ALLHOLES_YES;
  };
  Fallholes(enum f_allholes f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    allholes = f;
  };
  Fallholes(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    allholes = (enum f_allholes)(fget(f));
  };

  Fallholes& operator=(f_allholes f)
  {
    allholes = f;
    return (*this);
  };
  Fallholes& operator=(flag_t f)
  {
    allholes = (enum f_allholes)fget(f);
    return (*this);
  };

  // What is the allholes
  enum f_allholes get() { return (allholes); };
  enum f_allholes get(flag_t f) { return ((enum f_allholes)fget(f)); };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, allholes);
    return f;
  };

  // Print out the current allholes
  string print();
};

/*
 * Request Type - Bit 24-32
 *  REQUEST
 */
enum f_requesttype
{
  F_REQUEST_NOACTION = 0x00,
  F_REQUEST_GET = 0x01,
  F_REQUEST_PUT = 0x02,
  F_REQUEST_GETDELETE = 0x03,
  F_REQUEST_PUTDELETE = 0x04,
  F_REQUEST_DELETE = 0x05,
  F_REQUEST_GETDIR = 0x06
};

class Frequesttype : private Sflag
{
protected:
  static const flag_t bits = 8;
  static const flag_t msb = 24;
  enum f_requesttype requesttype;

public:
  // Constructor set the requesttype.
  Frequesttype()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    requesttype = F_REQUEST_NOACTION;
  };
  Frequesttype(enum f_requesttype f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    requesttype = f;
  };
  Frequesttype(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    requesttype = (enum f_requesttype)(fget(f));
  };

  Frequesttype& operator=(f_requesttype f)
  {
    requesttype = f;
    return (*this);
  };
  Frequesttype& operator=(flag_t f)
  {
    requesttype = (enum f_requesttype)fget(f);
    return (*this);
  };

  // What is the requesttype
  enum f_requesttype get() { return (requesttype); };
  enum f_requesttype get(flag_t f) { return ((enum f_requesttype)fget(f)); };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, requesttype);
    return f;
  };

  // Print out the current requesttype
  string print();
};

/*
 * Receiver Willingness - Bits 14-15
 *  BEACON
 */
enum f_rxwilling
{
  F_RXWILLING_NO = 0x00,
  F_RXWILLING_INVALID = 0x01,
  F_RXWILLING_CAPABLE = 0x02,
  F_RXWILLING_YES = 0x03
};

class Frxwilling : private Sflag
{
protected:
  static const flag_t bits = 2;
  static const flag_t msb = 14;
  enum f_rxwilling rxwilling;

public:
  // Constructor set the rxwilling.
  Frxwilling()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    rxwilling = F_RXWILLING_INVALID;
  };
  Frxwilling(enum f_rxwilling f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    rxwilling = f;
  };
  Frxwilling(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    rxwilling = (enum f_rxwilling)(fget(f));
  };

  Frxwilling& operator=(f_rxwilling f)
  {
    rxwilling = f;
    return (*this);
  };
  Frxwilling& operator=(flag_t f)
  {
    rxwilling = (enum f_rxwilling)fget(f);
    return (*this);
  };

  // What is the rxwilling
  enum f_rxwilling get() { return (rxwilling); };
  enum f_rxwilling get(flag_t f) { return ((enum f_rxwilling)fget(f)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, rxwilling);
    return f;
  };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  // Print out the current rxwilling
  string print();
};

/*
 * Holes requested or sent voluntarily - Bit 15
 *  STATUS
 */
enum f_reqholes
{
  F_HOLES_REQUESTED = 0x00,
  F_HOLES_SENTVOLUNTARILY = 0x01
};

class Freqholes : private Sflag
{
protected:
  static const flag_t bits = 1;
  static const flag_t msb = 15;
  enum f_reqholes reqholes;

public:
  // Constructor set the holes.
  Freqholes()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    reqholes = F_HOLES_SENTVOLUNTARILY;
  };
  Freqholes(enum f_reqholes f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    reqholes = f;
  };
  Freqholes(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    reqholes = (enum f_reqholes)(fget(f));
  };

  Freqholes& operator=(f_reqholes f)
  {
    reqholes = f;
    return (*this);
  };
  Freqholes& operator=(flag_t f)
  {
    reqholes = (enum f_reqholes)fget(f);
    return (*this);
  };

  // What is the holes
  enum f_reqholes get() { return (reqholes); };
  enum f_reqholes get(flag_t f) { return ((enum f_reqholes)fget(f)); };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, reqholes);
    return f;
  };

  // Print out the current reqholes
  string print();
};

/*
 * File or Directory - Bit 15
 *  REQUEST
 */
enum f_fileordir
{
  F_FILEORDIR_FILE = 0x00,
  F_FILEORDIR_DIRECTORY = 0x01
};

class Ffileordir : private Sflag
{
protected:
  static const flag_t bits = 1;
  static const flag_t msb = 15;
  enum f_fileordir fileordir;

public:
  // Constructor set the fileordir.
  Ffileordir()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    fileordir = F_FILEORDIR_FILE;
  };
  Ffileordir(enum f_fileordir f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    fileordir = f;
  };
  Ffileordir(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    fileordir = (enum f_fileordir)(fget(f));
  };

  Ffileordir& operator=(f_fileordir f)
  {
    fileordir = f;
    return (*this);
  };
  Ffileordir& operator=(flag_t f)
  {
    fileordir = (enum f_fileordir)fget(f);
    return (*this);
  };

  // What is the fileordir
  enum f_fileordir get() { return (fileordir); };
  enum f_fileordir get(flag_t f) { return ((enum f_fileordir)fget(f)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, fileordir);
    return f;
  };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  // Print out the current fileordir
  string print();
};

/*
 * Request Status - Bit 15
 *  DATA
 */
enum f_reqstatus
{
  F_REQSTATUS_NO = 0x00,
  F_REQSTATUS_YES = 0x01
};

class Freqstatus : private Sflag
{
protected:
  static const flag_t bits = 1;
  static const flag_t msb = 15;
  enum f_reqstatus reqstatus;

public:
  // Constructor set the reqstatus.
  Freqstatus()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    reqstatus = F_REQSTATUS_NO;
  };
  Freqstatus(enum f_reqstatus f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    reqstatus = f;
  };
  Freqstatus(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    reqstatus = (enum f_reqstatus)(fget(f));
  };

  Freqstatus& operator=(f_reqstatus f)
  {
    reqstatus = f;
    return (*this);
  };
  Freqstatus& operator=(flag_t f)
  {
    reqstatus = (enum f_reqstatus)fget(f);
    return (*this);
  };

  // What is the reqstatus
  enum f_reqstatus get() { return (reqstatus); };
  enum f_reqstatus get(flag_t f) { return ((enum f_reqstatus)fget(f)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, reqstatus);
    return f;
  };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  // Print out the current reqstatus
  string print();
};

/*
 * UDP & UDP-Lite Support - Bit 16
 *  BEACON,REQUEST
 * Already defined previously but is same here
 *
 * enum f_udptype
 * {
 * 	F_UDPONLY = 0x00,
 * 	F_UDPLITE = 0x01
 * };
 */
class Fudptype : private Sflag
{
protected:
  static const flag_t bits = 1;
  static const flag_t msb = 16;
  enum f_udptype udptype;

public:
  // Constructor set the udptype.
  Fudptype()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    udptype = F_UDPONLY;
  };
  Fudptype(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    udptype = (enum f_udptype)(fget(f));
  };

  Fudptype& operator=(f_udptype f)
  {
    udptype = f;
    return (*this);
  };
  Fudptype& operator=(flag_t f)
  {
    udptype = (enum f_udptype)fget(f);
    return (*this);
  };

  // What is the udptype
  enum f_udptype get() { return (udptype); };
  enum f_udptype get(flag_t f) { return ((enum f_udptype)fget(f)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, udptype);
    return f;
  };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  // Print out the current udptype
  string print();
};

/*
 * Last Frame in Data Transfer, used to indicate end of file/stream - Bit 16
 *  DATA
 */
enum f_eod
{
  F_EOD_NO = 0x00,
  F_EOD_YES = 0x01
};

class Feod : private Sflag
{
protected:
  static const flag_t bits = 1;
  static const flag_t msb = 16;
  enum f_eod eod;

public:
  // Constructor set the eod.
  Feod()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    eod = F_EOD_NO;
  };
  Feod(enum f_eod f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    eod = f;
  };
  Feod(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    eod = (enum f_eod)(fget(f));
  };

  Feod& operator=(f_eod f)
  {
    eod = f;
    return (*this);
  };
  Feod& operator=(flag_t f)
  {
    eod = (enum f_eod)fget(f);
    return (*this);
  };

  // What is the eod
  enum f_eod get() { return (eod); };
  enum f_eod get(flag_t f) { return ((enum f_eod)fget(f)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, eod);
    return f;
  };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  // Print out the current eod
  string print();
};

/*
 * Advertise Available Free Space - Bit 17
 * Applicable to: BEACON
 */
enum f_freespace
{
  F_FREESPACE_NO = 0x00,
  F_FREESPACE_YES = 0x01
};

class Ffreespace : private Sflag
{
protected:
  static const flag_t bits = 1;
  static const flag_t msb = 17;
  enum f_freespace freespace;

public:
  // Constructor set the freespace.
  Ffreespace()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    freespace = F_FREESPACE_NO;
  };
  Ffreespace(enum f_freespace f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    freespace = f;
  };
  Ffreespace(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    freespace = (enum f_freespace)(fget(f));
  };

  Ffreespace& operator=(f_freespace f)
  {
    freespace = f;
    return (*this);
  };
  Ffreespace& operator=(flag_t f)
  {
    freespace = (enum f_freespace)fget(f);
    return (*this);
  };

  // What is the freespace
  enum f_freespace get() { return (freespace); };
  enum f_freespace get(flag_t f) { return ((enum f_freespace)fget(f)); };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, freespace);
    return f;
  };

  // Print out the current freespace
  string print();
};

/*
 * Advertise Descriptor Size of Free Space - Bits 18-19
 * BEACON
 */
enum f_freespaced
{
  F_FREESPACED_16 = 0x00,
  F_FREESPACED_32 = 0x01,
  F_FREESPACED_64 = 0x02,
  F_FREESPACED_128 = 0x03
};

class Ffreespaced : private Sflag
{
protected:
  static const flag_t bits = 2;
  static const flag_t msb = 18;
  enum f_freespaced freespaced;

public:
  // Constructor set the freespaced.
  Ffreespaced()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    freespaced = F_FREESPACED_16;
  };
  Ffreespaced(enum f_freespaced f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    freespaced = f;
  };
  Ffreespaced(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    freespaced = (enum f_freespaced)(fget(f));
  };

  Ffreespaced& operator=(f_freespaced f)
  {
    freespaced = f;
    return (*this);
  };
  Ffreespaced& operator=(flag_t f)
  {
    freespaced = (enum f_freespaced)fget(f);
    return (*this);
  };

  // What is the freespaced
  enum f_freespaced get() { return (freespaced); };
  enum f_freespaced get(flag_t f) { return ((enum f_freespaced)fget(f)); };

  size_t length();
  // Print out the current freespaced
  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, freespaced);
    return f;
  };

  string print();
};

/*
 * Checksum Length - Bits 24-27
 * METADATA
 */
enum f_csumlen
{
  F_CSUMLEN_NONE = 0x00,  // No Checksum
  F_CSUMLEN_CRC32 = 0x01, // 32 x 1 For CRC32 - 32 Bit
  F_CSUMLEN_MD5 = 0x04,   // 32 x 4 FOr MD5 - 128 Bit
  F_CSUMLEN_SHA1 = 0x05   // 32 x 5 For SHA-1 - 160 Bit
};

class Fcsumlen : private Sflag
{
protected:
  static const flag_t bits = 4;
  static const flag_t msb = 24;
  enum f_csumlen csumlen;

public:
  // Constructor set the csumlen.
  // Lets make the defaule none
  Fcsumlen()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    csumlen = F_CSUMLEN_NONE;
  };
  Fcsumlen(enum f_csumlen f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    csumlen = f;
  };
  Fcsumlen(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    csumlen = (enum f_csumlen)(fget(f));
  };

  Fcsumlen& operator=(f_csumlen f)
  {
    csumlen = f;
    return (*this);
  };
  Fcsumlen& operator=(flag_t f)
  {
    csumlen = (enum f_csumlen)fget(f);
    return (*this);
  };

  // What is the csum
  enum f_csumlen get() { return (csumlen); };
  enum f_csumlen get(flag_t f) { return ((enum f_csumlen)fget(f)); };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, csumlen);
    return f;
  };

  // Print out the current csumlen
  string print();
};

/*
 * Checksum - Bits 28-31
 *  METADATA
 */
enum f_csumtype
{
  F_CSUM_NONE = 0x00,
  F_CSUM_CRC32 = 0x01,
  F_CSUM_MD5 = 0x02,
  F_CSUM_SHA1 = 0x03
};

class Fcsumtype : private Sflag
{
protected:
  static const flag_t bits = 4;
  static const flag_t msb = 28;
  enum f_csumtype csumtype;

public:
  // Constructor set the csum.
  Fcsumtype()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    csumtype = F_CSUM_NONE;
  };
  Fcsumtype(enum f_csumtype f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    csumtype = f;
  };
  Fcsumtype(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    csumtype = (enum f_csumtype)(fget(f));
  };

  Fcsumtype& operator=(f_csumtype f)
  {
    csumtype = f;
    return (*this);
  };
  Fcsumtype& operator=(flag_t f)
  {
    csumtype = (enum f_csumtype)fget(f);
    return (*this);
  };

  // What is the csum
  enum f_csumtype get() { return (csumtype); };
  enum f_csumtype get(flag_t f) { return ((enum f_csumtype)fget(f)); };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, csumtype);
    return f;
  };

  // Print out the current csum
  string print();
};

/*
 * Status = Bits 24-31
 *  STATUS
 */
enum f_errcode
{
  F_ERRCODE_SUCCESS = 0x00,
  F_ERRCODE_UNSPEC = 0x01,
  F_ERRCODE_NOSEND = 0x02,
  F_ERRCODE_NORECEIVE = 0x03,
  F_ERRCODE_NOFILE = 0x04,
  F_ERRCODE_NOACCESS = 0x05,
  F_ERRCODE_NOID = 0x06,
  F_ERRCODE_NODELETE = 0x07,
  F_ERRCODE_TOOBIG = 0x08,
  F_ERRCODE_BADDESC = 0x09,
  F_ERRCODE_BADPACKET = 0x0A,
  F_ERRCODE_BADFLAG = 0x0B,
  F_ERRCODE_SHUTDOWN = 0x0C,
  F_ERRCODE_PAUSE = 0x0D,
  F_ERRCODE_RESUME = 0x0E,
  F_ERRCODE_INUSE = 0x0F,
  F_ERRCODE_NOMETADATA = 0x10
};

class Ferrcode : private Sflag
{
protected:
  static const flag_t bits = 8;
  static const flag_t msb = 24;
  enum f_errcode errcode;

public:
  // Constructor set the errcode.
  Ferrcode()
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    errcode = F_ERRCODE_UNSPEC;
  };
  Ferrcode(enum f_errcode f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    errcode = f;
  };
  Ferrcode(flag_t f)
    : Sflag(SHIFT(bits, msb), MASK(bits))
  {
    errcode = (enum f_errcode)(fget(f));
  };

  Ferrcode& operator=(f_errcode f)
  {
    errcode = f;
    return (*this);
  };
  Ferrcode& operator=(flag_t f)
  {
    errcode = (enum f_errcode)fget(f);
    return (*this);
  };

  // What is the errcode
  enum f_errcode get() { return (errcode); };
  enum f_errcode get(flag_t f) { return ((enum f_errcode)fget(f)); };

  flag_t shift() { return (SHIFT(bits, msb)); };
  flag_t mask() { return (MASK(bits)); };

  flag_t setflag(flag_t f)
  {
    f = fset(f, errcode);
    return f;
  };

  // Print out the current errcode
  string print();
};

// The Saratoga 32 bit flag
class Fflag
{
private:
  flag_t flag;

public:
  inline Fflag() { flag = 0; };
  inline Fflag(flag_t f) { flag = f; };
  inline ~Fflag() { flag = 0; };

  flag_t get() { return flag; };

  // Copy flags or set to discreet value
  Fflag operator=(flag_t f)
  {
    flag = f;
    return (*this);
  };
  Fflag operator=(Fflag f)
  {
    flag = f.get();
    return (*this);
  };

  // Set the bits in a flag to the value f
  // Nice shorthand way to set them
  // Lots of arguments if this should of been |= but I choose +=
  // as it "adds" the flags value to the total flags
  Fflag operator+=(Fversion f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Fframetype f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Fdescriptor f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Fstream f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Ftransfer f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Freqtstamp f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Fprogress f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Ftxwilling f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Fmetadata_udptype f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Fmetadatarecvd f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Fallholes f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Frequesttype f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Frxwilling f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Freqholes f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Ffileordir f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Freqstatus f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Fudptype f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Feod f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Ffreespace f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Ffreespaced f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Fcsumlen f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Fcsumtype f)
  {
    flag = f.setflag(flag);
    return (*this);
  };
  Fflag operator+=(Ferrcode f)
  {
    flag = f.setflag(flag);
    return (*this);
  };

  string print();
};

}; // Namespace saratoga

#endif // _SARFLAGS_H
