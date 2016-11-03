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

#include "saratoga.h"
#include "sarflags.h"
#include "screen.h"
#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <sys/timeb.h>

#ifndef _TIMESTAMP_H
#define _TIMESTAMP_H

const uint32_t Y2KCONST = 946684800; // Seconds from Unix Epoch to Y2K epoch

using namespace std;

namespace saratoga {

/*
 * Timestamp Entry Tflag Field Format - 8 bit unsigned char (unsigned char)
 *
 *  01234567
 * +--------+
 * |     TTT|
 * +--------+
 *
 * TTT == Timestamp Type (Bits 5-7)
 */
enum t_timestamp
{
  T_TSTAMP_UDEF = 0x00,
  T_TSTAMP_32 = 0x01,
  T_TSTAMP_64 = 0x02,
  T_TSTAMP_32_32 = 0x03,
  T_TSTAMP_64_32 = 0x04,
  T_TSTAMP_32_Y2K = 0x05
};

class Tflag
{
protected:
  tflag_t shift; // Number of bits to shift
  tflag_t set;   // The set mask for the bit field covered
public:
  inline Tflag(const tflag_t s, const tflag_t m)
  {
    shift = s;
    set = m << s;
  };
  inline ~Tflag()
  {
    shift = 0;
    set = 0;
  };
  inline tflag_t fget(tflag_t f) { return ((f & set) >> shift); };
  inline tflag_t fset(tflag_t c, tflag_t f)
  {
    return (((c) & ~set) | ((f) << shift));
  };
};

class Ttimestamp : private Tflag
{
protected:
  static const flag_t bits = 3;
  static const flag_t msb = 5;
  static const flag_t tshift = (8 - bits - msb);
  static const tflag_t tmask = 0x07; // Last 3 bits hold timestamp type
  enum t_timestamp ttype;

public:
  // Constructor set the timestamp
  Ttimestamp(enum t_timestamp t)
    : Tflag(tshift, tmask)
  {
    ttype = t;
  };

  // Copy constructor
  Ttimestamp(const Ttimestamp& old)
    : Tflag(tshift, tmask)
  {
    ttype = old.ttype;
  };

  // Assignment constructor
  Ttimestamp& operator=(const Ttimestamp& old)
  {
    ttype = old.ttype;
    return (*this);
  };

  // Given Current Tflag set the bits for this timestamp
  enum t_timestamp set(enum t_timestamp t)
  {
    ttype = t;
    return (ttype);
  };

  // What is the timestamp
  enum t_timestamp get() { return (ttype); };

  string print();
};

// Use Local or UTC timestamps in ascii output
enum timezone
{
  TZ_UTC = 0,
  TZ_LOCAL = 1
};

class timestamp
{
private:
  // Directory Entry records are in Y2K bundle time standard
  // this is the coversion to/from Posix time
  static const time_t y2k = 946684800;
  enum t_timestamp _ttype;
  std::chrono::system_clock::time_point _timestamp;

public:
  /*
   * We can assign,add,increment,compare,subtract,decrement timestamps
   * to/from eachother or int64_t number of seconds to/from a timestamp
   */

  timestamp& operator=(const timestamp& t1)
  {
    _ttype = t1._ttype;
    _timestamp = t1._timestamp;
    return (*this);
  };

  friend const timestamp operator+(const timestamp& t1, const timestamp& t2);
  friend const timestamp operator+(const timestamp& t1, const int64_t);

  friend const timestamp operator-(const timestamp& t1, const timestamp& t2);
  friend const timestamp operator-(const timestamp& t1, const int64_t);

  // Constructor set the timestamp secs and nanosecs
  timestamp(enum t_timestamp t, int64_t s, int64_t n)
  {
    std::chrono::system_clock::time_point epoch;
    Ttimestamp tstamp(t);
    _ttype = tstamp.set(t);
    _timestamp = epoch + std::chrono::seconds((time_t)s) +
                 std::chrono::nanoseconds((time_t)n);
  };

  // Copy constructor
  timestamp(const timestamp& t)
  {
    _ttype = t._ttype;
    _timestamp = t._timestamp;
  };

  // Constructor set the timestamp given a time_t
  timestamp(enum t_timestamp, time_t);

  // Constructor Create a timestamp with current Zulu time
  timestamp()
  {
    _ttype = T_TSTAMP_32_32;
    _timestamp = std::chrono::system_clock::now();
  };

  // Constructor set the timestamp to the current Zulu time and type
  timestamp(enum t_timestamp t)
  {
    _ttype = t;
    _timestamp = std::chrono::system_clock::now();
  }

  // Constructor given a timestamp buffer decode & set it
  timestamp(char*);

  // Destrucotor
  ~timestamp()
  {
    std::chrono::system_clock::time_point epoch;
    _ttype = T_TSTAMP_UDEF;
    _timestamp = epoch;
  };

  void clear()
  {
    std::chrono::system_clock::time_point epoch;
    _ttype = T_TSTAMP_UDEF;
    _timestamp = epoch;
  };

  // Reset the timestamp to the current Zulu time
  void setzulu()
  {
    _ttype = T_TSTAMP_32_32;
    _timestamp = std::chrono::system_clock::now();
  };

  // A timestamp is ALWAYS 16 octets long
  const size_t length() { return 16; };

  // Convert to posix epoch if required
  time_t y2ktime()
  {
    if (this->_ttype != T_TSTAMP_32_Y2K)
      return (0);
    return (timestamp::y2k);
  };

  // Bump me by a second prefix notation ie ++x
  timestamp& operator++()
  {
    _timestamp += std::chrono::seconds(1);
    return (*this);
  }

  // Bump me by a second postfix notation ie x++
  const timestamp operator++(int)
  {
    timestamp tmp(*this);
    ++(*this);
    return (tmp);
  }

  // Bump me by n seconds
  timestamp& operator+=(int64_t n)
  {
    _timestamp += std::chrono::seconds(n);
    return (*this);
  }
  /*
          // Add a timestamp to current
          timestamp&	operator+=(timestamp t) {
                  timestamp	tmp(*this);
                  *this = tmp + t;
                  return(*this);
          }
   */

  // Unbump me by a second
  timestamp& operator--()
  {
    _timestamp -= std::chrono::seconds(1);
    return (*this);
  }

  // Unbump me by a second postfix notation ie x--
  const timestamp operator--(int)
  {
    timestamp tmp(*this);
    --(tmp);
    return (tmp);
  }

  // Unbump me by n seconds
  timestamp& operator-=(int64_t n)
  {
    _timestamp -= std::chrono::seconds(n);
    return (*this);
  }

  // Subtract a timestamp from current
  timestamp& operator-=(timestamp t)
  {
    timestamp tmp(*this);
    *this = tmp - t;
    return (*this);
  }

  // Equality etc operators comparing timestamps
  bool operator==(timestamp t2) { return (this->_timestamp == t2._timestamp); }

  bool operator!=(timestamp t2) { return (this->_timestamp != t2._timestamp); }

  bool operator<=(timestamp t2) { return (this->_timestamp <= t2._timestamp); }

  bool operator>=(timestamp t2) { return (this->_timestamp >= t2._timestamp); }

  bool operator>(timestamp t2) { return (this->_timestamp > t2._timestamp); }

  bool operator<(timestamp t2) { return (this->_timestamp < t2._timestamp); }

  // Given Current Tflag set the bits for this timestamp
  t_timestamp settype(enum t_timestamp t)
  {
    Ttimestamp tstamp(t);
    _ttype = tstamp.set(t);
    return (_ttype);
  };

  // What is the timestamp type
  enum t_timestamp ttype() { return (_ttype); };

  // What is the timestamp secs
  offset_t secs()
  {
    return ((offset_t)std::chrono::system_clock::to_time_t(_timestamp));
  };

  // What is the timestamp nanosecs
  // offset_t	nsecs() { return(_nsecs); };

  // Return the time in directory entry format which
  // is number of seconds since the Y2K epoch
  uint32_t dirtime()
  {
    return (uint32_t)std::chrono::system_clock::to_time_t(_timestamp);
  };

  offset_t dirtimelength() { return sizeof(uint32_t); };

  // Ascii time string of a timestamp
  string asctime();

  // Convert timestamp to output buffer to send to peer
  char* hton();
  char* ntoh(char*);

  // Print out the current timestamp
  string print();
  string printlong();
  string printshort(); // Short HH:MM:SS
  string typeprint();  // What timestamp type are we
};

}; // namespace saratoga

#endif // _TIMESTAMP_H
