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

#ifndef _SYSINFO_H
#define _SYSINFO_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits>
#include <unistd.h>
#include <errno.h>
// #include <stdio.h>
#include <string>

using namespace std;

#include "screen.h"
#include "saratoga.h"
#include "sysinfo.h"

/*
 * System information
 */
namespace saratoga
{

// Display values as
enum bkmg {
	BYTE = 0,
	KILO = 1,
	MEGA = 2,
	GIGA = 3
};

class sysinfo
{
private:
	string		_fsname; // File system name
public:
	sysinfo(string fsname) { _fsname = fsname; };
	~sysinfo() { _fsname = ""; };
	
	string fsname() { return _fsname; };
	offset_t diskfree(enum bkmg div);
	offset_t diskfree();
};

} // namespace saratoga

#endif // _SYSINFO_H
