/* MD5.H - header file for MD5C.C
 */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */

/* PROTOTYPES should be set to one if and only if the compiler supports
  function argument prototyping.
  The following makes PROTOTYPES default to 0 if it has not already
  been defined with C compiler flags.
 */

#ifndef _MD5_H
#define _MD5_H

#ifndef PROTOTYPES
#define PROTOTYPES 0
#endif

/* PROTO_LIST is defined depending on how PROTOTYPES is defined above.
If using PROTOTYPES, then PROTO_LIST returns the list, otherwise it
  returns an empty list.
 */
#if PROTOTYPES
#define PROTO_LIST(list) list
#else
#define PROTO_LIST(list) ()
#endif

#include <inttypes.h>

/* MD5 context. */
typedef struct {
  uint32_t state[4];                                   /* state (ABCD) */
  uint32_t count[2];        /* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];                         /* input buffer */
} MD5_CTX;

void MD5Init PROTO_LIST ((MD5_CTX *));
void MD5Update PROTO_LIST
  ((MD5_CTX *, unsigned char *, unsigned int));
extern void MD5Final PROTO_LIST ((unsigned char [16], MD5_CTX *));

#endif // _MD5_H

/*
 * $Log: md5.h,v $
 * Revision 1.1.1.1  2014/10/15 11:55:36  lloydwood
 * Creating saratoga-vallona-dev.
 *
 * This is the Saratoga transfer protocol Vallona implementation,
 * under development by Charles Smith.
 *
 * Revision 1.1.1.1  2013/03/09 07:28:18  chas
 * Checksum Library and standalone programs
 * crc32, md5 and sha1
 *
 */

