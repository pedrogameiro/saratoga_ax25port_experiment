/*

 Copyright (c) 2014, Charles Smith
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

#include <cstring>
#include <string>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "screen.h"
#include "globals.h"
#include "sysinfo.h"

using namespace std;

namespace saratoga
{


// How much disk space is free on the named fs default bytes
offset_t
sysinfo::diskfree()
{
	return(this->diskfree(BYTE));
}

offset_t
sysinfo::diskfree(enum bkmg div)
{
	char	*fnPath = new char[_fsname.length()];
	struct statvfs fiData;
	uint64_t size;

	strcpy(fnPath, _fsname.c_str());
	if ((statvfs(fnPath, &fiData)) < 0)
	{
		scr.error("No such filesystem: %s\n", fnPath);
		delete [] fnPath;
		return(0);
	}
	else
	{
		delete [] fnPath;
//		printf("Disk %s: \n", fnPath);
//		printf("\tblock size: %lu\n", fiData.f_bsize);
//		printf("\ttotal no blocks: %lu\n", fiData.f_blocks);
//		printf("\tfree blocks: %lu\n", fiData.f_bfree);

		// So we dont blow size of uint64_t
		switch(div)
		{
		case BYTE:
			size = 1;
			break;
		case KILO:
			size = 1024;
			break;
		case MEGA:
			size = 1024 * 1024;
			break;
		case GIGA:
			size = 1024 * 1024 * 1024;
			break;
		default:
			return(0);
		}
		return ((fiData.f_bsize * fiData.f_bfree)/size);
	}
}

}; // namespace saratoga
