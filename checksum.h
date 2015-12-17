/*
 
 Copyright (c) 2011, Charles Smith
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, 
 are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this 
      list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this
      list of conditions and the following disclaimer in the documentation and/or 
      other materials provided with the distribution.
    * Neither the name of Vallona Networks nor the names of its contributors 
      may be used to endorse or promote products derived from this software without 
      specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#ifndef _CHECKSUM_H
#define _CHECKSUM_H

#include <unistd.h>
#include <stdio.h>
#include <inttypes.h>
#include <string>
#include <cctype>
#include "screen.h"
#include "checksums/md5.h"
#include "checksums/sha1.h"

// Functions used within the checksums library
extern "C"
{
	unsigned long Crc32_ComputeBuf(unsigned long, char *, size_t);
	void MD5Init(MD5_CTX *);
	void MD5Update(MD5_CTX *, unsigned char *, ssize_t);
	void MD5Final(unsigned char *, MD5_CTX *);
	void SHA1Reset(SHA1Context *);
	int SHA1Result(SHA1Context *);
	void SHA1Input(SHA1Context *, unsigned char *, unsigned);
}

using namespace std;
using namespace saratoga;
namespace checksums
{

enum csum_type
{
	CSUM_NONE = 0,
	CSUM_CRC32 = 1,
	CSUM_MD5 = 3,
	CSUM_SHA1 = 4
};

enum csum_len
{
	CSUMLEN_NONE = 0,
	CSUMLEN_CRC32 = 1,
	CSUMLEN_MD5 = 4,
	CSUMLEN_SHA1 = 5
};

typedef unsigned char uchar_t;

// Remove white space and punctuation from checksum strings
extern string	strip(string);

// Superclass for checksum so we can just have one of these pointing
// to the actual type of checksum. Makes adding new one easy later on

// No checksum to calculate
class none
{
private:
	static const enum csum_type		_csumtype = CSUM_NONE;
	static const enum csum_len		_csumlen = CSUMLEN_NONE;
	uint32_t			_csum = 0;
public:
	none() {};
	
	~none() { };
	void clear() { };


	bool	operator==(none c1) { return true; };
	bool	operator==(string s) { return (s == ""); };
	bool	operator!=(none c1) { return false; };
	bool	operator!=(string s) { return (s != ""); };

	
	// Copy constructor
	none(const none& old) { _csum = old._csum; };

	none& operator=(const none& old) { _csum = old._csum; return(*this); };
	
	size_t	size() { return (size_t) _csumlen; };
	enum csum_type	csumtype() { return _csumtype; };
	enum csum_len	csumlen() { return _csumlen; };

	string print();
	string strval() { return ""; };
};

// Handle CRC32's
class crc32
{
private:
	static const enum csum_type		_csumtype = CSUM_CRC32;
	static const enum csum_len		_csumlen = CSUMLEN_CRC32;
	uint32_t		_crc32;
public:

	crc32() { this->clear(); };
	crc32(string);
	crc32(uint32_t *);

	~crc32() { this->clear(); };
	void clear() { _crc32 = 0; };
	
	// Copy constructor
	crc32(const crc32& old) { _crc32 = old._crc32; };
	crc32& operator=(const crc32& old) { 
		_crc32 = old._crc32; return(*this); };
	
	// Equals Compare with another crc32
	bool	operator==(crc32 c1) {
		crc32	tmp(*this);
		return (tmp._crc32 == c1._crc32);
	}

	// Equals Compare with a string hlding a crc32 value
	bool	operator==(string s) {
		crc32	tmp(*this);
		string	s1 = tmp.strval();
		string s2 = s;
		s1 = strip(s1);
		s2 = strip(s2);
		return (s1 == s2);
	}

	// Not equals Compare with another crc32
	bool	operator!=(crc32 c1) {
		crc32	tmp(*this);
		return (tmp == c1) ? 0 : 1;
	}

	// Not equals Compare with a string holding a crc32 value
	bool	operator!=(string s) {
		crc32	tmp(*this);
		string	s1 = tmp.strval();
		string s2 = s;
		s1 = strip(s1);
		s2 = strip(s2);
		return (s1 != s2);
	}

	size_t	size() { return (size_t) _csumlen; };
	enum csum_type	csumtype() { return _csumtype; };
	enum csum_len	csumlen() { return _csumlen; };
	uint32_t	value() { return _crc32; };
	uint32_t	value(size_t i) { return _crc32; };

	string print();
	string	strval();
};

// Handle MD5's
class md5
{
private:
	static const enum csum_type		_csumtype = CSUM_MD5;
	static const enum csum_len		_csumlen = CSUMLEN_MD5;
	uint32_t		_md5[CSUMLEN_MD5]; // How we store and report
	uchar_t			_md5char[CSUMLEN_MD5 * 4]; // Char array used in calc
public:
	md5() { this->clear(); };
	md5(string);
	md5(uint32_t *);

	~md5() { this->clear(); };

	void clear() {
		for (size_t i = 0; i < CSUMLEN_MD5; i++)
			_md5[i] = 0;
	}

	// Copy Constructor
	md5(const md5& old) { 
		for (size_t i = 0; i < CSUMLEN_MD5; i++)
			_md5[i] = old._md5[i];
	}
	
	md5& operator=(const md5& old) { 
		for (size_t i = 0; i < CSUMLEN_MD5; i++)
			_md5[i] = old._md5[i];
		return(*this);
	}
	
	// Compare ==
	bool	operator==(md5 m1) {
		md5	tmp(*this);
		for (size_t i = 0; i < CSUMLEN_MD5; i++)
			if (tmp._md5[i] != m1._md5[i])
				return 0;
		return 1;
	}

	bool	operator==(string s) {
		md5	tmp(*this);
		string	s1 = tmp.strval();
		string s2 = s;
		s1 = strip(s1);
		s2 = strip(s2);
		return (s1 == s2);
	}

	// Compare !=
	bool	operator!=(md5 m1) {
		md5	tmp(*this);
		return (tmp == m1) ? 0 : 1;
	}

	bool	operator!=(string s) {
		md5	tmp(*this);
		string	s1 = tmp.strval();
		string s2 = s;
		s1 = strip(s1);
		s2 = strip(s2);
		return (s1 != s2);
	}

	size_t	size() { return (size_t) _csumlen; };
	enum csum_type	csumtype() { return _csumtype; };
	enum csum_len	csumlen() { return _csumlen; };
	uint32_t	value(size_t i) { return _md5[i]; };

	string print();
	string	strval();
};

// Handle SHA1's
class sha1
{
private:
	static const enum csum_type		_csumtype = CSUM_SHA1;
	static const enum csum_len		_csumlen = CSUMLEN_SHA1;
	uint32_t		_sha1[CSUMLEN_SHA1];
public:
	sha1() { this->clear(); };
	sha1(string);
	sha1(uint32_t *);

	~sha1() { this->clear(); };

	void clear() {
		for (size_t i = 0; i < CSUMLEN_SHA1; i++)
			_sha1[i] = 0;
	}

	// Copy Constructor
	sha1(const sha1& old) { 
		for (size_t i = 0; i < CSUMLEN_SHA1; i++)
			_sha1[i] = old._sha1[i];
	}
	
	sha1& operator=(const sha1& old) { 
		for (size_t i = 0; i < CSUMLEN_SHA1; i++)
			_sha1[i] = old._sha1[i];
		return(*this);
	}
	
	// Compare == with another sha1 value
	bool	operator==(sha1 s1) {
		sha1	tmp(*this);
		for (size_t i = 0; i < CSUMLEN_SHA1; i++)
			if (tmp._sha1[i] != s1._sha1[i])
				return 0;
		return 1;
	}

	bool	operator==(string s) {
		sha1	tmp(*this);
		string	s1 = tmp.strval();
		string s2 = s;
		s1 = strip(s1);
		s2 = strip(s2);
		return (s1 == s2);
	}

	// Compare != with another sha1 value
	bool	operator!=(sha1 s1) {
		sha1	tmp(*this);
		return (tmp == s1) ? 0 : 1;
	}

	bool	operator!=(string s) {
		sha1	tmp(*this);
		string	s1 = tmp.strval();
		string s2 = s;
		s1 = strip(s1);
		s2 = strip(s2);
		return (s1 != s2);
	}

	// size in uint32_t
	size_t	size() { return (size_t) _csumlen; };
	enum csum_type	csumtype() { return _csumtype; };
	enum csum_len	csumlen() { return _csumlen; };
	uint32_t	value(size_t i) { return _sha1[i]; };

	string print();
	string	strval();
};

class checksum
{
private:
	// All of the types they can be
	enum csum_type 		_csumtype;
	checksums::none		_none;
	checksums::crc32	_crc32;
	checksums::md5		_md5;
	checksums::sha1		_sha1;
public:
	checksum() { this->clear(); 
		_csumtype = CSUM_NONE; _none = checksums::none(); };

	checksum(const enum csum_type, const string&);

	checksum(const enum csum_type, uint32_t *);

	~checksum() { this->clear(); };

	void	clear() { _csumtype = CSUM_NONE; 
		_none.clear(); _crc32.clear(); _md5.clear(); _sha1.clear(); };

	// Copy constructor
	checksum(const checksum& old) 
	{
		_csumtype = old._csumtype;
		switch(_csumtype)
		{
		case CSUM_NONE:
			_none = old._none;
			_crc32.clear(); _md5.clear(); _sha1.clear();
			break;
		case CSUM_CRC32:
			_crc32 = old._crc32;
			_none.clear(); _md5.clear(); _sha1.clear();
			break;
		case CSUM_MD5:
			_md5 = old._md5;
			_none.clear(); _crc32.clear(); _sha1.clear();
			break;
		case CSUM_SHA1:
			_sha1 = old._sha1;
			_none.clear(); _crc32.clear(); _md5.clear();
			break;
		default:
			_none.clear(); _crc32.clear(); _md5.clear(); _sha1.clear();
			break;
		}
	}

	checksum& operator=(const checksum& old)
	{
		_csumtype = old._csumtype;
		switch(_csumtype)
		{
		case CSUM_NONE:
			_none = old._none;
			_crc32.clear(); _md5.clear(); _sha1.clear();
			break;
		case CSUM_CRC32:
			_crc32 = old._crc32;
			_none.clear(); _md5.clear(); _sha1.clear();
			break;
		case CSUM_MD5:
			_md5 = old._md5;
			_none.clear(); _crc32.clear(); _sha1.clear();
			break;
		case CSUM_SHA1:
			_sha1 = old._sha1;
			_none.clear(); _crc32.clear(); _md5.clear();
			break;
		default:
			_none.clear(); _crc32.clear(); _md5.clear(); _sha1.clear();
			break;
		}
		return (*this);
	}

	enum csum_type csumtype() { return _csumtype; };

	enum csum_len csumlen() {
		switch(_csumtype)
		{
		case CSUM_NONE: return CSUMLEN_NONE;
		case CSUM_CRC32: return CSUMLEN_CRC32;
		case CSUM_MD5: return CSUMLEN_MD5;
		case CSUM_SHA1: return CSUMLEN_SHA1;
		default: return CSUMLEN_NONE;
		}
	}

	size_t size() { return (size_t) this->csumlen(); };

	// Value at the index
	uint32_t value(size_t i) {
		switch(_csumtype)
		{
		case CSUM_NONE: return 0;
		case CSUM_CRC32: return _crc32.value();
		case CSUM_MD5: return _md5.value(i);
		case CSUM_SHA1: return _sha1.value(i);
		default: return 0;
		}
	}

	string strval();
	string print();
};

}; // Namespace checksums

#endif // _CHECKSUM_H

