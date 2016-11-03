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

#include "globals.h"
#include "screen.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <inttypes.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <unistd.h>

namespace checksums {

// Remove white space, delimeter cahracters and convert to upper case
// e.g. ab:cd:ef -> ABCDEF
string
strip(string str)
{
  str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
  str.erase(std::remove(str.begin(), str.end(), '\t'), str.end());
  str.erase(std::remove(str.begin(), str.end(), ':'), str.end());
  str.erase(std::remove(str.begin(), str.end(), '-'), str.end());
  str.erase(std::remove(str.begin(), str.end(), '.'), str.end());
  for (size_t i = 0; i < str.length(); i++) {
    str[i] = toupper(str[i]);
  }
  return str;
}

// ************************************************************************************
// Checksum
// ************************************************************************************

// Calculate a checksum for a file/directory entry
checksum::checksum(const enum csum_type f, const string& fname)
{
  _csumtype = f;
  switch (_csumtype) {
    case CSUM_NONE:
      _none = checksums::none();
      _crc32.clear();
      _md5.clear();
      _sha1.clear();
      return;
    case CSUM_CRC32:
      _crc32 = checksums::crc32(fname);
      _none.clear();
      _md5.clear();
      _sha1.clear();
      return;
    case CSUM_MD5:
      _md5 = checksums::md5(fname);
      _none.clear();
      _crc32.clear();
      _sha1.clear();
      return;
    case CSUM_SHA1:
      _sha1 = checksums::sha1(fname);
      _none.clear();
      _crc32.clear();
      _md5.clear();
      return;
    default:
      scr.error("checksum:: Bad Checksum type, setting to none");
      _csumtype = CSUM_NONE;
      _none = checksums::none();
      _crc32.clear();
      _md5.clear();
      _sha1.clear();
      return;
  }
}

checksum::checksum(const enum csum_type f, uint32_t* cp)
{
  _csumtype = f;
  switch (_csumtype) {
    case CSUM_NONE:
      _none = checksums::none();
      _crc32.clear();
      _md5.clear();
      _sha1.clear();
      return;
    case CSUM_CRC32:
      _crc32 = checksums::crc32(cp);
      _none.clear();
      _md5.clear();
      _sha1.clear();
      return;
    case CSUM_MD5:
      _md5 = checksums::md5(cp);
      _none.clear();
      _sha1.clear();
      _crc32.clear();
      return;
    case CSUM_SHA1:
      _sha1 = checksums::sha1(cp);
      _none.clear();
      _md5.clear();
      _crc32.clear();
      return;
    default:
      scr.error("checksum:: Bad Checksum type, setting to none");
      _none = checksums::none();
      _crc32.clear();
      _md5.clear();
      _sha1.clear();
      return;
  }
}

string
checksum::print()
{
  switch (_csumtype) {
    case CSUM_NONE:
      return (_none.print());
    case CSUM_CRC32:
      return (_crc32.print());
    case CSUM_MD5:
      return (_md5.print());
    case CSUM_SHA1:
      return (_sha1.print());
    default:
      return (_none.print());
  }
}

// ************************************************************************************
// No Checksum
// ************************************************************************************

string
none::print()
{
  string s = "No CSUM";
  return (s);
}

// ************************************************************************************
// CRC32 Checksum
// ************************************************************************************

// Return the string value of a crc32 checksum
string
crc32::strval()
{
  char tmp[128];

  string s = "";
  sprintf(tmp, "%08" PRIX32 "", (uint32_t)_crc32);
  s += tmp;
  scr.debug(4, "crc32::strval() %s", s.c_str());
  return (s);
}

string
crc32::print()
{
  char tmp[128];

  string s = "CRC32";
  sprintf(tmp, "%08" PRIX32 "", (uint32_t)_crc32);
  s += tmp;
  scr.debug(4, "crc32::print() %s", s.c_str());
  return (s);
}

crc32::crc32(string fname)
{
  const size_t buflen = 1024;
  char buf[buflen];
  unsigned long csum = 0;

  ifstream fs(fname.c_str(), ios::binary);
  if (!fs.is_open()) {
    scr.error("crc32: Can't open file: %s", fname.c_str());
    _crc32 = 0;
    return;
  }
  /* accumulate crc32 from file */
  while (fs.read(buf, buflen)) {
    if (fs.fail()) {
      scr.error("Error reading crc32 checksum file:%s", fname.c_str());
      _crc32 = 0;
      return;
    }
    csum = ::Crc32_ComputeBuf(csum, buf, buflen);
  }
  // Handle the last buffer < buflen
  if (fs.gcount() > 0)
    csum = ::Crc32_ComputeBuf(csum, buf, fs.gcount());
  fs.close();
  _crc32 = csum;
  char tmp1[128];
  sprintf(tmp1, "%08" PRIX32 "", (uint32_t)_crc32);
  scr.debug(4, "crc32::crc32(%s) %s", fname.c_str(), tmp1);
  return;
}

crc32::crc32(uint32_t* i)
{
  _crc32 = *i;
  char tmp1[128];
  sprintf(tmp1, "crc32::crc32(%08" PRIX32 ") %08" PRIX32 "", (uint32_t)_crc32,
          *i);
  string s = tmp1;
  scr.debug(4, s);
  return;
}

// ************************************************************************************
// MD5 Checksum
// ************************************************************************************

// Return the string value of a md5 checksum
string
md5::strval()
{
  char tmp[128];

  string s("");
  for (size_t i = 0; i < CSUMLEN_MD5 * 4; i++) {
    sprintf(tmp, "%02" PRIX8 "", (uint8_t)_md5[i]);
    s += tmp;
    if (i == 3 || i == 7 || i == 11)
      s += " ";
  }
  return (s);
}

string
md5::print()
{
  char tmp[128];

  string s("MD5 ");
  for (size_t i = 0; i < CSUMLEN_MD5 * 4; i++) {
    sprintf(tmp, "%02" PRIX8 "", (uint8_t)_md5char[i]);
    s += tmp;
    if (i == 3 || i == 7 || i == 11)
      s += " ";
  }
  return (s);
}

md5::md5(uint32_t* i)
{
  uint32_t* j = i;

  for (size_t x = 0; x < CSUMLEN_MD5 * 4; x++) {
    _md5char[x * 4 + 0] = ((*i) >> 24);
    _md5char[x * 4 + 1] = ((*i) >> 16) & 0xFF;
    _md5char[x * 4 + 2] = ((*i) >> 8) & 0xFF;
    _md5char[x * 4 + 3] = (*i) & 0xFF;
    i++;
  }
  for (size_t x = 0; x < CSUMLEN_MD5; x++)
    _md5[x] = *j++;
}

md5::md5(string fname)
{
  const size_t buflen = 1024;
  char buf[buflen];
  ::MD5_CTX context;

  ifstream fs(fname.c_str(), ios::binary);
  if (!fs.is_open()) {
    scr.error("MD5: Can't open file: %s", fname.c_str());
    for (size_t i = 0; i < CSUMLEN_MD5 * 4; i++)
      _md5char[i] = (uchar_t)0;
    for (size_t i = 0; i < CSUMLEN_MD5; i++)
      _md5[i] = 0;
    return;
  }
  ::MD5Init(&context);
  /** accumulate MD5 from file **/
  while (fs.read(buf, buflen)) {
    if (fs.fail()) {
      scr.error("Error reading MD5 checksum file:%s", fname.c_str());
      for (size_t i = 0; i < CSUMLEN_MD5; i++)
        _md5[i] = 0;
      for (size_t i = 0; i < CSUMLEN_MD5 * 4; i++)
        _md5char[i] = (uchar_t)0;
      fs.close();
      return;
    }
    ::MD5Update(&context, (unsigned char*)&buf[0], buflen);
  }
  // Handle the last buffer < buflen
  if (fs.gcount() > 0)
    ::MD5Update(&context, (unsigned char*)&buf[0], fs.gcount());
  ::MD5Final(_md5char, &context);
  fs.close();
  for (size_t i = 0; i < CSUMLEN_MD5; i++) {
    _md5[i] = _md5char[i * 4] << 24;
    _md5[i] &= (_md5char[i * 4 + 1] << 16);
    _md5[i] &= (_md5char[i * 4 + 2] << 8);
    _md5[i] &= (_md5char[i * 4 + 3]);
  }
  return;
}

// ************************************************************************************
// SHA1 Checksum
// ************************************************************************************

// Return the string value of a sha1 checksum
string
sha1::strval()
{
  char tmp[128];

  string s("");
  for (size_t i = 0; i < CSUMLEN_SHA1; i++) {
    sprintf(tmp, "%08" PRIX32 "", (uint32_t)_sha1[i]);
    s += tmp;
    if (i < CSUMLEN_SHA1 - 1)
      s += " ";
  }
  return (s);
}

string
sha1::print()
{
  char tmp[128];

  string s("SHA1 ");
  for (size_t i = 0; i < CSUMLEN_SHA1; i++) {
    sprintf(tmp, "%08" PRIX32 "", (uint32_t)_sha1[i]);
    s += tmp;
    if (i < CSUMLEN_SHA1 - 1)
      s += " ";
  }
  return (s);
}

sha1::sha1(uint32_t* i)
{
  for (size_t c = 0; c < CSUMLEN_SHA1; c++)
    _sha1[c] = *i++;
}

sha1::sha1(string fname)
{
  ::SHA1Context sha;
  const size_t buflen = 1024;
  char buf[buflen];

  ifstream fs(fname.c_str(), ios::binary);
  if (!fs.is_open()) {
    scr.error("SHA1: Can't open file: %s", fname.c_str());
    return;
  }
  ::SHA1Reset(&sha);
  /** accumulate SHA1 from file **/
  while (fs.read(buf, buflen)) {
    if (fs.fail()) {
      scr.error("Error reading SHA1 checksum file:%s", fname.c_str());
      return;
    }
    for (unsigned int i = 0; i < buflen; i++)
      SHA1Input(&sha, (unsigned char*)&buf[i], 1);
  }
  // Handle the last buffer < buflen
  if (fs.gcount() > 0) {
    for (unsigned int i = 0; i < fs.gcount(); i++)
      ::SHA1Input(&sha, (unsigned char*)&buf[i], 1);
  }
  if (!::SHA1Result(&sha)) {
    scr.error("Could not compute SHA1 message digest for %s\n", fname.c_str());
    return;
  }
  for (unsigned int i = 0; i < CSUMLEN_SHA1; i++)
    _sha1[i] = (uint32_t)sha.Message_Digest[i];
  fs.close();
  return;
}

}; // Namespace checksums
