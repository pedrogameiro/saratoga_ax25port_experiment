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
#include <curses.h>

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
#include "holes.h"
#include "ip.h"
#include "globals.h"
#include "timer.h"

const int SARATOGASOCKET = 7548;

#define DEBUG_LEVEL 0
// #define TESTBEACON
// #define TESTDATA
#define TESTTIMESTAMP
// #define TESTMETADATA
// #define TESTDIRENT
// #define TESTCHECKSUMS

// #include <iostream>

#define DEBUGFLAGS

using namespace std;

using namespace saratoga;

#ifdef TESTCHECKSUMS
// Test out the checksum logic and == and != checking
void
testchecksum(string fname1, string fname2)
{
	scr.msg("Doing No checksum");
	checksums::none	n1(fname1);
	checksums::none	n2(fname2);
	scr.msg(n1.print());
	scr.msg(n2.print());
	if (n1 == n2)
		scr.msg("== NONE true");
	else
		scr.msg("== NONE false");
	if (n1 != n2)
		scr.msg("!= NONE true");
	else
		scr.msg("!= NONE false");

	scr.msg("Doing CRC32");
	checksums::crc32	c1(fname1);
	checksums::crc32 c2(fname2);
	scr.msg(c1.print());
	scr.msg(c2.print());
	if (c1 == c2)
		scr.msg("== CRC32 true");
	else
		scr.msg("== CRC32 false");
	if (c1 != c2)
		scr.msg("!= CRC32 true");
	else
		scr.msg("!= CRC32 false");
	if (c2 == "BBCD0A7D")
		scr.msg("== String CRC32 true");
	else
		scr.msg("== String CRC32 false");
	if (c2 != "BBCD0A7D")
		scr.msg("!= String CRC32 true");
	else
		scr.msg("!= String CRC32 false");

	scr.msg("Doing MD5");
	checksums::md5 m1(fname1);
	checksums::md5 m2(fname2);
	scr.msg(m1.print());
	scr.msg(m2.print());
	if (m1 == m2)
		scr.msg("== MD5 true");
	else
		scr.msg("== MD5 false");
	if (m1 != m2)
		scr.msg("!= MD5 true");
	else
		scr.msg("!= MD5 false");
	if (m2 == "C3882AC2 32E69CAA 8928D904 10E3B1F9")
		scr.msg("== String MD5 true");
	else
		scr.msg("== String MD5 false");
	if (m2 != "C3882AC2 32E69CAA 8928D904 10E3B1F9")
		scr.msg("!= String MD5 true");
	else
		scr.msg("!= String MD5 false");

	scr.msg("Doing SHA1");
	checksums::sha1	s1(fname1);
	checksums::sha1	s2(fname2);
	scr.msg(s1.print());
	scr.msg(s2.print());
	if (s1 == s2)
		scr.msg("== SHA1 true");
	else
		scr.msg("== SHA1 false");
	if (s1 != s2)
		scr.msg("!= SHA1 true");
	else
		scr.msg("!= SHA1 false");
	if (s2 == "E3989E28 0E43A79A E32067A0 30FA3E07 1397AB1F")
		scr.msg("== String SHA1 true");
	else
		scr.msg("== String SHA1 false");
	if (s2 != "E3989E28 0E43A79A E32067A0 30FA3E07 1397AB1F")
		scr.msg("!= String SHA1 true");
	else
		scr.msg("!= String SHA1 false");
}
#endif // TESTCHECKSUMS

#ifdef TEST1
int
main(int argc, char **argv)
{
	sarwin::screen	scr;
	c_debug.level(DEBUG_LEVEL);

	scr.msg("Testing Saratoga");

#ifdef TESTTIMESTAMP
	timestamp	t;
	scr.msg("Default: %s", t.asctime().c_str());

	timestamp	t_32(T_TSTAMP_32);
	scr.msg("t_32: %s", t_32.asctime().c_str());

	timestamp	t_1(T_TSTAMP_32, 10, 0);
	scr.msg("t_1: %s", t_1.asctime().c_str());

	timestamp	tn1 = t_1 + 25;
	scr.msg("tn1 = t_1 + 25: %s",tn1.asctime().c_str());
	timestamp	tn2 = t_1 - 5;
	scr.msg("tn2 = t_1 - 5: %s", tn2.asctime().c_str());
	tn2--;
	scr.msg("tn2--: %s", tn2.asctime().c_str());
	--tn2;
	scr.msg("--tn2: %s", tn2.asctime().c_str());

	timestamp	tn3(T_TSTAMP_32, 0, 0);
	tn3++;
	++tn3;
	scr.msg("Should be 2: %s", tn3.asctime().c_str());
	(--tn3)--;
	scr.msg("Should be 0: %s", tn3.asctime().c_str());

	timestamp	tn4(T_TSTAMP_32, 0, 0);
	--tn4;
	scr.msg("Should be ?: %s", tn4.asctime().c_str());

	timestamp	tn5(T_TSTAMP_32, 0, 0);
	scr.msg("Should be epoch: %s", tn5.asctime().c_str());
	tn5 += 60;
	scr.msg("Should be epoch +60: %s", tn5.asctime().c_str());
	tn5 -= 60;
	scr.msg("Should be epoch again: %s", tn5.asctime().c_str());

	timestamp	tn6(T_TSTAMP_32, 100, 0);
	tn5 -= tn6;
	scr.msg("Should be epoch -100: %s", tn5.asctime().c_str());

	timestamp	tn7(T_TSTAMP_32, 100, 0);
	timestamp	tn8(T_TSTAMP_32, 100, 0);
	scr.msg("tn7=%s tn8=%s", tn7.asctime().c_str(), tn7.asctime().c_str());
	if (tn7 == tn8)
		scr.msg("tn7 == tn8");
	if (tn7 != tn8)
		scr.msg("tn7 != tn8");
	if (tn7 <= tn8)
		scr.msg("tn7 <= tn8");
	if (tn7 >= tn8)
		scr.msg("tn7 >= tn8");
	if (tn7 > tn8)
		scr.msg("tn7 > tn8");
	if (tn7 < tn8)
		scr.msg("tn7 < tn8");

	timestamp	tn9(T_TSTAMP_64_32);
	scr.msg("Current Time is: %s", tn9.asctime().c_str());
	char	buf1[16];
	memcpy(buf1, tn9.hton(), 16);
	timestamp	tn10(buf1);
	scr.msg("memcpy And it still is Current Time is: %s", tn10.asctime().c_str());

	timestamp	tn11 = tn10;
	scr.msg("Assignement And it still is Current Time is: %s", tn11.asctime().c_str());
	timer_group::timer	t1;
	timer_group::timer	t2;

	t1 = 5 * 10E6;
	t2 = 0 * 10E6;
	if (t2.timedout())
		scr.msg("T2 has expired");
	else
		scr.msg("T2 has NOT expired");

	while (!t1.timedout())
	{
		scr.msg("T1 Not expired");
		sleep(1);
	}
	if (t1.timedout())
		scr.msg("T1 has expired");
	else
		scr.msg("T1 has not expired");
	
	sleep(15);
	exit(0);
#endif

// #define TESTCHECKSUMS
#ifdef TESTCHECKSUMS
	scr.msg("Checking where checksum is same");
	string fname1 = "checksum.cpp";
	string fname2 = fname1;
	testchecksum(fname1, fname2);

	scr.msg("Checking where checksum is different");
	string fname3 = "checksum.cpp";
	string fname4 = "checksum.h";
	testchecksum(fname3, fname4);
	
	sleep(15);
	exit(0);
#endif

// #define TESTBEACONS
#ifdef TESTBEACON

	// A beacon with no freespace advertised
	beacon *b1p = new beacon( F_DESCRIPTOR_16, 
		F_STREAM_NO,
		F_TXWILLING_YES, 
		F_RXWILLING_YES, 
		"0123456789");
	cout << "b1p F_FREESPACE_NO: F_STREAM_NO, F_TXWILLING_YES, F_RXWILLING_YES, 0123456789" << endl << b1p->print() << endl;

	// Get the freespace data we need
	fsinfo  *fs = new fsinfo("."); // The current directory at the moment

	// A beacon with freespace advertised
	beacon *b2p = new beacon( F_DESCRIPTOR_64, 
		F_STREAM_NO,
		F_TXWILLING_YES, 
		F_RXWILLING_NO,
		fs,
		"9876543210");
	cout << "b2p F_FREESPACE_YES: F_DESCRIPTOR_64, F_STREAM_NO, F_TXWILLING_YES, F_RXWILLING_NO, 9876543210" << endl << b2p->print() << endl;

	// Another one with freespace
	beacon b3( F_DESCRIPTOR_64, 
		F_STREAM_NO,
		F_TXWILLING_YES, 
		F_RXWILLING_YES,
		fs,
		"9876501234");
	cout << "b3 F_FREESPACE_YES: F_DESCRIPTOR_64, F_STREAM_NO, F_TXWILLING_YES, F_RXWILLING_YES, 9876501234" << endl << b3.print() << endl;

	// ANother one without freespace
	beacon b4( F_DESCRIPTOR_32, 
		F_STREAM_NO,
		F_TXWILLING_YES, 
		F_RXWILLING_YES, 
		"abcdefghij");
	cout << "b4 F_FREESPACE_NO: F_DESCRIPTOR_32 F_STREAM_NO F_TXWILLING_YES F_RXWILLING_YES abcdefghij" << endl << b4.print() << endl;

	// Lets see how we go when we create a frame that way
	frame *f = new beacon(b2p->payload(), b2p->paylen());


	if ( f->type() == F_FRAMETYPE_BEACON)
		cout << "f (same as b2p): " << f->print() << endl;
	else
		cout << "WTF we should of had a beacon!" << endl;

	beacon b5(f->payload(), f->paylen());

	frame *f2 = &b5;

	cout << "f2 == b5 F_TXWILLING_YES:" << f2->print() << endl;

	b5.txwilling(F_TXWILLING_NO);
	cout << "f2 == b5 F_TXWILLING_NO: " << f2->print() << endl;


	cout << "b5 (same as b1p): " << b5.print() << endl;
	sleep(15);
	exit(0);
#endif

// # TESTDATAS
#ifdef TESTDATA
	char payload[] = "0123456789";


	data* d1 = new data( F_DESCRIPTOR_16, 
		F_TRANSFER_FILE,
		F_REQSTATUS_YES, 
		F_EOD_YES,
		F_TIMESTAMP_YES,
		99,
		9876,
		payload,
		strlen(payload));
	cout << "d1: F_DESC_16 F_TRANSFER_FILE F_REQSTATUS_YES F_EOD_YES F_TIMESTAMP_YES 99 9876 0123456789" << endl << d1->print() << endl << endl;

	data* d2 = new data( F_DESCRIPTOR_32, 
		F_TRANSFER_FILE,
		F_REQSTATUS_YES, 
		F_EOD_NO,
		F_TIMESTAMP_YES,
		9999,
		12345,
		payload,
		strlen(payload));
	cout << "d2: F_DESC_32 F_TRANSFER_FILE F_REQSTATUS_YES F_EOD_NO F_TIMESTAMP_YES 9999 12345 0123456789" << endl << d2->print() << endl << endl;

	data* d3 = new data( F_DESCRIPTOR_64, 
		F_TRANSFER_FILE,
		F_REQSTATUS_NO, 
		F_EOD_NO,
		F_TIMESTAMP_YES,
		55,
		8231,
		payload,
		strlen(payload));
	cout << "d3: F_DESC_64 F_TRANSFER_FILE F_REQSTATUS_NO F_EOD_NO F_TIMESTAMP_YES 55 8231 0123456789" << endl << d3->print() << endl << endl;

	data* d4 = new data(d2->payload(), d2->paylen());
	cout << "Should be same as d2: " << endl << d4->print() << endl << endl;
	sleep(15);
	exit(0);
#endif

#ifdef FSINFO
	fsinfo *filsys = new fsinfo("/");

	cout << filsys->print() << endl;
	sleep(15);
	exit(0);
#endif

#ifdef NUMLIMITS
	cout << "Minimum value for uint16: " << numeric_limits<uint16_t>::min() << endl;
	cout << "Maximum value for unint16: " << numeric_limits<uint16_t>::max() << endl;
	cout << "Minimum value for uint32: " << numeric_limits<uint32_t>::min() << endl;
	cout << "Maximum value for uint32: " << numeric_limits<uint32_t>::max() << endl;
	cout << "Minimum value for unint64: " << numeric_limits<uint64_t>::min() << endl;
	cout << "Maximum value for unint64: " << numeric_limits<uint64_t>::max() << endl;
	cout << "Minimum value for offset: " << numeric_limits<offset_t>::min() << endl;
	cout << "Maximum value for offset: " << numeric_limits<offset_t>::max() << endl;
	sleep(15);
	exit(0);
#endif

#ifdef TESTDIRENT
	// Test  a file
/*
	scr.msg("\n");
	scr.msg("TESTING FILE DIRENT test.cpp");
	sardir::dirent *finfo = new sardir::dirent("test.cpp");
	scr.msg(finfo->print());
	scr.msg("Sleeping 5\n");
	sleep(5);
	scr.msg("\n");
	scr.msg("TESTING BUFFERED FILE DIRENT OF ABOVE test.cpp");
	sardir::dirent *nfinfo = new sardir::dirent(finfo->payload(), finfo->paylen());
	scr.msg("printing nfinfo");
	scr.msg(nfinfo->print());
	scr.msg("printed nfinfo");
	scr.msg("\n");
	// Test a directory
	scr.msg("TESTING DIRECTORY ENTRY OF .\n");
	sardir::dirent *dinfo = new sardir::dirent(".");
	scr.msg(dinfo->print());

	scr.msg("TESTING BUFFERED DIRENT OF .\n");
	sardir::dirent *ndinfo = new sardir::dirent(dinfo->payload(), dinfo->paylen());
	scr.msg(ndinfo->print());
*/

	// Test invalid file
	scr.msg("TESTING NON EXISTANT FILE frednurke\n");
	sarfile::fileio	fn("frednurke", sarfile::FILE_READ);
	sardir::dirent *iinfo = new sardir::dirent(fn->fname());
	scr.msg(iinfo->print());

	scr.msg("TESTING BUFFERED DIRENT OF ABOVE frednurke\n");
	sardir::dirent *idinfo = new sardir::dirent(iinfo->payload(), iinfo->paylen());
	scr.msg(idinfo->print());

	scr.msg("Sleeping 30\n");
	sleep(30);
	scr.msg("Bye\n");
	exit(0);
#endif // TESTDIRENT

#ifdef TESTMETADATA
	//Test metadata's

	sarfile::fileio	f("test.cpp", sarfile::FILE_READ);
	scr.msg("m1: F_DESC_16 F_TRANSFER_FILE F_PROGRESS_INPROG 55 test.cpp");
	metadata* m1 = new metadata( F_DESCRIPTOR_16,
		F_TRANSFER_FILE,
		F_PROGRESS_INPROG,
		55,
		&f);
	scr.msg(m1->print());
/*
	scr.msg("m2: F_DESC_32  F_TRANSFER_FILE F_PROGRESS_INPROG 56 .");
	metadata* m2 = new metadata( F_DESCRIPTOR_32,
			F_TRANSFER_FILE,
			F_PROGRESS_INPROG,
			56,
			".");
	scr.msg(m2->print());
	
	scr.msg("m3: F_DESC_32  F_TRANSFER_FILE F_CSUM_CRC32 F_PROGRESS_INPROG 57 test");
	metadata* m3 = new metadata( F_DESCRIPTOR_32,
			F_TRANSFER_FILE,
			F_PROGRESS_INPROG,
			F_CSUM_CRC32,
			57,
			"test");
	scr.msg(m3->print());

	scr.msg("m4: F_DESC_32  F_TRANSFER_FILE F_CSUM_MD5 F_PROGRESS_INPROG 58 test");
	metadata* m4 = new metadata( F_DESCRIPTOR_32,
			F_TRANSFER_FILE,
			F_PROGRESS_INPROG,
			F_CSUM_MD5,
			58,
			"test");
	scr.msg(m4->print());

	scr.msg("m5: F_DESC_32  F_TRANSFER_FILE F_CSUM_SHA1 F_PROGRESS_INPROG 59 test");
	metadata* m5 = new metadata( F_DESCRIPTOR_32,
			F_TRANSFER_FILE,
			F_PROGRESS_INPROG,
			F_CSUM_SHA1,
			59,
			"test");
	scr.msg(m5->print());

	scr.msg("m6==m5: F_DESC_32  F_TRANSFER_FILE F_CSUM_SHA1 F_PROGRESS_INPROG 59 test");
	metadata* m6 = new metadata(m5->payload(), m5->paylen());
	scr.msg(m6->print());
*/
	scr.msg("sleeping 15");
	sleep(15);
	scr.msg("bye");
	exit(0);

#endif // TESTMETADATA

#ifdef HOLES

	// offset_t b = numeric_limits<offset_t>::max();

	hole	h(0, 2);
	cout << "h=" << h.print() << endl;
	hole	h2(h);
	cout << "h2=" << h2.print() << endl;
	hole newh = h;
	cout << "Assign h=" << h.print() << " newh=" << newh.print() << endl;
	if (h == h2)
		cout << "h==h2 Correct" << endl;
	else
		cout << "ERROR h==h2" << endl;
	hole h3(4, 5);
	if (h != h3)
		cout << "h!=h3" << endl;
	else
		cout << "ERROR h!=h3" << endl;

	holes	hlist;
	cout << "\n\nBefore:" << hlist.print() << endl;
	hlist.add(10, 8);
	cout << "\n\nAfter:" << hlist.print() << endl;

	hlist.add(20, 8);
	hlist.add(30, 8);
	cout << "\n\nAfter:" << hlist.print() << endl;

	hole h1(21,4);
	holes hl1;
	hl1 += h1;
	hl1.add(29, 5);
	cout << "\n\nAfter:" << hlist.print() << endl;
	hlist -= hl1;
	cout << "\n\nAfter Remove a hole:" << hlist.print() << endl;
	hlist += h3;
	cout << "\n\nAfter Add hole:" << hlist.print() << endl;

	hlist.remove(21, 4);
	cout << "\n\nAfter Remove hole:" << hlist.print() << endl;
	hlist -= h3;
	cout << "\n\nAfter:" << hlist.print() << endl;


	hlist.zap();

	cout << "\n\nAfter Zap:" << hlist.print() << endl;
	sleep(15);
	exit(0);
#endif // HOLES

#ifdef IPV4TEST
	sarnet::ip ip1("192.94.244.11");
	sarnet::ip ip2("192.94.244.12");
	cout << "IP 1 Address:" << ip1.print() << " IP 2 Address:" << ip2.print() << endl;
	sarnet::ip ip3 = ip2;
	cout << "IP 3 Address:" << ip3.print() << endl;
	if (ip1 == ip2)
		cout << "IP1==IP2 " << ip1.print() << " " << ip2.print() << endl;
	else
		cout << "IP1!=IP2 " << ip1.print() << " " << ip2.print() << endl;
	// sarnet::ip ip2 = ip1; 
	// cout << "IP 2 Address:" << "192.94.244.11" << " " << ip2.str() << endl;
	sarnet::udp udp1("1.2.3.5", 16);
	cout << udp1.print() << endl;
	sarnet::udp udp2("127.0.0.1", 9999);
	cout << udp2.print() << endl;

	string s = "hello world!";
	char *cstr = (char *) malloc(s.length()) + 1;
	strcpy(cstr, s.c_str());
	udp1.tx(cstr, strlen(cstr));
	udp2.tx(cstr, strlen(cstr));

	sarnet::udp inv4(AF_INET, SARATOGASOCKET);

//	sarnet::udp udp61("fe80::20c:29ff:fe84:78ab", SARATOGASOCKET);
	sarnet::udp udp61("::1", SARATOGASOCKET);
	cout << udp61.print() << endl;
	udp61.tx(cstr, strlen(cstr));

	char buf[1024];
	ssize_t nread;
	sarnet::ip	*addrbuf = nullptr;

	nread = inv4.rx(buf, addrbuf);
	if (nread < 0)
		cout << "ERROR NREAD < 0" << endl;

	sarnet::udp inv6(AF_INET6, SARATOGASOCKET);
	nread = inv6.rx(buf, addrbuf);
	if (nread < 0)
		cout << "ERROR NREAD < 0" << endl;
	sarnet::udp udp62(AF_INET6, SARATOGASOCKET);

	sysinfo df("/dev/sda1");

	uint64_t xfree = df.diskfree();
	cout << "Disk Free is:" << xfree << "kB" << endl;
	sleep(15);
	exit(0);
#endif

}
#endif
