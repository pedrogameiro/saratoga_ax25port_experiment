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

/*
 * Handle the Saratoga Header and Saratoga Directory Entry Flags Fields
 * We can set them, get them, print them
 *
 * Basic principle is to have an enum that holds the values of each particular
 * flag and then a class to handle variables & functions attaining to that flag.
 *
 * There is lots of bit manipulation going on here. 
 * The saratoga header uses a 32 bit unsigned integer (uint32_t) flags field.
 * The saratoga directory header uses a 16 bit unsigned integer (uint16_t) flags field.
 */

#include <iostream>

#include <cstring>
#include <string>
#include "sarflags.h"
#include "timestamp.h"
#include "screen.h"
#include "globals.h"
#include "dirent.h"

using namespace std;

namespace saratoga
{

string
printbits(flag_t fshift, flag_t fmask)
{
	string s("");
#ifdef DEBUGFLAGS
	// Very verbose it prints out the flag masks
	uint32_t shift = (uint32_t) fshift;
	uint32_t mask = (uint32_t) fmask;

	bool flags[32];
	for (size_t i = 0; i < 32; i++)
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
		flags[31 - shift - i] = 1;
	s += "\n                     1                   2                   3\n";
	s += " 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1\n";
	s += "|";
	for (size_t i = 0; i < 32; i++)
	{
		if (flags[i] == 1)
			s += "1|";
		else
			s += " |";
	}
	s += "\n";
#endif
	return s;
}

// Print out the header frame type and the 32 bit flags
string
printflags(string header, flag_t flags)
{
	string s;
	char	tmp[30];

	sprintf(tmp, "\n    %-20.20s ", header.c_str());
	s += tmp;
	s += "1                   2                   3\n";
	s += "    |0 1 2 3 4 5 6 7|8 9 0 1 2 3 4 5|6 7 8 9 0 1 2 3|4 5 6 7 8 9 0 1|\n";
	s += "    |";
	for (int i = 0; i < 32; i++)
	{
		flag_t tmpf = flags;
		tmpf <<= i;
		tmpf >>= 31;
		if (tmpf == 1)
			s += "1|";
		else
			s += " |";
	}
	s += "\n";
	return s;
}

string
Fflag::print()
{
	string s = "";

	s += "FLAGS                  1                   2                   3\n";
	s += " 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1\n";
	s += "|";
	for (int i = 0; i < 32; i++)
	{
		flag_t tmpf = flag;
		tmpf <<= i;
		tmpf >>= 31;
		if (tmpf == 1)
			s += "1|";
		else
			s += " |";
	}
	s += "\n";
	return(s);
}

/*
 **********************************************************************************************************
 * Fversion functions
 */

// Print out the current saratoga version
string
Fversion::print()
{
	string s("Saratoga ");

	switch(Fversion::get())
	{
	case F_VERSION_0:
		s += "Version 0";
		break;
	case F_VERSION_1:
		s += "Version 1";
		break;
	default:
		s += "Unsupported Version";
		break;
	}
	s += printbits(Fversion::shift(), Fversion::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Fframetype functions
 */

// Print out the frametype 
string
Fframetype::print()
{
	string s("");

	switch(Fframetype::get())
	{
	case F_FRAMETYPE_BEACON:
		s += "BEACON";
		break;
	case F_FRAMETYPE_REQUEST:
		s += "REQUEST";
		break;
	case F_FRAMETYPE_METADATA:
		s += "METADATA";
		break;
	case F_FRAMETYPE_DATA:
		s += "DATA";
		break;
	case F_FRAMETYPE_STATUS:
		s += "ERRCODE";
		break;
	default:
		s += "Unrecognised Frame Type";
		break;
	}
	s += printbits(Fframetype::shift(), Fframetype::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Fdescriptor functions
 */

// Print out the descriptor 
string
Fdescriptor::print()
{
	string s("Descriptor ");

	switch(Fdescriptor::get())
	{
	case F_DESCRIPTOR_16:
		s += "16 Bit";
		break;
	case F_DESCRIPTOR_32:
		s += "32 Bit";
		break;
	case F_DESCRIPTOR_64:
		s += "64 Bit";
		break;
	case F_DESCRIPTOR_128:
#ifdef UINT128_T
		s+= "128 Bit";
#else
		s += "128 Bit UNSUPPORTED";
#endif
		break;
	default:
		s += "Invalid";
		break;
	}
	s += printbits(Fdescriptor::shift(), Fdescriptor::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Fstream funtions
 */

// Print out the stream 
string
Fstream::print()
{
	string s("Streams ");

	switch(Fstream::get())
	{
	case F_STREAMS_NO:
		s += "Off";
		break;
	case F_STREAMS_YES:
		s += "On";
		break;
	default:
		s += "Invalid";
		break;
	}
	s += printbits(Fstream::shift(), Fstream::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Ftransfer functions
 */

// Print out the transfer 
string
Ftransfer::print()
{
	string s("Transfering ");

	switch(Ftransfer::get())
	{
	case F_TRANSFER_FILE:
		s += "File";
		break;
	case F_TRANSFER_DIR:
		s += "Directory";
		break;
	case F_TRANSFER_BUNDLE:
		s += "Bundle";
		break;
	case F_TRANSFER_STREAM:
		s += "Stream";
		break;
	default:
		s += "Invalid";
		break;
	}
	s += printbits(Ftransfer::shift(), Ftransfer::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Ftimestamp functions
 */

// Print out the timestamp 
string
Freqtstamp::print()
{
	string s("Timestamps ");

	switch(Freqtstamp::get())
	{
	case F_TIMESTAMP_NO:
		s += "Off";
		break;
	case F_TIMESTAMP_YES:
		s += "On";
		break;
	default:
		s + "Invalid";
		break;
	}
	s += printbits(Freqtstamp::shift(), Freqtstamp::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Fprogress functions
 */

// Print out the progress 
string
Fprogress::print()
{
	string s("Progress ");

	switch(Fprogress::get())
	{
	case F_PROGRESS_INPROG:
		s += "In Progress";
		break;
	case F_PROGRESS_TERMINATED:
		s += "Terminated";
		break;
	default:
		s += "Invalid";
		break;
	}

	s += printbits(Fprogress::shift(), Fprogress::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Ftxwilling functions
 */

// Print out the txwilling 
string
Ftxwilling::print()
{
	string s("TX Willing ");

	switch(Ftxwilling::get())
	{
	case F_TXWILLING_NO:
		s += "No";
		break;
	case F_TXWILLING_INVALID:
		s += "Invalid";
		break;
	case F_TXWILLING_CAPABLE:
		s += "Yes but not right now";
		break;
	case F_TXWILLING_YES:
		s += "Yes";
		break;
	default:
		s += "Invalid";
		break;
	}
	s += printbits(Ftxwilling::shift(), Ftxwilling::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Freliable functions
 */

// Print out the metadata UDP Only Flag 
string
Fmetadata_udptype::print()
{
	string s("Udplite Unreliability ");

	switch(Fmetadata_udptype::get())
	{
	case F_UDPONLY:
		s += "Only";
		break;
	case F_UDPLITE:
		s += "or UDPLITE";
		break;
	default:
		s += "Invalid";
		break;
	}
	s += printbits(Fmetadata_udptype::shift(), Fmetadata_udptype::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Fmetadatarecvd functions
 */

// Print out the metadatarecvd 
string
Fmetadatarecvd::print()
{
	string s("Metadata ");

	switch(Fmetadatarecvd::get())
	{
	case F_METADATARECVD_YES:
		s += "Received";
		break;
	case F_METADATARECVD_NO:
		s += "Not Received";
		break;
	default:
		s += "Invalid";
		break;
	}
	s += printbits(Fmetadatarecvd::shift(), Fmetadatarecvd::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Fallholes functions
 */

// Print out the allholes 
string
Fallholes::print()
{
	string s("All Holes ");

	switch(Fallholes::get())
	{
	case F_ALLHOLES_YES:
		s += "Received";
		break;
	case F_ALLHOLES_NO:
		s += "Not Received Yet";
		break;
	default:
		s += "Invalid";
		break;
	}
	s += printbits(Fallholes::shift(), Fallholes::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Frequesttype functions
 */

// Print out the requesttype 
string
Frequesttype::print()
{
	string s("Request ");

	switch(Frequesttype::get())
	{
	case F_REQUEST_NOACTION:
		s += "No action";
		break;
	case F_REQUEST_GET:
		s += "Get File";
		break;
	case F_REQUEST_PUT:
		s += "Put File";
		break;
	case F_REQUEST_GETDELETE:
		s += "Get then Delete File";
		break;
	case F_REQUEST_PUTDELETE:
		s += "Put then Delete File";
		break;
	case F_REQUEST_DELETE:
		s += "Delete File";
		break;
	case F_REQUEST_GETDIR:
		s += "Get Directory Listing";
		break;
	default:
		s += "Invalid";
		break;
	}
	s += printbits(Frequesttype::shift(), Frequesttype::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Frxwilling functions
 */

// Print out the rxwilling 
string
Frxwilling::print()
{
	string s("RX Willing ");

	switch(Frxwilling::get())
	{
	case F_RXWILLING_NO:
		s += "No";
		break;
	case F_RXWILLING_INVALID:
		s += "Invalid";
		break;
	case F_RXWILLING_CAPABLE:
		s += "Yes but not right now";
		break;
	case F_RXWILLING_YES:
		s += "Yes";
		break;
	default:
		s += "Invalid";
		break;
	}
	s += printbits(Frxwilling::shift(), Frxwilling::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Fholes functions
 */

// Print out the holes 
string
Freqholes::print()
{
	string s("Holes ");

	switch(Freqholes::get())
	{
	case F_HOLES_REQUESTED:
		s += "Requested";
		break;
	case F_HOLES_SENTVOLUNTARILY:
		s += "Sent Voluntarily";
		break;
	default:
		s += "Invalid";
		break;
	}
	s += printbits(Freqholes::shift(), Freqholes::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Ffileordir functions
 */

// Print out the fileordir 
string
Ffileordir::print()
{
	string s("Transferring ");

	switch(Ffileordir::get())
	{
	case F_FILEORDIR_FILE:
		s += "File";
		break;
	case F_FILEORDIR_DIRECTORY:
		s += "Directory";
		break;
	default:
		s += "Invalid";
		break;
	}
	s += printbits(Ffileordir::shift(), Ffileordir::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Freqstatus functions
 */

// Print out the reqstatus 
string
Freqstatus::print()
{
	string s("Status ");

	switch(Freqstatus::get())
	{
	case F_REQSTATUS_NO:
		s += "Not Requested";
		break;
	case F_REQSTATUS_YES:
		s += "Requested";
		break;
	default:
		s += "Invalid";
		break;
	}
	s += printbits(Freqstatus::shift(), Freqstatus::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Fudptype functions
 */

// Print out the udptype 
string
Fudptype::print()
{
	string s("UDP ");

	switch(Fudptype::get())
	{
	case F_UDPONLY:
		s += "Only";
		break;
	case F_UDPLITE:
		s += "or UDPLITE";
		break;
	default:
		s += "Invalid";
		break;
	}
	s += printbits(Fudptype::shift(), Fudptype::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Feod functions
 */

// Print out the eod 
string
Feod::print()
{
	string s("End of Data ");

	switch(Feod::get())
	{
	case F_EOD_NO:
		s += "No";
		break;
	case F_EOD_YES:
		s += "Set";
		break;
	default:
		s += "Invalid";
		break;
	}
	s += printbits(Feod::shift(), Feod::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Ffreespace functions
 */

// Print out the freespace 
string
Ffreespace::print()
{
	string s("Freespace ");

	switch(Ffreespace::get())
	{
	case F_FREESPACE_NO:
		s += "Not Advertised";
		break;
	case F_FREESPACE_YES:
		s += "Advertised";
		break;
	default:
		s += "Invalid";
		break;
	}
	s += printbits(Ffreespace::shift(), Ffreespace::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Ffreespace descriptor functions
 */

// Print out the freespace descriptor size
string
Ffreespaced::print()
{
	string s("");

	switch(Ffreespaced::get())
	{
	case F_FREESPACED_16:
		s += "16 Bit";
		break;
	case F_FREESPACED_32:
		s += "32 Bit";
		break;
	case F_FREESPACED_64:
		s += "64 Bit";
		break;
	case F_FREESPACED_128:
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
	s += printbits(Ffreespaced::shift(), Ffreespaced::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Fcsumlen functions
 */

// Print out the csum 
string
Fcsumlen::print()
{
	string s("Csum Len ");

	switch(Fcsumlen::get())
	{
	case F_CSUMLEN_NONE:
		s += "0 bits";
		break;
	case F_CSUMLEN_CRC32:
		s += "32 bits";
		break;
	case F_CSUMLEN_MD5:
		s += "128 bits";
		break;
	case F_CSUMLEN_SHA1:
		s += "160 bits";
		break;
	default:
		s += "Unsupported";
		break;
	}
	s += printbits(Fcsumlen::shift(), Fcsumlen::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Fcsum functions
 */

// Print out the csum 
string
Fcsumtype::print()
{
	string s("Checksum ");

	switch(Fcsumtype::get())
	{
	case F_CSUM_NONE:
		s += "None";
		break;
	case F_CSUM_CRC32:
		s += "CRC32";
		break;
	case F_CSUM_MD5:
		s += "MD5";
		break;
	case F_CSUM_SHA1:
		s += "SHA1";
		break;
	default:
		s += "Invalid";
		break;
	}
	// s += printbits(Fcsumtype::shift(), Fcsumtype::mask());
	return(s);
}

/*
 **********************************************************************************************************
 * Fstatus functions
 */

// Print out the Error Code 
string
Ferrcode::print()
{
	string s("Error Code ");

	switch(Ferrcode::get())
	{
	case F_ERRCODE_SUCCESS:
		s += "Success";
		break;
	case F_ERRCODE_UNSPEC:
		s += "Unspecified Error";
		break;
	case F_ERRCODE_NOSEND:
		s += "Cannot Send";
		break;
	case F_ERRCODE_NORECEIVE:
		s += "Cannot Receive";
		break;
	case F_ERRCODE_NOFILE:
		s += "File Not Found";
		break;
	case F_ERRCODE_NOACCESS:
		s += "Access Denied";
		break;
	case F_ERRCODE_NOID:
		s += "No Transaction ID Received";
		break;
	case F_ERRCODE_NODELETE:
		s += "Cannot Delete File";
		break;
	case F_ERRCODE_TOOBIG:
		s += "File is Too Big";
		break;
	case F_ERRCODE_BADDESC:
		s += "Bad Descriptor";
		break;
	case F_ERRCODE_BADPACKET:
		s += "Bad Saratoga Frame";
		break;
	case F_ERRCODE_BADFLAG:
		s += "Invalid Saratoga Flag";
		break;
	case F_ERRCODE_SHUTDOWN:
		s += "Shut Down Transaction";
		break;
	case F_ERRCODE_PAUSE:
		s += "Pause Transaction";
		break;
	case F_ERRCODE_RESUME:
		s += "Resume Transaction";
		break;
	case F_ERRCODE_INUSE:
		s += "File currently in use";
		break;
	case F_ERRCODE_NOMETADATA:
		s += "Metadata Has Not Been Received";
		break;
	default:
		s += "Invalid";
		break;
	}
	s += printbits(Ferrcode::shift(), Ferrcode::mask());
	return(s);
}

/*
 **********************************************************************************************************
 **********************************************************************************************************
 */

/*
 **********************************************************************************************************
 * Ttimestamp functions
 */

// Print out the timestamp flags
string
Ttimestamp::print()
{

	string s("Timestamp ");
	switch(Ttimestamp::get())
	{
	case T_TSTAMP_UDEF:
		s += "Undef";
		break;
	case T_TSTAMP_32:
		s += "32";
		break;
	case T_TSTAMP_64:
		s += "64";
		break;
	case T_TSTAMP_32_32:
		s += "32_32";
		break;
	case T_TSTAMP_64_32:
		s += "64_32";
		break;
	case T_TSTAMP_32_Y2K:
		s += "32_Y2K";
		break;
	default:
		scr.error("Invalid Timestamp Flag: %u", (uint16_t) Ttimestamp::get());
		s += "Invalid";
		break;
	}
	s += " ";
	return(s);
}

// How may bytes in my descriptor
size_t
Fdescriptor::length()
{

	switch(Fdescriptor::get())
	{
	case F_DESCRIPTOR_16:
		return sizeof(uint16_t);
	case F_DESCRIPTOR_32:
		return sizeof(uint32_t);
	case F_DESCRIPTOR_64:
		return sizeof(uint64_t);
	case F_DESCRIPTOR_128:
#ifdef UINT128_T
		return sizeof(uint128_t);
#else
		scr.error("Descriptor size of 128 bit not supported");
		return(16);
#endif
		break;
	}
	scr.error("Descriptor size not supported");
	return 2;
}

// How may bytes in my freespace descriptor
size_t
Ffreespaced::length()
{

	switch(Ffreespaced::get())
	{
	case F_FREESPACED_16:
		return sizeof(uint16_t);
	case F_FREESPACED_32:
		return sizeof(uint32_t);
	case F_FREESPACED_64:
		return sizeof(uint64_t);
	case F_FREESPACED_128:
#ifdef UINT128_T
		return sizeof(uint128_t);
#else
		scr.error("Descriptor size of 128 bit illegal");
#endif
		break;
	}
	scr.error("Freespace Descriptor size illegal");
	return 2;
}

}; // namespace saratoga

