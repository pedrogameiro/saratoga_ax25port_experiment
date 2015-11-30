/*
 *  sha1.h
 *
 *  Copyright (C) 1998, 2009
 *  Paul E. Jones <paulej@packetizer.com>
 *  All Rights Reserved
 *
 *****************************************************************************
 *  $Id: sha1.h,v 1.1.1.1 2014/10/15 11:55:36 lloydwood Exp $
 *****************************************************************************
 *
 *  Description:
 *      This class implements the Secure Hashing Standard as defined
 *      in FIPS PUB 180-1 published April 17, 1995.
 *
 *      Many of the variable names in the SHA1Context, especially the
 *      single character names, were used because those were the names
 *      used in the publication.
 *
 *      Please read the file sha1.c for more information.
 *
 */

#ifndef _SHA1_H
#define _SHA1_H

/* 
 *  This structure will hold context information for the hashing
 *  operation
 */
typedef struct SHA1Context
{
    unsigned Message_Digest[5]; /* Message Digest (output)          */

    unsigned Length_Low;        /* Message length in bits           */
    unsigned Length_High;       /* Message length in bits           */

    unsigned char Message_Block[64]; /* 512-bit message blocks      */
    int Message_Block_Index;    /* Index into message block array   */

    int Computed;               /* Is the digest computed?          */
    int Corrupted;              /* Is the message digest corruped?  */
} SHA1Context;

/*
 *  Function Prototypes
 */
#ifndef __cplusplus

extern void SHA1Reset(SHA1Context *);
extern int SHA1Result(SHA1Context *);
extern void SHA1Input( SHA1Context *, const unsigned char *, unsigned);

#endif // __cplusplus

#endif // _SHA1_H

/*
 * $Log: sha1.h,v $
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

