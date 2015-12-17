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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits>

#include <iostream>
#include <cstring>
#include <string>

#include "saratoga.h"
#include "screen.h"
#include "globals.h"
#include "sarflags.h"
#include "timestamp.h"
#include "dirent.h"

extern "C"
{
extern int stat(const char *path, struct stat *buf);
extern int statvfs(const char *path, struct statvfs *fidata);
}

namespace sardir 
{

// Prints out Directory Entry Flag Bits
string
printdbits(dflag_t fshift, dflag_t fmask)
{
	string s("");
#ifdef DEBUGFLAGS
	// Very verbose it prints out the directory flag masks
	uint16_t shift = (uint16_t) fshift;
	uint16_t mask = (uint16_t) fmask;

	bool flags[16];
	for (size_t i = 0; i < 16; i++)
		flags[i] = 0;
	size_t numbits = 0;

	switch(mask)
	{
	case 0x01: // 1
		numbits = 1;
		break;
	case 0x03: // 11
		numbits = 2;
		break;
	case 0x07: // 111
		numbits = 3;
		break;
	case 0x0F: // 1111
		numbits = 4;
		break;
	case 0x1F: // 11111
		numbits = 5;
		break;
	case 0x3F: // 111111
		numbits = 6;
		break;
	case 0x7F: // 1111111
		numbits = 7;
		break;
	case 0xFF: // 11111111
		numbits = 8;
		break;
	}
	for (size_t i = 0; i < numbits; i++)
		flags[15 - shift - i] = 1;
	s += "\n                   1\n";
	s += " 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5\n";
	s += "|";
	for (size_t i = 0; i < 16; i++)
	{
		if (flags[i] == 1)
			s += "1|";
		else
			s += " |";
	}
	s += "\n";
	// Now print out what the flags are set to
	/*
	for (size_t i = 0; i < 16; i++)
	{
		if (f >> i == 1)
			s += "1|";
		else
			s += " |";
	}
	*/
	s += "\n";
#endif
	return s;
}

// Given a local filename get its details and create
// a dirent from it IT WILL BE PLAFORM SPECIFIC
// you Windows lot put your coding hats on :)
dirent::dirent(const string &fname)
{
	struct stat	st;
	uint16_t	tmp_16;
	uint32_t	tmp_32;
	uint64_t	tmp_64;
#ifdef UINT128
	uint128_t	tmp_128;
#endif

	saratoga::scr.debug(7, "dirent: FORWARD CREATION OF DIRENT <%s>", fname.c_str());
	_valid = true;	// Assume all is good and we can access dir entry
	_baddir = false;
	_fname = fname;
	
	if (_fname.length() > 254)
	{
		_baddir = true;
		_valid = false;
		saratoga::scr.error("dirent: Max Length filename > 254");
		sleep(2);
		return;
	}
	Dsod sod = D_SOD_YES; // Start of Directory Marker
	Dprop prop = D_PROP_FILE; // We are a file
	Ddescriptor descriptor = D_DESCRIPTOR_16; // Default 16 bit

	// Make sure I can read and stat the file
	if (access(fname.c_str(), R_OK) == -1)
	{
		saratoga::scr.error("dirent: Can't access file: %s", fname.c_str());
		_mtime = timestamp(T_TSTAMP_32_Y2K, 0);
		_ctime = timestamp(T_TSTAMP_32_Y2K, 0);
		_filesize = 0;
		_fname = fname;
		prop = D_PROP_SPECIALFILE;
		_valid = false; // Always as it does not exist!
	}
	else if (stat(fname.c_str(), &st) != 0)
	{
		/*
		 * I can't stat it
		 * errno set to: EACCESS, EBADF, EFAULT, ELOOP
		 * ENAMETOOLONG, ENOENT, ENOMEM, ENOTDIR, EOVERFLOW
		 */
		saratoga::scr.error("dirent: Can't stat file: %s", fname.c_str());
		_mtime = timestamp(T_TSTAMP_32_Y2K, 0);
		_ctime = timestamp(T_TSTAMP_32_Y2K, 0);
		_filesize = 0;
		_fname = fname;
		prop = D_PROP_SPECIALFILE;
		_valid = false; // Always as it does not exist!
	}
	else
	{
		// We have stat'd it so lets get the correct info from the stat
		_filesize = (offset_t) st.st_size;
		_mtime = timestamp(T_TSTAMP_32_Y2K, st.st_mtime);
		_ctime = timestamp(T_TSTAMP_32_Y2K, st.st_ctime);

		saratoga::scr.debug(7, "dirent: Filesize=%ld MTIME=%s CTIME=%s",
			_filesize,
			_mtime.asctime().c_str(),
			_ctime.asctime().c_str());	
		/*
		 * These are the valid flags types applicable
		 * to a dirent set them
		 */
		switch(st.st_mode & S_IFMT)
		{
		case S_IFREG:	// regular file
			prop = D_PROP_FILE;
			saratoga::scr.debug(7, "dirent: Is a File");
			_valid = true;
			break;
		case S_IFDIR:	// directoy
			prop = D_PROP_DIR;
			saratoga::scr.debug(7, "dirent: Is a Directory");
			_valid = true;
			break;
		case S_IFCHR:	// character device
		case S_IFBLK:	// block device
		case S_IFIFO:	// FIFO named pipe
		case S_IFLNK:	// symbolic link
		case S_IFSOCK:	// socket
			prop = D_PROP_SPECIALFILE;
			_valid = false; // For the moment!!!!!
			saratoga::scr.error("dirent: Is a Special Device");
			break;
		default:
			prop = D_PROP_SPECIALFILE;
			saratoga::scr.error("dirent: Is an Unknown  Device");
			_valid = false;
		}
	}
	// The appropriate descriptor
	if (_filesize >= 0 && _filesize < (offset_t) numeric_limits<uint16_t>::max())
		descriptor = D_DESCRIPTOR_16;
	else if (_filesize < (offset_t) numeric_limits<uint32_t>::max())
		descriptor = D_DESCRIPTOR_32;
	else if (_filesize < (offset_t) numeric_limits<uint64_t>::max())
		descriptor = D_DESCRIPTOR_64;
	#ifdef UINT128_T
	else if (_filesize < (offset_t) numeric_limits<uint128_t>::max())
		descriptor = D_DESCRIPTOR_128;
	#endif
	else
	{
		_baddir = true;
		_valid = false;
		saratoga::scr.error("dirent: Can't get file size");
		return;
	}

	/*
	 * Turn on/off the bits appropriate in the
	 * header flags field so our flags are set correctly
	 */
	_flags = 0;
	_flags += sod;
	_flags += descriptor;
	_flags += prop;

	// Copy the directory info to the payload
	_dpaylen = sizeof(dflag_t) +
		descriptor.length() +
		sizeof(dtime_t) + sizeof(dtime_t) +
		fname.length() + 1;

	// Allocate the payload and copy it all
	_dpayload = new char[_dpaylen];
	char *bptr = _dpayload;

	// Flags
	tmp_16 = htons((uint16_t) _flags.get());
	memcpy(bptr, &tmp_16, sizeof(dflag_t));
	bptr += sizeof(dflag_t);

	// Filesize
	switch(descriptor.get())
	{
	case D_DESCRIPTOR_16:
		tmp_16 = htons((uint16_t) _filesize);
		memcpy(bptr, &tmp_16, sizeof(uint16_t));
		bptr += sizeof(uint16_t);
		break;
	case D_DESCRIPTOR_32:
		tmp_32 = htonl((uint32_t) _filesize);
		memcpy(bptr, &tmp_32, sizeof(uint32_t));
		bptr += sizeof(uint32_t);
		break;
	case D_DESCRIPTOR_64:
		tmp_64 = htonll((uint64_t) _filesize);
		memcpy(bptr, &tmp_64, sizeof(uint64_t));
		bptr += sizeof(uint64_t);
		break;
	case D_DESCRIPTOR_128:
#ifdef UINT128_T
		tmp_128 = htonlll((uint128_t) _filesize);
		memcpy(bptr, &tmp_128, sizeof(uint128_t));
		bptr += sizeof(uint128_t);
#else
		saratoga::scr.error("dirent: Descriptor 128 Bit Size Not Supported");
		break;
#endif
	default:
		saratoga::scr.error("dirent: Undefined Descriptor");
		_valid = false;
		_baddir = true;
		return;
	}

	dtime_t		dtmp;

	// MTime
	dtmp = htonl(_mtime.dirtime());
	memcpy(bptr, &dtmp, sizeof(dtime_t));
	bptr += sizeof(dtime_t);
	// CTime
	dtmp = htonl(_ctime.dirtime());
	memcpy(bptr, &dtmp, sizeof(dtime_t));
	bptr += sizeof(dtime_t);

	// The File Name
	memcpy(bptr, _fname.c_str(), _fname.length());
	bptr += _fname.length();
	*bptr = '\0'; // Make sure its always NULL terminated
}

// An input buffer convert to dirent
dirent::dirent(const char *payload,
	const size_t	&plen)

{
	uint32_t	tmp_32;
#ifdef UINT128_T
	uint128_t	tmp_128;
#endif

	size_t	paylen = plen;

	/* Copy the full dirent payload */
	_dpaylen = paylen;
	_dpayload = new char[_dpaylen];
	memcpy(_dpayload, payload, _dpaylen);

	// Copy the data fields and convert them
	// as required to host byte order

	// flags	
	_flags = ntohs(*(uint16_t *) payload);
	Dsod		sod = _flags.get();
	Ddescriptor	descriptor = _flags.get();
	Dprop		prop = _flags.get();
	
	payload += sizeof(dflag_t);
	paylen -= sizeof(dflag_t);

	_baddir = false;
	if (sod.get() != D_SOD_YES)
	{
		saratoga::scr.error("dirent: Invalid SOD Flag Not Set");
		_baddir = true;
		_valid = false;
		return;
	}	
	switch(descriptor.get())
	{
	case D_DESCRIPTOR_16:
		_filesize = (offset_t) ntohs(*(uint16_t *) payload);
		payload += sizeof(uint16_t);
		paylen -= sizeof(uint16_t);
		break;
	case D_DESCRIPTOR_32:
		_filesize = (offset_t) ntohl(*(uint32_t *) payload);
		payload += sizeof(uint32_t);
		paylen -= sizeof(uint32_t);
		break;
	case D_DESCRIPTOR_64:
		_filesize = (offset_t) ntohll(*(uint64_t *) payload);
		payload += sizeof(uint64_t);
		paylen -= sizeof(uint64_t);
		break;
	case D_DESCRIPTOR_128:
#ifdef UINT128_T
		_filesize = (offset_t) ntohlll(*(uint128_t *) payload);
		payload += sizeof(uint128_t);
		paylen -= sizeof(uint128_t);
#else
		saratoga::scr.error("dirent: Descriptor 128 Bit Size Not Supported");
		sleep(2);
		payload += 16;
		paylen -= 16;
		break;
#endif
	default:
		saratoga::scr.error("dirent: Undefined Descriptor");
		_baddir = true;
		_valid = false;
		break;
	}
	// We must have at least MTIME & CTIME & a NULL left
	if (paylen < sizeof(uint32_t) * 2 + 1)
	{
		saratoga::scr.error("dirent: Not enough space left for CTIME, MTIME & FNAME");
		sleep(2);
		_baddir = true;
		_valid = false;
		return;
	}
	// Mtime
	tmp_32 = (offset_t) ntohl(*(uint32_t *) payload);
	// We have to subtract it so that we don't double add it when we create new timestamp
	tmp_32 -= Y2KCONST; 
	_mtime = timestamp(T_TSTAMP_32_Y2K, tmp_32);
	// scr.debug(2, "DIRENT MTIME=%s", _mtime.asctime().c_str());
	payload += sizeof(dtime_t);
	paylen -= sizeof(dtime_t);
	
	// Ctime
	tmp_32 = (offset_t) ntohl(*(uint32_t *) payload);
	// We have to subtract is so we don't double add it when we create new timestamp
	tmp_32 -= Y2KCONST; 
	_ctime = timestamp(T_TSTAMP_32_Y2K, tmp_32);
	// scr.debug(2, "DIRENT CTIME=%s", _ctime.asctime().c_str());
	payload += sizeof(dtime_t);
	paylen -= sizeof(dtime_t);

	// Rest of payload is filename
	// scr.debug(2, "DIRENT File Name is: <%s>", payload);
	_fname.assign(payload, paylen);
	if (prop.get() == D_PROP_SPECIALFILE)
		_valid = false;
	else
		_valid = true;
	// scr.debug(2, "DIRENT PROPERTIRES is: <%s>", prop.print().c_str());
	saratoga::scr.debug(7, "Received DIRENT Decoded OK");
}

size_t
Ddescriptor::length()
{

	switch(Ddescriptor::get())
	{
	case D_DESCRIPTOR_16:
		return sizeof(uint16_t);
	case D_DESCRIPTOR_32:
		return sizeof(uint32_t);
	case D_DESCRIPTOR_64:
		return sizeof(uint64_t);
	case D_DESCRIPTOR_128:
#ifdef UINT128_T
		return sizeof(uint128_t);
#else
		saratoga::scr.error("Ddescriptor::length() Descriptor size of 128 bit illegal");
#endif
	break;
	}
	saratoga::scr.error("Ddescriptor::length() Descriptor size illegal");
	return D_DESCRIPTOR_16;
}

/*
 * Print out the bits set in the dirent flags
 */
string
dirent::printdflags(dflag_t flags)
{
	string s;

	s += "\n        DIRENTFLAGS          1\n";
	s += "        |0 1 2 3 4 5 6 7|8 9 0 1 2 3 4 5|\n";
	s += "        |";
	for (int i = 0; i <  16; i++)
	{
		dflag_t tmpf = flags;
		tmpf <<= i;
		tmpf >>= 15;
		if (tmpf == 1) 
			s += "1|";
		else
			s += " |";
	}
	s += "\n";
	return s;
}

/*
 * Print out the dirent flags and info
 */
string
dirent::print()
{
	char	tmp[256];
	Dsod	sod = _flags.get();
	Dprop	prop = _flags.get();
	Ddescriptor  des = _flags.get();

	enum d_prop dp = prop.get();

	if (_baddir)
		return "dirent::print(): Bad Directory Entry\n";
	string s("Directory Entry Information: ");
	s += this->printdflags(_flags.get());
	s += "        ";
	s += sod.print();
	s += "\n        ";
	if (_valid)
	{
		s += "Directory Entry is accessible";
	}
	else
	{
		s += "Directory Entry is not accessible";
	}
	s += "\n        ";
	s += prop.print();
	s += "\n        ";
	sprintf(tmp, "Filename: %s", _fname.c_str());
	s += tmp;
	if (dp == D_PROP_FILE)
	{
		s += "\n        ";
		sprintf(tmp, "Filesize=%" PRIu64 "", (uint64_t) _filesize);
		s += tmp;
		s += "\n        ";

		s += des.print();
	}
	if (dp == D_PROP_FILE || dp == D_PROP_DIR)
	{
		s += "\n        CTime: ";
		s += _ctime.asctime();
		s += "\n        MTime: ";
		s += _mtime.asctime();
	}
	return(s);
}

/*
 * ******************************************************************************************************
 */

/*
 *  Allocator for file system space information. Get the info and then
 *  work out what descriptor to use to show the free space and put
 *  into the beacon frame.
 */
fsinfo::fsinfo(string fname)
{
	struct statvfs fsdata;
	char path[256];

	// Copy fname from string to null terminated char*
	if (fname.length() < 256 )
	{
		size_t len = fname.copy(path,fname.length(),0);
		path[len]='\0';
	}
	else
	{
		saratoga::scr.perror(errno, "Can't fsinfo: file name too long");
		_fname = fname;
		_blocksize = 0;
		_totblocks = 0;
		_freeblocks = 0;
		_free1kblocks = 0;
		_freespaced = F_FREESPACED_16;
		return;
	}


	// Get the file system info for the file name
	// and complain if we can't
	if ((statvfs(path, &fsdata)) < 0)
	{
		saratoga::scr.perror(errno, "Can't fsinfo(%s): statvfs", path);
		_fname = fname;
		_blocksize = 0;
		_totblocks = 0;
		_freeblocks = 0;
		_freespaced = F_FREESPACED_16;
		return;
	}
	// Put it all into our class
	_fname = fname;
	_blocksize = (offset_t) fsdata.f_bsize;
	_totblocks = (offset_t) fsdata.f_blocks;
	_freeblocks = (offset_t) fsdata.f_bfree;
	_free1kblocks = (offset_t) ((double) _blocksize * ((double) _freeblocks / 1024.0));
	if (_free1kblocks < (offset_t) numeric_limits<uint16_t>::max())
		_freespaced = F_FREESPACED_16;
	else if (_free1kblocks < (offset_t) numeric_limits<uint32_t>::max())
		_freespaced = F_FREESPACED_32;
	else if (_free1kblocks < (offset_t) numeric_limits<uint64_t>::max())
		_freespaced = F_FREESPACED_64;
	else
	#ifdef UINT128_T
		_freespaced = F_FREESPACED_128;
	#else
		_freespaced = F_FREESPACED_64;
	#endif
	return;
}

string
fsinfo::print()
{
	char	tmp[128];

		string s("File System Information for: ");
		s += _fname;
		s += "\n    ";
		sprintf(tmp,
			"Blocksize: %ld", _blocksize);
		s += tmp;
		s += "\n    ";
		sprintf(tmp,
			"Total Blocks: %ld", _totblocks);
		s += tmp;
		s += "\n    ";
		sprintf(tmp, "Free Blocks: %ld", _freeblocks);
		s += tmp;
		s += "\n    ";
		sprintf(tmp, "Free Space 1kB Blocks: %ld\n", this->freespace());
		s += tmp;
		return(s);
}

/*
 *******************************************************************************************
 * Ddescriptor functions
 */

// Print out the descriptor 
string
Ddescriptor::print()
{
	string s("Directory Descriptor ");

	switch(Ddescriptor::get())
	{
	case D_DESCRIPTOR_16:
		s += "16 Bit";
		break;
	case D_DESCRIPTOR_32:
		s += "32 Bit";
		break;
	case D_DESCRIPTOR_64:
		s += "64 Bit";
		break;
	case D_DESCRIPTOR_128:
#ifdef UINT128_T
		s += "128 Bit";
#else
		s += "128 Bit UNSUPPORTED";
#endif
		break;
	default:
		s += "Invalid";
		break;
	}
	s += printdbits(Ddescriptor::shift(), Ddescriptor::mask());
	return(s);
}

/*
 ********************************************************************************************
 * Dproperties functions
 */

// Print out the properties 
string
Dprop::print()
{
	string s("Directory Properties ");

	switch(Dprop::get())
	{
	case D_PROP_FILE:
		s += "File";
		break;
	case D_PROP_DIR:
		s += "Directory";
		break;
	case D_PROP_SPECIALFILE:
		s += "Special File";
		break;
	case D_PROP_SPECIALDIR:
		s += "Special Directory";
		break;
	default:
		s += "Invalid";
		break;
	}
	s += printdbits(Dprop::shift(), Dprop::mask());
	return(s);
}

// Start of Directory Entry Bit
string
Dsod::print()
{
		string s("Start of Directory Entry is ");

		switch(Dsod::get())
		{
		case D_SOD_YES:
			s += "Set";
			break;
		default:
			s += "Invalid MUST BE 1";
		}
		s += printdbits(Dsod::shift(), Dsod::mask());
		return(s);
}

}; // Namespace sardir
