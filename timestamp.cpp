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
#include <chrono>

#include "saratoga.h"
#include "sarflags.h"
#include "screen.h"
#include "globals.h"
#include "timestamp.h"

using namespace std;


namespace saratoga
{

// util/timepoint.hpp

#include <chrono>
#include <ctime>
#include <string>

const time_t timestamp::y2k;

// convert calendar time to timepoint of system clock
inline
std::chrono::system_clock::time_point
makeTimePoint (int year, int mon, int day,
               int hour, int min, int sec=0)
{
   struct std::tm t;
   t.tm_sec = sec;        // second of minute (0 .. 59 and 60 for leap seconds)
   t.tm_min = min;        // minute of hour (0 .. 59)
   t.tm_hour = hour;      // hour of day (0 .. 23)
   t.tm_mday = day;       // day of month (0 .. 31)
   t.tm_mon = mon-1;      // month of year (0 .. 11)
   t.tm_year = year-1900; // year since 1900
   t.tm_isdst = -1;       // determine whether daylight saving time
   std::time_t tt = std::mktime(&t);
   if (tt == -1) {
       throw "no valid system time";
   }
   return std::chrono::system_clock::from_time_t(tt);
}

/*
 * Given a timestamp buffer assemble the timestamp
 * The timestamp buffer is 16 bytes long with the last
 * byte holding the format and fields held in the previous 15 bytes
 * we use this when we read in a timestamp buffer from the peer
 */
timestamp::timestamp(char b[])
{
	uint32_t	tmp_32;
	uint64_t	tmp_64;
	time_t		secs;
	time_t		nsecs;
	std::chrono::system_clock::time_point	epoch;

	_ttype = (t_timestamp) b[15];

	switch(_ttype)
	{
	case T_TSTAMP_UDEF:
		_timestamp = epoch;
		return;
	case T_TSTAMP_32:
	case T_TSTAMP_32_Y2K:
		memcpy(&tmp_32, &b[0], sizeof(uint32_t));
		secs = (int64_t) ntohl(tmp_32);
		nsecs = 0;
		break;
	case T_TSTAMP_64:
		memcpy(&tmp_64, &b[0], sizeof(uint64_t));
		secs = (int64_t) ntohll(tmp_64);
		nsecs = 0;
		break;
	case T_TSTAMP_32_32:
		memcpy(&tmp_32, &b[0], sizeof(uint32_t));
		secs = (int64_t) ntohl(tmp_32);
		memcpy(&tmp_32, &b[4], sizeof(uint32_t));
		nsecs = (int64_t) ntohl(tmp_32);
		break;
	case T_TSTAMP_64_32:
		memcpy(&tmp_64, &b[0], sizeof(uint64_t));
		secs = (int64_t) ntohll(tmp_64);
		memcpy(&tmp_32, &b[8], sizeof(uint32_t));
		nsecs = (int64_t) ntohl(tmp_32);
		break;
	default:
		scr.error("timestamp(char b[]):Invalid Timestamp Type %u", (uint_t) _ttype);
		_timestamp = epoch;
		return;
	}
	_timestamp = epoch + std::chrono::seconds(secs) + std::chrono::nanoseconds(nsecs);
}

/*
 * Set the timestamp according to an exsting time_t time
 */
timestamp::timestamp(enum t_timestamp t, time_t tm)
{
	std::chrono::system_clock::time_point	epoch;
	Ttimestamp	tstamp(t);
	_ttype = tstamp.set(t);

	switch(tstamp.get())
	{
	case T_TSTAMP_32:
	case T_TSTAMP_32_32:
	case T_TSTAMP_64:
	case T_TSTAMP_64_32:
		_timestamp = epoch + std::chrono::seconds(tm);
		break;
	case T_TSTAMP_32_Y2K:
		_timestamp = epoch + std::chrono::seconds(tm) + std::chrono::seconds(timestamp::y2k);
		break;
	default:
		_ttype = T_TSTAMP_UDEF;
		_timestamp = epoch;
		break;
	}
}

string
timestamp::typeprint()
{
	string s;
	switch(_ttype)
	{
	case T_TSTAMP_UDEF:
		s += "Undefined";
		break;
	case T_TSTAMP_32:
		s += "32 bit secs";
		break;
	case T_TSTAMP_64:
		s += "64 bit secs";
	case T_TSTAMP_32_32:
		s += "32 bit secs 32 bit nsecs";
		break;
	case T_TSTAMP_64_32:
		s += "64 bit secs 32 bit nsecs";
		break;
	case T_TSTAMP_32_Y2K:
		s += "32 bit secs Y2K epoch";
		break;
	default:
		s += "Invalid Timestamp Type";
		break;
	}
	return s;
}

// convert timestamp to calendar time string
string
timestamp::print()
{
	struct tm	t;
	struct tm	*tp;
	time_t	isecs;
	string	ts;

	// convert to system time:
	isecs = std::chrono::system_clock::to_time_t(_timestamp);
	tp = gmtime_r(&isecs, &t); // convert to calendar Zulu time
	if (tp != NULL)
		ts.resize(ts.size()-1);	// remove trailing newline
	else
		return "Invalid timetamp::print()";
	return ts;
}

/*
 * Return an ascii readable string of the date and time
 * it is a UTC or LOCAL time and decodes the struct tm
 */
string
timestamp::asctime()
{
	string s = "";
	struct tm	t;
	struct tm	*tp;
	char		tb[128];
	time_t		isecs;

	isecs = std::chrono::system_clock::to_time_t(_timestamp);
	switch(_ttype)
	{
	case T_TSTAMP_32:
	case T_TSTAMP_64:
	case T_TSTAMP_32_32:
	case T_TSTAMP_64_32:
		break;
	case T_TSTAMP_32_Y2K:
		isecs -= timestamp::y2k;
		break;
	case T_TSTAMP_UDEF:
		return("USER-DEFINED TIMESTAMP");
	default:
		return("UNFAMILIAR TIMESTAMP TYPE");
	}
	if (c_timezone.tz() == TZ_UTC)
		tp = gmtime_r(&isecs, &t);
	else
		tp = localtime_r(&isecs, &t);
	if (tp != nullptr)
	{
		strftime(tb, 64, "%c %Z", tp);
		s += tb;
	}
	else
		s += "timestamp::asctime: Invalid timestamp";
	return(s);
}

string
timestamp::printlong()
{

	string s = "Timestamp (";
	s += this->typeprint();
	s += "): ";
	s += this->asctime();
	return(s);
}

// Short hh:mm:ss UTC Time
string
timestamp::printshort()
{
	string s = "";
	struct tm	t;
	struct tm	*tp;
	char		tb[128];
	time_t		isecs;

	isecs = std::chrono::system_clock::to_time_t(_timestamp);
	/*
	 * we make sure everything is in UTC time zone
	 */
	switch(_ttype)
	{
	case T_TSTAMP_32:
	case T_TSTAMP_64:
	case T_TSTAMP_32_32:
	case T_TSTAMP_64_32:
		break;
	case T_TSTAMP_32_Y2K:
		isecs -= timestamp::y2k;
	case T_TSTAMP_UDEF:
		return("USER-DEFINED TIMESTAMP");
		break;
	default:
		return("UNFAMILIAR TIMESTAMP TYPE");
	}
	if (c_timezone.tz() == TZ_UTC)
		tp = gmtime_r(&isecs, &t);
	else
		tp = localtime_r(&isecs, &t);
	if (tp != nullptr)
	{
		strftime(tb, 64, "%c %Z", tp);
		s += tb;
	}
	else
		s += "timestamp::printshort: Invalid timestamp";
	return(s);
}

/*
 * Add n secs to a timestamp.
 */
const timestamp 
operator+(const timestamp& t1, const int64_t n)
{
	timestamp	tmp;

	tmp._ttype = t1._ttype;
	tmp._timestamp = t1._timestamp + std::chrono::seconds(n);
	return tmp;
}

/*
 * Subtract timestamp t2 from t1
 */
const timestamp
operator-(const timestamp& t1, const timestamp& t2)
{
	std::chrono::system_clock::time_point	epoch;
	timestamp	tmp;
	timestamp	tmp1 = t1;
	timestamp	tmp2 = t2;
	std::chrono::nanoseconds	dur;

	// Convert to posix epoch if needed
	if (t1._ttype == T_TSTAMP_32_Y2K)
		tmp1._timestamp += std::chrono::seconds(timestamp::y2k);
	if (t2._ttype == T_TSTAMP_32_Y2K)
		tmp2._timestamp += std::chrono::seconds(timestamp::y2k);

	dur = tmp1._timestamp - tmp2._timestamp;
	tmp._ttype = t1._ttype;
	tmp._timestamp = epoch + dur;

	// Convert to y2k epoch if needed
	if (tmp._ttype == T_TSTAMP_32_Y2K)
		tmp._timestamp -= std::chrono::seconds(timestamp::y2k);
	return(tmp);
}

/*
 * Subtract n secs from a timestamp.
 */
const timestamp
operator-(const timestamp& t1, int64_t n)
{
	timestamp	tmp;

	tmp._ttype = t1._ttype;
	tmp._timestamp = t1._timestamp - std::chrono::seconds(n);
	return(tmp);
}

/*
 * Convert timestamp structure to network byte order from host order
 * ready for transmission return pointer to static buffer
 */
char *
timestamp::hton()
{
	static char	b[16];	// Our output buffer is always 16 bytes
	uint32_t	tmp_32;
	uint64_t	tmp_64;
	time_t		secs = std::chrono::system_clock::to_time_t(_timestamp);
	time_t		nsecs = 0;

	// Clean slate
	bzero(&b[0], 16);
	switch(_ttype)
	{
	case T_TSTAMP_UDEF:
		b[15] = (char) T_TSTAMP_UDEF;
		return(b);
	case T_TSTAMP_32:
		b[15] = (char) T_TSTAMP_32;
		tmp_32 = (uint32_t) htonl((uint32_t) secs);
		memcpy(&b[0], &tmp_32, sizeof(uint32_t));
		tmp_32 = (uint32_t) htonl(0);
		memcpy(&b[4], &tmp_32, sizeof(uint32_t));
		return(b);
	case T_TSTAMP_64:
		b[15] = (char) T_TSTAMP_64;
		tmp_64 = (uint64_t) htonll((uint64_t) secs);
		memcpy(&b[0], &tmp_64, sizeof(uint64_t));
		tmp_32 = (uint32_t) htonl(0);
		memcpy(&b[8], &tmp_32, sizeof(uint32_t));
		return(b);
	case T_TSTAMP_32_32:
		b[15] = (char) T_TSTAMP_32_32;
		tmp_32 = (uint32_t) htonl((uint32_t) secs);
		memcpy(&b[0], &tmp_32, sizeof(uint32_t));
		tmp_32 = (uint32_t) htonl((uint32_t) nsecs);
		memcpy(&b[4], &tmp_32, sizeof(uint32_t));
		return(b);
	case T_TSTAMP_64_32:
		b[15] = (char) T_TSTAMP_64_32;
		tmp_64 = (uint64_t) htonll((uint64_t) secs);
		memcpy(&b[0], &tmp_64, sizeof(uint64_t));
		tmp_32 = (uint32_t) htonl((uint32_t) nsecs);
		memcpy(&b[8], &tmp_32, sizeof(uint32_t));
		return(b);
	case T_TSTAMP_32_Y2K:
		b[15] = (char) T_TSTAMP_32_Y2K;
		tmp_32 = (uint32_t) htonl((uint32_t) secs);
		memcpy(&b[0], &tmp_32, sizeof(uint32_t));
		tmp_32 = (uint32_t) htonl(0);
		memcpy(&b[4], &tmp_32, sizeof(uint32_t));
		return(b);
	default:
		scr.error("timestamp.hton(): Invalid Timestamp type %u", (uint32_t) b[15]);
		b[15] = (char) T_TSTAMP_UDEF;
		return(b);
	}
}

/*
 * Convert timestamp structure to host byte order from network order
 */

char *
timestamp::ntoh(char b[])
{
	static char	newb[16];

	uint32_t	tmp_32;
	uint64_t	tmp_64;

	_ttype = (t_timestamp) b[15];

	bzero(newb, 16);
	switch(_ttype)
	{
	case T_TSTAMP_UDEF:
		memcpy(newb, b, 16);
		break;
	case T_TSTAMP_32:
		memcpy(&tmp_32, &b[0], sizeof(uint32_t));
		tmp_32 = (uint32_t) ntohl(tmp_32);
		memcpy(&newb[0], &tmp_32, sizeof(uint32_t));
		break;
	case T_TSTAMP_64:
		memcpy(&tmp_64, &b[0], sizeof(uint64_t));
		tmp_64 = (uint64_t) ntohll(tmp_64);
		memcpy(&newb[0], &tmp_64, sizeof(uint64_t));
		break;
	case T_TSTAMP_32_32:
		// secs
		memcpy(&tmp_32, &b[0], sizeof(uint32_t));
		tmp_32 = (uint32_t) ntohl(tmp_32);
		memcpy(&newb[0], &tmp_32, sizeof(uint32_t));
		// nsecs
		memcpy(&tmp_32, &b[4], sizeof(uint32_t));
		tmp_32 = (uint32_t) ntohl(tmp_32);
		memcpy(&newb[4], &tmp_32, sizeof(uint32_t));
		break;
	case T_TSTAMP_64_32:
		// secs
		memcpy(&tmp_64, &b[0], sizeof(uint64_t));
		tmp_64 = (uint64_t) ntohll(tmp_64);
		memcpy(&newb[0], &tmp_64, sizeof(uint64_t));
		// nsecs
		memcpy(&tmp_32, &b[8], sizeof(uint32_t));
		tmp_32 = (uint32_t) ntohl(tmp_32);
		memcpy(&newb[8], &tmp_32, sizeof(uint32_t));
		break;
	case T_TSTAMP_32_Y2K:
		memcpy(&tmp_32, &b[0], sizeof(uint32_t));
		tmp_32 = (uint32_t) ntohl(tmp_32);
		memcpy(&newb[0], &tmp_32, sizeof(uint32_t));
		break;
	default:
		scr.error("timestamp(char b[]):Invalid Timestamp Type %u", (uint_t) _ttype);
		break;
	}
	return newb;
}

}; // Namespace saratoga

