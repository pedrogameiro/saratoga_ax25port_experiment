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

#include "saratoga.h"

/*
 * Convert host to unsigned long long
 */
uint64_t
htonll(uint64_t x)
{
	union htonll_type
	{
		uint64_t	ull;
		char		c[8];
	};
	union htonll_type	tmp, y;
	static int		i = 1;

	/*
	 * We are Bigendian just return the same
	 */
	if ((*(char *)&i) == 0)
		return((uint64_t) x);

	/*
	 * We are Little Endian swap the bytes
	 */
	tmp.ull = x;

	y.c[0] = tmp.c[7];
	y.c[1] = tmp.c[6];
	y.c[2] = tmp.c[5];
	y.c[3] = tmp.c[4];
	y.c[4] = tmp.c[3];
	y.c[5] = tmp.c[2];
	y.c[6] = tmp.c[1];
	y.c[7] = tmp.c[0];

	return((uint64_t) y.ull);
}

/*
 * Convert network to host unsigned long long
 */
uint64_t
ntohll(uint64_t x)
{
	union htonll_type
	{
		uint64_t	ull;
		char		c[8];
	};
	union htonll_type	tmp, y;
	static int		i = 1;

	/*
	 * We are Bigendian just return the same
	 */
	if ((*(char *)&i) == 0)
		return((uint64_t) x);

	/*
	 * We are Little Endian swap the bytes
	 */
	tmp.ull = x;

	y.c[0] = tmp.c[7];
	y.c[1] = tmp.c[6];
	y.c[2] = tmp.c[5];
	y.c[3] = tmp.c[4];
	y.c[4] = tmp.c[3];
	y.c[5] = tmp.c[2];
	y.c[6] = tmp.c[1];
	y.c[7] = tmp.c[0];

	return((uint64_t) y.ull);
}
