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

#ifndef _GLOBALS_H
#define _GLOBALS_H

#include "checksum.h"
#include "cli.h"
#include "fileio.h"
#include "ip.h"
#include "peerinfo.h"
#include "sarflags.h"
#include "screen.h"
#include "timestamp.h"
#include "tran.h"
#include <cstring>
#include <iostream>
#include <list>
#include <string>
#include <sys/types.h>
#include <vector>

// Special Characters for inputs
static const int CHAR_CTL_C = 3;           // Bye
static const int CHAR_CTL_D = 4;           // Bye
static const int CHAR_CTL_H = 8;           // Backspace
static const int CHAR_DELETE = 127;        // Treat as Backspace
static const int CHAR_BSPACE = 8;          // Backspace
static const int CHAR_TAB = (int)'\t';     // auto complete
static const int CHAR_CR = (int)'\r';      // send command
static const int CHAR_NL = (int)'\n';      // send command
static const int CHAR_LF = (int)'\n';      // send command
static const int CHAR_QUESTION = (int)'?'; // help

using namespace std;

/*
 * If a variable or class is to be used globally then it must be declared as
 * external
 * here and it to be fully declared in globals.cpp so that saratoga.o and
 * checksum.a
 * can reference it as well
 */

namespace saratoga {

extern sarwin::screen scr;

extern cli_beacon c_beacon;
extern cli_exit c_exit;
extern cli_checksum c_checksum;
extern cli_debug c_debug;
extern cli_descriptor c_descriptor;
extern cli_eid c_eid;
extern cli_freespace c_freespace;
extern cli_get c_get;
extern cli_getrm c_getrm;
extern cli_history c_history;
extern cli_home c_home;
extern cli_ls c_ls;
extern cli_put c_put;
extern cli_putrm c_putrm;
extern cli_rm c_rm;
extern cli_rmdir c_rmdir;
extern cli_rx c_rx;
extern cli_session c_session;
extern cli_stream c_stream;
extern cli_timer c_timer;
extern cli_timestamp c_timestamp;
extern cli_timezone c_timezone;
extern cli_transfers c_transfers;
extern cli_tx c_tx;
extern cli_multicast c_multicast;
extern cli_prompt c_prompt;
extern cli_maxbuff c_maxbuff;

// The saratoga iana assigned port number
extern uint16_t sarport;
extern const string if_loop;
extern const string if6_loop;
extern const string if_mcast;
extern const string if6_mcast;

// Permanently open udp sockets
extern sarnet::udp* v4out;

extern sarnet::udp* ax25multiout;
extern sarnet::udp* ax25multiin;
extern sarnet::udp* v4mcastout;
extern sarnet::udp* v6out;
extern sarnet::udp* v6mcastout;

extern sarnet::udp* v4in;
extern sarnet::udp* v6in;
extern sarnet::udp* v4loop;
extern sarnet::udp* v6loop;
extern sarnet::udp* v4mcastin;
extern sarnet::udp* v6mcastin;

// Saratoga Log file
extern sarfile::fileio* sarlog;

// Current Zulu time - Used for timers
extern saratoga::timestamp curzulu;

// Currently open list of local file descriptors
extern sarfile::files sarfiles;
// Currently open list of UDP sockets
extern sarnet::peers sarpeers;
// Current information on known peers (from beacons)
extern saratoga::peersinfo sarpeersinfo;
// Current transfers in progress
extern saratoga::transfers sartransfers;

// Functions in globals.cpp
extern int maxfd();
extern string printhex(const char*, const size_t);
extern string printhex(const string);
extern string printflags(string, flag_t);
extern void splitargs(string, std::vector<string>&);
extern void readconf(string);
extern void writeconf(string);
extern unsigned int asciitoint(const char*, const int);

// Timer for beacons - every n secs
extern timer_group::timer beacontimer;

}; // namespace saratoga

#endif // _GLOBALS_H
