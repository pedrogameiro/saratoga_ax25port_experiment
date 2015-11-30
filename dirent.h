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

#ifndef _DIRENT_H
#define _DIRENT_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <cstring>
#include <string>
using namespace std;

#include "saratoga.h"
#include "screen.h"
#include "timestamp.h"
#include "dirflags.h"

namespace sardir 
{

/*
 * DIRECTORY ENTRY
 */
class dirent
{
private:
	DEflag		_flags;	// Directory enties flags
	char		*_dpayload;	// A dirent payload
	size_t		_dpaylen;	// This is how long the payload is
	offset_t	_filesize;
	timestamp	_mtime;
	timestamp	_ctime;
	string		_fname;
	bool		_valid; // Can I access or stat this dirent ie does it exist.
	bool		_baddir; // Is this a good or bad direntory entry
public:
	dirent() { _dpayload = nullptr,
		_dpaylen = 0;
		_flags = 0;
		_filesize = 0;
		_mtime = timestamp();
		_ctime = timestamp();
		_fname = "";
		_valid = false; };

	// Local file on the system look its details up
	// and create a directory entry for it
	dirent(const string&);

	// We have been given a directory entry payload
	// so decode it and set up the values
	dirent(const char *, const size_t &);
	
	~dirent() { this->clear(); };

	void clear() { if (_dpaylen > 0) delete[] _dpayload;
		_dpaylen = 0;
		_flags = 0;
		_filesize = 0;
		_mtime.clear();
		_ctime.clear();
		_fname.clear();
		_valid = false;
		_baddir = true;
	};

	// Copy constructor
	dirent(const dirent &old)
	{
		if (old._dpaylen > 0)
		{
			_dpayload = new char[old._dpaylen];
			memcpy(_dpayload, old._dpayload, old._dpaylen);
		}
		else
			_dpayload = nullptr;
		_dpaylen = old._dpaylen;
		_flags = old._flags;
		_filesize = old._filesize;
		_mtime = old._mtime;
		_ctime = old._ctime;
		_fname = old._fname;
		_valid = old._valid;
		_baddir = old._baddir;
	}

	dirent& operator=(const dirent &old)
	{
		if (old._dpaylen > 0)
		{
			_dpayload = new char[old._dpaylen];
			memcpy(_dpayload, old._dpayload, old._dpaylen);
		}
		else
			_dpayload = nullptr;
		_dpaylen = old._dpaylen;
		_flags = old._flags;
		_filesize = old._filesize;
		_mtime = old._mtime;
		_ctime = old._ctime;
		_fname = old._fname;
		_valid = old._valid;
		_baddir = old._baddir;
		return *this; 
	};

	// Are we an invalid/bad directory entry record
	bool baddir()	{ return _baddir; };

	// Get various flags applicable to dirent
	enum d_prop	prop() { Dprop d = _flags.get(); return d.get(); };
	enum d_sod	sod() { Dsod d = _flags.get(); return d.get(); };
	enum d_descriptor	descriptor() { Ddescriptor d = _flags.get(); return d.get(); }; 

	// Set various flags applicable to dirent
	// enum d_prop	prop(d_prop);
	// enum d_sod	sod(d_sod);
	// enum d_descriptor	descriptor(d_descriptor);
	
	char	*payload() { return(_dpayload); };
	size_t	paylen() { return _dpaylen; };
	dflag_t 	flags() { return _flags.get(); };
	timestamp	mtime() { return _mtime; };
	timestamp	ctime() { return _ctime; };
	string		fname() { return _fname; };
	offset_t	filesize() { return _filesize; };
	bool		valid() { return _valid; };

	// enum d_descriptor	descriptor() { Ddescriptor d(_flags); return d.get(); };

	/* Print the details of the dirent */
	string	printdflags(dflag_t);
	string	print();
};

// Handles calculating the available free space where the named file is located
// Now this returns different values than df does and I dont know why!!
// This is VERY PLATFORM DEPENDANT. I am a linux guy, you windows lot can write your own :)

class fsinfo
{
private:
		string		_fname;		// File name relative to file system
		offset_t	_blocksize; // Block Size used
		offset_t	_totblocks; // Total number of blocks in file system
		offset_t	_freeblocks; // How many of those blocks are free
		offset_t	_free1kblocks; // How many 1kB blocks there are
		enum f_freespaced	_freespaced; // What freespace descriptor we use
public:
		fsinfo(string);

		offset_t	blocksize() { return _blocksize; };
		offset_t	totalblocks() { return _totblocks; };
		offset_t	freeblocks() { return _freeblocks; };

		// Number 1kB blocks  on file system
		offset_t freespace() { return _free1kblocks; };
		// What freespace descriptor needed to hold the _free1kblocks integer
		// Now there MAY be a problem here once file system sizes exceed offset_t
		// given offset_t is a signed integer and we use unsigned integers in saratoga
		// offsets
		enum f_freespaced	freespaced() { return _freespaced; };

		string	print();

};

} // Namepace sardir

#endif // _DIRENT_H

