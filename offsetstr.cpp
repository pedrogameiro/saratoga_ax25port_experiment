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

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "saratoga.h"
#include "sarflags.h"

using namespace std;

namespace saratoga {

// Test out the checksum logic and == and != checking
string
offsetstr(f_descriptor d, offset_t o)
{
  char tmp[128];
  uint16_t tmp16 = (uint16_t)o;
  uint32_t tmp32 = (uint32_t)o;
  uint64_t tmp64 = (uint64_t)o;
#ifdef UINT128_T
  uint128_t tmp128 = (uint128_t)o;
#endif

  string s = "";
  switch (d) {
    case F_DESCRIPTOR_16:
      sprintf(tmp, "%" PRIu16 "", (uint16_t)tmp16);
      break;
    case F_DESCRIPTOR_32:
      sprintf(tmp, "%" PRIu32 "", (uint32_t)tmp32);
      break;
    case F_DESCRIPTOR_64:
      sprintf(tmp, "%" PRIu64 "", (uint64_t)tmp64);
      break;
    case F_DESCRIPTOR_128:
#ifdef UINT128_T
      sprintf(tmp, "%" PRIu128 "", (uint128_t)tmp128);
#else
      sprintf(tmp, "128 bit Offset not supported");
#endif
      break;
  }
  s += tmp;
  return s;
}

}; // Namespace saratoga
