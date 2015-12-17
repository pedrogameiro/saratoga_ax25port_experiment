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
#include <fcntl.h>
#include <limits>
#include <ncurses.h>

#include "screen.h"

#include "sysinfo.h"
#include "fileio.h"
#include "saratoga.h"
#include "sarflags.h"
#include "timestamp.h"
#include "checksum.h"
#include "frame.h"
#include "beacon.h"
#include "data.h"
#include "dirent.h"
#include "metadata.h"
#include "request.h"
#include "holes.h"
#include "ip.h"
#include "globals.h"

// #define TESTBEACON
// #define TESTDATA
// #define TESTTIMESTAMP

// #include <iostream>

#define DEBUGFLAGS

using namespace std;

using namespace saratoga;

// Test out the checksum logic and == and != checking
#ifdef TEST
int
main(int argc, char **argv)
{
	cout << "Testing Saratoga" << endl;
	Fversion myversion = F_VERSION_1;
	Fframetype myframetype = F_FRAMETYPE_REQUEST;
	Fdescriptor mydescriptor = F_DESCRIPTOR_64;

	Fflag	flag;
	flag += myversion;
	flag += myframetype;
	flag += mydescriptor;

	myversion = flag.get();
	myframetype = flag.get();
	mydescriptor = flag.get();

	cerr << myversion.print() << myframetype.print() << mydescriptor.print();

	int x = 10;
	cerr << "x=" << x << " x/9=" << x / 9 << " x/10=" << x / 10 << " x/11=" << x / 11 << endl;

	char	buf[128];

	char	*p1 = &buf[0];
	char	*p2 = &buf[0];

	uint16_t tmp_16 = 123;
	uint32_t tmp_32 = 456;
	uint16_t tmp1_16;
	uint32_t tmp1_32;

	tmp_16 = htons((uint16_t) tmp_16);
	memcpy(p1, &tmp_16, sizeof(uint16_t));
	p1 += sizeof(uint16_t);
	tmp_32 = htonl((uint32_t) tmp_32);
	memcpy(p1, &tmp_32, sizeof(uint32_t));
	p1 += sizeof(uint32_t);

	tmp1_16 = ntohs(*(uint16_t *) p2);
	p2 += sizeof(uint16_t);
	tmp1_32 = ntohl(*(uint32_t *) p2);
	p2 += sizeof(uint32_t);

	cerr << "Before p1=" << tmp_16 << " After p1=" << tmp1_16 << endl;	
	cerr << "Before p2=" << tmp_32 << " After p2=" << tmp1_32 << endl;	

	sleep(10);	

//	sarwin::endwindows();
}
#endif
