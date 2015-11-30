/*
 
 Copyright (c) 2012, Charles Smith
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


#include <iostream>
#include <cstring>
#include <string>
#include <sys/time.h>
#include <sys/resource.h>

#include "saratoga.h"
#include "screen.h"
#include "globals.h"
#include "sarflags.h"

using namespace std;

namespace saratoga
{

const flag_t Fversion::bits;  // No initializer here.
const flag_t Fversion::msb;  // No initializer here.

const flag_t Fframetype::bits;  // No initializer here.
const flag_t Fframetype::msb;  // No initializer here.

const flag_t Fdescriptor::bits;  // No initializer here.
const flag_t Fdescriptor::msb;  // No initializer here.

const flag_t Fstream::bits;  // No initializer here.
const flag_t Fstream::msb;  // No initializer here.

const flag_t Ftransfer::bits;  // No initializer here.
const flag_t Ftransfer::msb;  // No initializer here.

const flag_t Freqtstamp::bits;  // No initializer here.
const flag_t Freqtstamp::msb;  // No initializer here.

const flag_t Fprogress::bits;  // No initializer here.
const flag_t Fprogress::msb;  // No initializer here.

const flag_t Ftxwilling::bits;  // No initializer here.
const flag_t Ftxwilling::msb;  // No initializer here.

const flag_t Fmetadata_udptype::bits;  // No initializer here.
const flag_t Fmetadata_udptype::msb;  // No initializer here.

const flag_t Fmetadatarecvd::bits;  // No initializer here.
const flag_t Fmetadatarecvd::msb;  // No initializer here.

const flag_t Fallholes::bits;  // No initializer here.
const flag_t Fallholes::msb;  // No initializer here.

const flag_t Frequesttype::bits;  // No initializer here.
const flag_t Frequesttype::msb;  // No initializer here.

const flag_t Frxwilling::bits;  // No initializer here.
const flag_t Frxwilling::msb;  // No initializer here.

const flag_t Freqholes::bits;  // No initializer here.
const flag_t Freqholes::msb;  // No initializer here.

const flag_t Ffileordir::bits;  // No initializer here.
const flag_t Ffileordir::msb;  // No initializer here.

const flag_t Freqstatus::bits;  // No initializer here.
const flag_t Freqstatus::msb;  // No initializer here.

const flag_t Fudptype::bits;  // No initializer here.
const flag_t Fudptype::msb;  // No initializer here.

const flag_t Feod::bits;  // No initializer here.
const flag_t Feod::msb;  // No initializer here.

const flag_t Ffreespace::bits;  // No initializer here.
const flag_t Ffreespace::msb;  // No initializer here.

const flag_t Ffreespaced::bits;  // No initializer here.
const flag_t Ffreespaced::msb;  // No initializer here.

const flag_t Fcsumlen::bits;  // No initializer here.
const flag_t Fcsumlen::msb;  // No initializer here.

const flag_t Fcsumtype::bits;  // No initializer here.
const flag_t Fcsumtype::msb;  // No initializer here.

const flag_t Ferrcode::bits;  // No initializer here.
const flag_t Ferrcode::msb;  // No initializer here.





















// What is the descriptor size for the largest file type
enum f_descriptor
maxdescriptor()
{
	struct rlimit rlim;

	if (getrlimit(RLIMIT_FSIZE, &rlim) < 0)
	{
		scr.perror(errno, "maxdescriptor(): Cannot getrlimit setting 16 bit:");
		return(F_DESCRIPTOR_16);
	}
	if (rlim.rlim_cur <= 65536)	// 2^16
		return(F_DESCRIPTOR_16);
	if (rlim.rlim_cur <= 4294967296) // 2^32
		return(F_DESCRIPTOR_32);
	return(F_DESCRIPTOR_64);	// 2^64 or more we dont do 2^128 yet
}

}; // Namespace saratoga
