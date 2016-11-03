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

#ifndef _SARATOGA_H
#define _SARATOGA_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS // So we can d PRI_u64 in printf
#endif

#include <arpa/inet.h>
#include <inttypes.h>
#include <list>
#include <string.h>
#include <string>

/*
 * At this stage we do not support 128 bit descriptors but the code is
 * all there. Just define this and we are (well should be) good to go
 */
#undef UINT128_T

#ifdef UINT128_T
extern uint128_t htonlll(uint128_t);
extern uint128_t ntohlll(uint128_t);
#endif // UINT128_T

extern uint64_t htonll(uint64_t);
extern uint64_t ntohll(uint64_t);

extern int debuglevel();

// using namespace std;

namespace saratoga {

typedef uint32_t flag_t;       // Saratoga Flags
typedef uint16_t dflag_t;      // Directory Flags
typedef uint32_t dtime_t;      // Directory Time Fields
typedef unsigned char tflag_t; // Timestamp Flags
typedef uint32_t session_t;    // Session ID
typedef off64_t offset_t; // Memory & Offset maths I need it to be the biggest
                          // unsigned int type

// enum boolean	{ TRUE = 1, FALSE = 0 }; // Good old bools...

// hton and ntoh conversions
typedef uint16_t hton_t;
typedef uint16_t ntoh_t;
typedef uint32_t htonl_t;
typedef uint32_t ntohl_t;
typedef uint64_t htonll_t;
typedef uint64_t ntohll_t;
#ifdef UINT128_T
typedef uint128_t htonlll_t; // htonlll conversions - maybe one day :)
typedef uint128_t ntohlll_t; // ntohlll conversions
#endif                       // UINT128_T

typedef unsigned int uint_t; // Used in sprintf's

// Buffers used for socket and file i/o
class buffer
{
  char* _b;
  size_t _len;
  offset_t _offset; // The offset into hte file for this buffer
                    // This is always 0 for streams and sockets of course
  static const size_t _maxbuff = 10000; // maximum size of a buffer
                                        // This has to be at least > jumbo
                                        // frame size
public:
  // Buffer with no offset to seek used for streams
  // and sockets and sequential reads/writes (offset always 0)
  buffer(const char* b, size_t len)
  {
    _len = len;
    if (len > 0) {
      _b = new char[len];
      memcpy(_b, b, len);

    } else
      _b = nullptr;
    _offset = 0;
  }

  // Buffer with an offset into the file
  // so we know where to seek & read/write it
  buffer(const char* b, size_t len, offset_t o)
  {
    _len = len;
    if (len > 0) {
      _b = new char[len];
      memcpy(_b, b, len);
    } else
      _b = nullptr;
    _offset = o;
  }

  buffer(buffer* b)
  {
    _len = b->len();
    if (b->_len > 0) {
      _b = new char[_len];
      memcpy(_b, b->buf(), _len);
    } else
      _b = nullptr;
    _offset = b->offset();
  }

  ~buffer() { this->clear(); };

  void clear()
  {
    if (_len > 0)
      delete _b;
    _b = nullptr;
    _offset = 0;
    _len = 0;
  }

  // Copy constructor
  buffer(const buffer& old)
  {
    if (old._len > 0) {
      _b = new char[old._len];
      memcpy(_b, old._b, old._len);
    } else
      _b = nullptr;
    _len = old._len;
    _offset = old._offset;
  }

  const buffer& operator=(const buffer& old)
  {
    if (old._len > 0) {
      _b = new char[old._len];
      memcpy(_b, old._b, old._len);
    } else
      _b = nullptr;
    _len = old._len;
    _offset = old._offset;
    return (*this);
  }

  // Equality
  bool operator==(const buffer& rhs)
  {
    if (_len != rhs._len || _offset != rhs._offset)
      return (false);
    for (size_t i = 0; i < _len; i++)
      if (_b[i] != rhs._b[i])
        return (false);
    return true;
  };

  // Inequality
  bool operator!=(const buffer& rhs)
  {
    if (_len != rhs._len || _offset != rhs._offset)
      return true;
    for (size_t i = 0; i < _len; i++)
      if (_b[i] != rhs._b[i])
        return true;
    return false;
  };

  // Add a buffer to the end of the existing buffer
  const buffer& operator+=(const buffer& b1)
  {
    buffer tmp(_b, _len, _offset);
    // Remove existing
    if (_len != 0)
      delete _b;
    // Whats the new length
    _len = tmp._len + b1._len;
    if (_len > 0) {
      _b = new char[_len];
      memcpy(&(this->_b[0]), &tmp._b[0], tmp._len);
      memcpy(&(this->_b[tmp._len]), &b1._b[0], b1._len);
    } else
      _b = nullptr;
    _offset = tmp._offset;
    return (*this);
  };

  char* buf() { return (_b); };
  size_t len() { return (_len); };
  offset_t offset() { return _offset; };
  size_t maxbuff() { return (_maxbuff); };

  // Mainly here for dubug purposes of printable ascii text use at
  // your own peril!!!!
  std::string print()
  {
    std::string s;
    s.append(_b, _len);
    return s;
  };
};

// Link Lists of buffer handling
// Keep it simple with few operators
class buffers
{
private:
  std::list<buffer> _bufs;

public:
  buffers() { _bufs.empty(); }

  ~buffers() { _bufs.clear(); }

  // Add a char* string to the list
  // Used when we have read in something from a file
  // or to socket
  void add(const char* b, size_t l)
  {
    buffer* newb = new buffer(b, l);
    _bufs.push_back(*newb);
  }

  // Seek to position in file offset for a read/write
  void add(const char* b, size_t l, offset_t o)
  {
    buffer* newb = new buffer(b, l, o);
    _bufs.push_back(*newb);
  }

  // We have an existing buffer add it
  // VERY IMPORTANT as it allocated the necessary memory
  void add(const buffer& b)
  {
    buffer* newb = new buffer(b);
    _bufs.push_back(*newb);
  }

  // Pop the buffer off of the front of the list
  void pop()
  {
    if (!_bufs.empty())
      _bufs.pop_front();
  }

  // Use at your own peril!!!
  std::string print()
  {
    std::string s = "";
    for (std::list<buffer>::iterator i = _bufs.begin(); i != _bufs.end(); i++)
      s += i->print();
    return s;
  }
};

} // namespace saratoga

#endif // _SARATOGA_H
