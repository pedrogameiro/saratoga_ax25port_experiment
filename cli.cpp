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

// Command Line Interpretor for saratoga

// #include <iostream>
#include "cli.h"
#include "beacon.h"
#include "fileio.h"
#include "globals.h"
#include "ip.h"
#include "request.h"
#include "sarflags.h"
#include "screen.h"
#include "timestamp.h"
#include <cstring>
#include <string>
#include <sys/types.h>
#include <vector>

using namespace std;

// A command has a name, a usage a help and a function to call

namespace saratoga {

// The command line interface flag classes
// these flags and variables are used to set the protocol flags
// default values and running state
// We have a class for each major cli keyword
// The handler is cmd::cmd_keyword
// The cli flags are cli_keyword

// Make sure we have an unsigned int string for stoi conversions
bool
isuint(string s)
{
  for (size_t i = 0; i < s.length(); i++)
    if (s[i] < '0' || s[i] > '9')
      return (false);
  return (true);
}

// ********************************************************************************
// * Command line handlers
// ********************************************************************************

bool
cmd::cmd_help()
{
  cmds c;

  // Print out the full command list
  scr.msg(c.print());
  return (true);
}

bool
cmd::cmd_exit()
{

  std::string::size_type sz; // needed for stoi

  if (_args.size() > 2) {
    scr.error("ERROR: exit - Invalid number of arguments");
    return (false);
  }

  if (_args.size() == 1) {
    scr.info("Goodbye");
    c_exit.flag(0);
    return (true);
  }
  if (_args.size() == 2) {
    int x = std::stoi(_args[1], &sz);
    if (x == 0) {
      // Exit gracefully
      c_exit.flag(0);
      return (true);
    }
    if (x == 1) {
      // Exit with extreme prejudice
      c_exit.flag(1);
      return (true);
    }
    scr.error("ERROR: exit - Invalid argument");
    c_exit.flag(-1);
    return (false);
  }
  // We never get here
  return (false);
}

// Set cli flags for beacons
// return whether to run the beacons or not
bool
cmd::cmd_beacon()
{
  cmds c;
  string s;

  std::string::size_type sz; // needed for stoi
  string outstr;
  // Show current beacon flag settings
  if (_args.size() == 1) {
    // Send a single beacon
    if (!c_beacon.ready()) {
      scr.info("No destination for beacons set");
      return (true);
    }
    s = "Sending beacons to " + c_beacon.print();
    scr.info(s);
    return (true);
  }
  // Show usage
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("beacon"));
    return (true);
  }
  // Turn all flags for beacons off
  if (_args.size() == 2 && _args[1] == "off") {
    // Just clear beacon flags
    c_beacon.v4flag(false);
    c_beacon.v6flag(false);
    c_timer.beacon(0);
    c_beacon.zap();
    scr.info("Beacons off");
    beacontimer = c_timer.beacon(); // mseconds
    return (true);
  }

  size_t startpos = 1;
  bool flagset = true;
  if (_args[1] == "off") {
    flagset = false;
    startpos = 2;
  }
  for (size_t i = startpos; i < _args.size(); i++) {
    // We send v4 broadcast beacon
    if (_args[i] == "v4") {
      c_beacon.v4flag(flagset);
      continue;
    }
    // We send v6 broadcast beacon
    if (_args[i] == "v6") {
      c_beacon.v6flag(flagset);
      continue;
    }
    // Send the beacon every x msecs
    if (isuint(_args[i])) {
      if (flagset == false)
        c_timer.beacon(0);
      else
        c_timer.beacon(std::stoi(_args[i], &sz));
      beacontimer = c_timer.beacon();
      continue;
    }
    // We send unicast beacon to ip address's
    sarnet::ip isip(_args[i]);
    if (isip.isv4() || isip.isv6()) {
      if (flagset == false)
        c_beacon.iprm(_args[i]);
      else {
        // Create the global peer and beacon peer
        sarpeers.add(_args[i], sarport);
        c_beacon.ipadd(_args[i]);
      }
      continue;
    }
  }
  s = "Beacon Flags: " + c_beacon.print();
  scr.info(s);
  return (true);
}

bool
cmd::cmd_checksum()
{
  cmds c;

  if (_args.size() == 1) {
    scr.info(c_checksum.print());
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("checksum"));
    return (true);
  }
  if (_args.size() == 2) {
    if (_args[1] == "off" || _args[1] == "none")
      c_checksum.flag(F_CSUM_NONE);
    else if (_args[1] == "crc32")
      c_checksum.flag(F_CSUM_CRC32);
    else if (_args[1] == "md5")
      c_checksum.flag(F_CSUM_MD5);
    else if (_args[1] == "sha1")
      c_checksum.flag(F_CSUM_SHA1);
    else {
      scr.error("Invalid Checksum Type");
      return (false);
    }
  }
  scr.info(c_checksum.print());
  return (true);
}

bool
cmd::cmd_debug()
{
  string s;
  cmds c;

  // We only expect a single argument
  if (_args.size() == 1) {
    scr.info(c_debug.print());
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("debug"));
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "off") {
    c_debug.level(0);
    scr.info(c_debug.print());
    return (true);
  }
  if (_args.size() == 2) {
    if (_args[1] == "0" || _args[1] == "1" || _args[1] == "2" ||
        _args[1] == "3" || _args[1] == "4" || _args[1] == "5" ||
        _args[1] == "6" || _args[1] == "7" || _args[1] == "8" ||
        _args[1] == "9")
      c_debug.level(_args[1][0] - '0');
    else {
      scr.error("Invalid Debug Level must be 0-9");
      scr.std(c.usage("debug"));
      return (true);
    }
  }
  scr.info(c_debug.print());
  return (true);
}

bool
cmd::cmd_descriptor()
{
  cmds c;

  extern enum f_descriptor
  maxdescriptor(); // Maximim descriptor for largest file size
  if (_args.size() == 1) {
    scr.info(c_descriptor.print());
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("descriptor"));
    return (true);
  }
  if (_args.size() == 2) {
    if (_args[1] == "off")
      c_descriptor.flag(maxdescriptor()); // Reset to largest file size
    else if (_args[1] == "16")
      c_descriptor.flag(F_DESCRIPTOR_16);
    else if (_args[1] == "32")
      c_descriptor.flag(F_DESCRIPTOR_32);
    else if (_args[1] == "64")
      c_descriptor.flag(F_DESCRIPTOR_64);
    else if (_args[1] == "128") {
      scr.error("128bit  Descriptors not supported");
      c_descriptor.flag(F_DESCRIPTOR_64);
    } else {
      scr.error("Invalid Descriptor Type");
      return (false);
    }
  }
  scr.info(c_descriptor.print());
  return (true);
}

// The default EID is the Process ID
// When we send it out on a beacon
// it is always prepended by the local IP
// Address of the sender
// If we set the EID manually it is still
// prepended by the calling IP address (in text)
bool
cmd::cmd_eid()
{
  string s;
  cmds c;

  // We only expect a single argument
  if (_args.size() == 1) {
    s = "EID: " + c_eid.print();
    scr.info(s);
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("eid"));
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "off") {
    string strpid;
    int pid;
    char str[128];
    pid = getpid();
    sprintf(str, "%d", pid);
    strpid = str;
    c_eid.eid(strpid);
    s = "EID: " + c_eid.print();
    scr.info(s);
    return (true);
  }
  if (_args.size() == 2) {
    c_eid.eid(_args[1]);
  }
  s = "EID: " + c_eid.print();
  scr.info(s);
  return (true);
}

// List current local files that are open and mode
bool
cmd::cmd_files()
{
  string s = "";
  cmds c;

  std::string::size_type sz; // needed for stoi
  // Show current peer flag settings
  if (_args.size() == 1) {
    for (std::list<saratoga::tran>::iterator tr = sartransfers.begin();
         tr != sartransfers.end(); tr++) {
      sarfile::fileio* fp = &(*tr->local());
      s += fp->print();
      s += "\n";
    }
    if (s == "")
      scr.info("No Open Files");
    else
      scr.info(s);
    return (true);
  }
  // Show usage
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("files"));
    return (true);
  }
  return (true);
}

bool
cmd::cmd_freespace()
{
  cmds c;

  // We only expect a single argument
  if (_args.size() == 1) {
    scr.info(c_freespace.print());
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("freespace"));
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "on") {
    c_freespace.flag(F_FREESPACE_YES);
    scr.info(c_freespace.print());
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "off") {
    c_freespace.flag(F_FREESPACE_NO);
    scr.info(c_freespace.print());
    return (true);
  }
  scr.info(c.usage("freespace"));
  return (false);
}

bool
cmd::cmd_get()
{
  cmds c;

  c_get.ready(false);
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("get"));
    return (true);
  }
  if (_args.size() != 3) {
    scr.info(c.usage("get"));
    return (true);
  }
  sarnet::ip isip(_args[1]);
  if (isip.isv4() || isip.isv6()) {
    c_get.peer(isip);
    c_get.fname(_args[2]);
    c_get.ready(true);
    return (true);
  }
  c_get.zap();
  c_get.fname("");
  scr.info(c.usage("get"));
  return (true);
}

bool
cmd::cmd_getrm()
{
  cmds c;

  c_getrm.ready(false);
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("getrm"));
    return (true);
  }
  if (_args.size() != 3) {
    scr.info(c.usage("getrm"));
    return (true);
  }
  sarnet::ip isip(_args[1]);
  if (isip.isv4() || isip.isv6()) {
    c_getrm.peer(isip);
    c_getrm.fname(_args[2]);
    c_getrm.ready(true);
    return (true);
  }
  c_getrm.zap();
  c_getrm.fname("");
  scr.info(c.usage("getrm"));
  return (true);
}

bool
cmd::cmd_history()
{
  cmds c;

  if (_args.size() == 1) {
    scr.info(c_history.print());
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "?")
    ;
  else
    scr.error("Invalid history command usage");
  scr.info(c.usage("history"));
  return (true);
}

bool
cmd::cmd_home()
{
  string s;
  cmds c;

  // We only expect a single argument
  if (_args.size() == 1) {
    s = "HOMEDIR: " + c_home.print();
    scr.info(s);
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("home"));
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "off") {
    c_home.fname(".");
    return (true);
  }
  if (_args.size() == 2) {
    sarfile::fileio fp(_args[1], sarfile::FILE_READ);
    if (fp.ok() && fp.isdir()) {
      // Write some code to make sure dir exists
      c_home.fname(_args[1]);
      s = "HOMEDIR: " + c_home.print();
      scr.info(s);
      return (true);
    } else {
      s = "Directory does not exist: ";
      s += _args[1];
      scr.error(s);
      c_home.fname(".");
      return (true);
    }
  }
  return (true);
}

bool
cmd::cmd_ls()
{
  cmds c;

  c_ls.ready(false);
  if (_args.size() == 1 || (_args.size() == 2 && _args[1] == "?")) {
    scr.info(c.usage("ls"));
    return (true);
  }
  if (_args.size() == 2) {
    c_ls.dname(".");
  }
  if (_args.size() == 3) {
    c_ls.dname(_args[2]);
  }
  sarnet::ip isip(_args[1]);
  if (isip.isv4() || isip.isv6()) {
    c_ls.peer(isip);
    c_ls.ready(true);
    return (true);
  }
  c_ls.zap();
  c_ls.dname("");
  scr.info(c.usage("ls"));
  return (true);
}

bool
cmd::cmd_maxbuff()
{
  cmds c;

  std::string::size_type sz; // needed for stoi
  // We only expect a single argument
  if (_args.size() == 1) {
    scr.info(c_maxbuff.print());
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("maxbuff"));
    return (true);
  }
  if (_args.size() == 2 && isuint(_args[1])) {
    size_t tmp = (size_t)std::stoi(_args[1], &sz);
    if (tmp > 0)
      c_maxbuff.set((size_t)std::stoi(_args[1], &sz));
    scr.info(c_maxbuff.print());
    return (true);
  }
  scr.info(c.usage("maxbuff"));
  return (false);
}

bool
cmd::cmd_pinfo()
{
  string s;
  cmds c;

  // Show current peer flag settings
  if (_args.size() == 1) {
    string ps = sarpeersinfo.print();
    if (ps == "")
      scr.info("No Peer Information");
    else {
      s = ps;
      scr.info(s);
    }
    return (true);
  }
  // Show usage
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("peerinfo"));
    return (true);
  }
  s = sarpeersinfo.print();
  scr.info(s);
  return (true);
}

bool
cmd::cmd_peers()
{
  string s;
  cmds c;

  std::string::size_type sz; // needed for stoi
  string outstr;
  // Show current peer flag settings
  if (_args.size() == 1) {
    string ps = sarpeers.print();
    if (ps == "")
      scr.info("No Peers");
    else {
      s = ps;
      scr.info(s);
    }
    return (true);
  }
  // Show usage
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("peers"));
    return (true);
  }
  // Remove all peers
  if (_args.size() == 2 && _args[1] == "off") {
    // Just clear all the current peers
    // And stop sending beacons
    sarpeers.zap();
    c_beacon.zap();
    return (true);
  }

  size_t startpos = 1;
  bool flagset = true;
  if (_args[1] == "remove") {
    flagset = false;
    startpos = 2;
  }
  for (size_t i = startpos; i < _args.size(); i++) {
    // We clear peers we have listed
    sarnet::ip isip(_args[i]);
    if (isip.isv4() || isip.isv6()) {
      if (flagset == false) {
        // Remove peer and stop sending beacons to it
        s = "Removing Peer: " + _args[i];
        scr.info(s);
        sarpeers.remove(_args[i], sarport);
        c_beacon.iprm(_args[i]);
      } else {
        // When we add a peer we start sending beacons as well to it
        sarpeers.add(_args[i], sarport);
        c_beacon.ipadd(_args[i]);
      }
      continue;
    }
    s = "peers - Invalid IP: " + isip.print();
    scr.error(s);
  }
  s = "Current Peers:\n" + sarpeers.print();
  scr.info(s);
  return (true);
}

bool
cmd::cmd_put()
{
  cmds c;

  c_put.ready(false);
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("put"));
    return (true);
  }
  if (_args.size() != 3) {
    scr.info(c.usage("put"));
    return (true);
  }
  sarnet::ip isip(_args[1]);
  if (isip.isv4() || isip.isv6() || isip.family() == AF_AX25) {
    string s = isip.straddr();
    scr.debug(2, "cmd::cmd_put() We have a put to %s", s.c_str());
    c_put.peer(isip);
    sarnet::ip* tmp = c_put.peer();
    string s1 = tmp->straddr();
    scr.debug(2, "cmd::cmd_put() After Copy We have a put to %s", s1.c_str());
    c_put.fname(_args[2]);
    // If our first character is / or ./ then local file name is absolute
    // If not then it is within the c_home directory
    if (_args[2].find("/") == 0 || _args[2].find("./") == 0)
      c_put.localfname(_args[2]);
    else
      c_put.localfname(c_home.dir() + "/" + _args[2]);
    c_put.ready(true);
    return (true);
  }
  c_put.zap();
  scr.info(c.usage("put"));
  return (true);
}

bool
cmd::cmd_putrm()
{
  cmds c;

  c_putrm.ready(false);
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("putrm"));
    return (true);
  }
  if (_args.size() != 3) {
    scr.info(c.usage("putrm"));
    return (true);
  }
  sarnet::ip isip(_args[1]);
  if (isip.isv4() || isip.isv6()) {
    c_putrm.peer(isip);
    c_putrm.fname(_args[2]);
    c_putrm.ready(true);
    return (true);
  }
  c_putrm.zap();
  c_putrm.fname("");
  scr.info(c.usage("putrm"));
  return (true);
}

bool
cmd::cmd_rm()
{
  cmds c;

  c_rm.ready(false);
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("rm"));
    return (true);
  }
  if (_args.size() != 3) {
    scr.info(c.usage("rm"));
    return (true);
  }
  sarnet::ip isip(_args[1]);
  if (isip.isv4() || isip.isv6()) {
    c_rm.peer(isip);
    c_rm.fname(_args[2]);
    c_rm.ready(true);
    return (true);
  }
  scr.info(c.usage("rm"));
  return (true);
}

bool
cmd::cmd_rmdir()
{
  cmds c;

  c_rmdir.ready(false);
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("rmdir"));
    return (true);
  }
  if (_args.size() != 3) {
    scr.info(c.usage("rmdir"));
    return (true);
  }
  sarnet::ip isip(_args[1]);
  if (isip.isv4() || isip.isv6()) {
    c_rmdir.peer(isip);
    c_rmdir.dname(_args[2]);
    c_rmdir.ready(true);
    return (true);
  }
  scr.info(c.usage("rmdir"));
  return (true);
}

bool
cmd::cmd_rx()
{
  cmds c;

  // We only expect a single argument
  if (_args.size() == 1) {
    scr.info(c_rx.print());
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("rx"));
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "on") {
    c_rx.flag(F_RXWILLING_YES);
    scr.info(c_rx.print());
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "off") {
    c_rx.flag(F_RXWILLING_NO);
    scr.info(c_rx.print());
    return (true);
  }
  scr.info(c.usage("rx"));
  return (false);
}

bool
cmd::cmd_session()
{
  cmds c;

  std::string::size_type sz; // needed for stoi
  // We only expect a single argument
  if (_args.size() == 1) {
    scr.info(c_session.print());
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("session"));
    return (true);
  }
  if (_args.size() == 2 && isuint(_args[1])) {
    c_session.set((session_t)std::stoi(_args[1], &sz));
    scr.info(c_session.print());
    return (true);
  }
  scr.info(c.usage("session"));
  return (false);
}

bool
cmd::cmd_stream()
{
  string s;
  cmds c;

  // We only expect a single argument
  if (_args.size() == 1) {
    scr.info(c_stream.print());
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("stream"));
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "on") {
    c_stream.flag(F_STREAMS_NO);
    s = "Streams not supported " + c_stream.print();
    scr.error(s);
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "off") {
    c_stream.flag(F_STREAMS_NO);
    scr.info(c_stream.print());
    return (true);
  }
  scr.info(c.usage("stream"));
  return (false);
}

bool
cmd::cmd_timezone()
{
  cmds c;
  if (_args.size() == 1) {
    scr.info(c_timezone.print());
    return (true);
  }
  if (_args[1] == "?" || _args.size() > 2) {
    scr.info(c.usage("timezone"));
    return (true);
  }
  if (_args[1] == "utc" || _args[1] == "UTC") {
    c_timezone.tz(TZ_UTC);
    scr.info(c_timezone.print());
    return (true);
  }
  if (_args[1] == "local" || _args[1] == "LOCAL") {
    c_timezone.tz(TZ_LOCAL);
    scr.info(c_timezone.print());
    return (true);
  }
  scr.info(c.usage("timezone"));
  return (false);
}

/*
 * Set/Reset various timers
 * Currently Supported: and all are seconds for each initated transfer
 *	request - The timeout for expected response from a REQUEST
 *		if this timeout is exceeded then the REQUEST is resent
 *	transfer - The timeout to terminate the transfer if there is no
 *		packet received.
 *	status - The timeout to resend a STATUS
 *	beacon - Timer for a beacon
 */
bool
cmd::cmd_timer()
{
  cmds c;
  std::string::size_type sz; // needed for stoi

  if (_args.size() == 1) {
    scr.info(c_timer.print());
    return (true);
  }
  if (_args[1] == "?" || _args.size() > 3) {
    scr.info(c.usage("timer"));
    return (true);
  }

  if (_args.size() == 2) {
    if (_args[1] == "off") {
      // Turn off timers
      c_timer.request(c_timer.def());
      c_timer.transfer(c_timer.def());
      c_timer.status(c_timer.def());
      c_timer.framedelay(0);
      scr.info(c_timer.print());
      return (true);
    }
    if (_args[1] == "request") {
      scr.info(c_timer.print_request());
      return (true);
    }
    if (_args[1] == "transfer") {
      scr.info(c_timer.print_transfer());
      return (true);
    }
    if (_args[1] == "status") {
      scr.info(c_timer.print_status());
      return (true);
    }
    if (_args[1] == "beacon") {
      scr.info(c_timer.print_beacon());
      return (true);
    }
    if (_args[1] == "framedelay") {
      scr.info(c_timer.print_framedelay());
      return (true);
    }
  }

  if (_args.size() == 3) {
    if (_args[1] == "request" && _args[2] == "off") {
      c_timer.request(c_timer.def());
      scr.info(c_timer.print_request());
      return (true);
    }
    if (_args[1] == "transfer" && _args[2] == "off") {
      c_timer.transfer(c_timer.def());
      scr.info(c_timer.print_transfer());
      return (true);
    }
    if (_args[1] == "status" && _args[2] == "off") {
      c_timer.status(c_timer.def());
      scr.info(c_timer.print_status());
      return (true);
    }
    if (_args[1] == "framedelay" && _args[2] == "off") {
      c_timer.framedelay(0);
      scr.info(c_timer.print_framedelay());
      return (true);
    }
    if (_args[1] == "beacon" && _args[2] == "off") {
      c_timer.beacon(0);
      scr.info(c_timer.print_beacon());
      beacontimer = c_timer.beacon();
      return (true);
    }

    if (_args[1] == "request" && isuint(_args[2])) {
      c_timer.request((offset_t)std::stoi(_args[2], &sz));
      scr.info(c_timer.print_request());
      return (true);
    }
    if (_args[1] == "transfer" && isuint(_args[2])) {
      c_timer.transfer((offset_t)std::stoi(_args[2], &sz));
      scr.info(c_timer.print_transfer());
      return (true);
    }
    if (_args[1] == "status" && isuint(_args[2])) {
      c_timer.status((offset_t)std::stoi(_args[2], &sz));
      scr.info(c_timer.print_status());
      return (true);
    }
    if (_args[1] == "framedelay" && isuint(_args[2])) {
      c_timer.framedelay((offset_t)std::stoi(_args[2], &sz));
      scr.info(c_timer.print_framedelay());
      return (true);
    }
    if (_args[1] == "beacon" && isuint(_args[2])) {
      c_timer.beacon((offset_t)std::stoi(_args[2], &sz));
      scr.info(c_timer.print_beacon());
      beacontimer = c_timer.beacon();
      return (true);
    }
  }
  scr.info(c.usage("timer"));
  return (true);
}

bool
cmd::cmd_timestamp()
{
  cmds c;

  if (_args.size() == 1) {
    scr.info(c_timestamp.print());
    return (true);
  }
  if (_args[1] == "?" || _args.size() != 2) {
    scr.info(c.usage("timestamp"));
    return (true);
  }
  if (_args[1] == "off") {
    c_timestamp.flag(F_TIMESTAMP_NO);
    // UDEF is not supported
    c_timestamp.ttype(T_TSTAMP_UDEF);
  } else if (_args[1] == "32") {
    c_timestamp.flag(F_TIMESTAMP_YES);
    c_timestamp.ttype(T_TSTAMP_32);
  } else if (_args[1] == "64") {
    c_timestamp.flag(F_TIMESTAMP_YES);
    c_timestamp.ttype(T_TSTAMP_64);
  } else if (_args[1] == "32_32") {
    c_timestamp.flag(F_TIMESTAMP_YES);
    c_timestamp.ttype(T_TSTAMP_32_32);
  } else if (_args[1] == "64_32") {
    c_timestamp.flag(F_TIMESTAMP_YES);
    c_timestamp.ttype(T_TSTAMP_64_32);
  } else if (_args[1] == "32_y2k") {
    c_timestamp.flag(F_TIMESTAMP_YES);
    c_timestamp.ttype(T_TSTAMP_32_Y2K);
  } else {
    scr.info(c.usage("timestamp"));
    return (true);
  }
  scr.info(c_timestamp.print());
  return (true);
}

bool
cmd::cmd_transfers()
{
  cmds c;

  if (_args.size() == 1) {
    scr.info(c_transfers.print());
    return (true);
  }
  scr.info(c.usage("transfers"));
  return (true);
}

bool
cmd::cmd_tx()
{
  cmds c;

  if (_args.size() == 1) {
    scr.info(c_tx.print());
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("tx"));
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "on") {
    c_tx.flag(F_TXWILLING_YES);
    scr.info(c_tx.print());
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "off") {
    c_tx.flag(F_TXWILLING_NO);
    scr.info(c_tx.print());
    return (true);
  }
  scr.info(c.usage("tx"));
  return (false);
}

bool
cmd::cmd_prompt()
{
  string s;
  cmds c;

  // We only expect a single argument
  if (_args.size() == 1) {
    c_prompt.prompt("saratoga: ");
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "?") {
    scr.info(c.usage("prompt"));
    return (true);
  }
  if (_args.size() == 2 && _args[1] == "off") {
    c_prompt.prompt("saratoga: ");
    return (true);
  }
  if (_args.size() == 2) {
    string p = _args[1];
    p += ": ";
    c_prompt.prompt(p);
  }
  return (true);
}

// *******************************************************************************

string
cmds::usage(string c)
{
  for (int i = 0; i < _ncmds; i++)
    if (_clist[i] == c)
      return _clist[i].usage();
  return "";
}

string
cmds::cmdstr(string c)
{
  for (int i = 0; i < _ncmds; i++)
    if (_clist[i] == c)
      return _clist[i].name();
  return "";
}

void
cmd::setargs(vector<string> argv)
{
  for (size_t i = 0; i < argv.size(); i++)
    _args.push_back(argv[i]);
}

void
cmd::delargs()
{
  if (_args.size() > 0)
    _args.erase(_args.begin(), _args.end());
}

bool
cmds::runcmd(vector<string> argv)
{
  for (int i = 0; i < _ncmds; i++)
    if (_clist[i] == argv[0]) {
      _clist[i].delargs();      // delete old
      _clist[i].setargs(argv);  // add new
      return (_clist[i].run()); // run it
    }
  return (false);
}

string
cmds::print()
{
  string s = "";
  for (int i = 0; i < _ncmds; i++) {
    s += _clist[i].name();
    s += ": ";
    s += _clist[i].help();
    s += '\n';
  }
  return s;
}

string
cmds::printusage(string c)
{
  string s = "";
  for (int i = 0; i < _ncmds; i++) {
    if (c == _clist[i].name())
      return (_clist[i].usage());
  }
  s = c;
  s += ": no such command";
  return s;
}

// Return a matching unique cammand to first chars in s
string
cmds::cmatch(const string s)
{

  string match = "";
  int matches = 0;
  int cmp = 0;

  scr.debug(9, "cmds::cmatch() Looking for cmatch <%s>", s.c_str());
  for (int i = 0; i < _ncmds; i++) {
    string name = _clist[i].name();
    if ((cmp = name.compare(0, s.length(), s)) == 0) {
      matches++;
      match = name;
    }
  }
  if (matches != 1)
    return ("");
  scr.debug(9, "cmds::cmatch() cmatch <%s> == <%s> %d", s.c_str(),
            match.c_str(), matches);
  return (match);
}

string
cmds::cmdmatches(const string s)
{
  string matchlist = "";
  for (int i = 0; i < _ncmds; i++) {
    string name = _clist[i].name();
    if ((name.compare(0, s.length(), s)) == 0) {
      matchlist += name;
      matchlist += ' ';
    }
  }
  return (matchlist);
}

/*
 *********************************************************************************
 */

string
cli_beacon::print()
{
  string outstr = "";
  if (this->v4flag())
    outstr += "IPv4 ";
  if (this->v6flag())
    outstr += "IPv6 ";
  for (std::list<sarnet::ip>::iterator i = begin(); i != end(); i++) {
    outstr += i->print();
    outstr += " ";
  }
  if (c_timer.beacon() > 0 && this->ready()) {
    char tmp[64];
    sprintf(tmp, "every %d ms", (int)c_timer.beacon());
    outstr += tmp;
  }
  if (outstr.length() > 0)
    return (outstr);
  else
    return ("Beacons Off");
}

string
cli_checksum::print()
{
  Fcsumtype f(this->flag());

  return (f.print());
}

string
cli_debug::print()
{
  char tmp[16];

  sprintf(tmp, "Debug Level %d", (int)this->level());
  string s = tmp;
  return (s);
}

string
cli_descriptor::print()
{
  Fdescriptor f(this->flag());

  return (f.print());
}

string
cli_eid::print()
{
  if (this->eid() == "")
    return ("EID Not Set");
  return (this->eid());
}

string
cli_freespace::print()
{
  Ffreespace f(this->flag());

  return (f.print());
}

string
cli_history::get(unsigned int h)
{
  unsigned int cnt = 1;

  if (h == 0 || _history.empty() || h > _history.size())
    return "";
  for (std::list<string>::iterator hist = _history.begin();
       hist != _history.end(); hist++) {
    string retstr = *hist;
    if (cnt >= h)
      return retstr;
    ++cnt;
  }
  return "";
}

string
cli_history::print()
{
  string s = "";
  int cnt = 0;

  for (std::list<string>::iterator hist = _history.begin();
       hist != _history.end(); hist++) {
    char tmp[256];
    string val = *hist;
    if (val != "") {
      cnt++;
      sprintf(tmp, "[%02d] %s\n", cnt, val.c_str());
      s += tmp;
    }
  }
  return s;
}

string
cli_maxbuff::print()
{
  char tmp[128];
  string s;

  sprintf(tmp, "Maxbuff %" PRIu32 "", (uint32_t)_maxbuff);
  s = tmp;
  return (s);
}

string
cli_rx::print()
{
  Frxwilling f(this->flag());

  return (f.print());
}

string
cli_stream::print()
{
  Fstream f(this->flag());

  return (f.print());
}

string
cli_tx::print()
{
  Ftxwilling f(this->flag());

  return (f.print());
}

string
cli_timer::print()
{
  string s;

  s = this->print_framedelay();
  s += '\n';
  s += this->print_request();
  s += '\n';
  s += this->print_transfer();
  s += '\n';
  s += this->print_status();
  s += '\n';
  s += this->print_beacon();
  s += '\n';
  return s;
}

string
cli_timer::print_request()
{
  char tmp[128];
  string s;

  sprintf(tmp, "Request Timeout %" PRIu64 " ms", (uint64_t)_request);
  s = tmp;
  return (s);
}

string
cli_timer::print_transfer()
{
  char tmp[128];
  string s;

  sprintf(tmp, "Transfer Timeout %" PRIu64 " s", (uint64_t)_transfer / 1000);
  s = tmp;
  return (s);
}

string
cli_timer::print_status()
{
  char tmp[128];
  string s;

  sprintf(tmp, "Status Timer %" PRIu64 " ms", (uint64_t)_status);
  s = tmp;
  return (s);
}

string
cli_timer::print_framedelay()
{
  char tmp[128];
  string s;

  sprintf(tmp, "UDP Frame Buffer Tx Delay %" PRIu64 " ms",
          (uint64_t)_framedelay);
  s = tmp;
  return (s);
}

string
cli_timer::print_beacon()
{
  char tmp[128];
  string s;

  sprintf(tmp, "Beacon Timer %" PRIu64 " ms", (uint64_t)_beacon);
  s = tmp;
  return (s);
}

string
cli_session::print()
{
  char tmp[128];
  string s;

  sprintf(tmp, "Session %" PRIu32 "", (uint32_t)_session);
  s = tmp;
  return (s);
}

string
cli_timestamp::print()
{
  Ttimestamp f(this->ttype());

  if (this->flag() == F_TIMESTAMP_NO)
    return ("Timestamps Disabled");
  return (f.print());
}

string
cli_timezone::print()
{
  if (this->tz() == TZ_UTC)
    return ("UTC");
  if (this->tz() == TZ_LOCAL)
    return ("LOCAL");
  return "Invalid Time Zone";
}

string
cli_transfers::print()
{
  return (sartransfers.print());
}

string
cli_multicast::print()
{
  if (this->state() == true)
    return ("Multicast Enabled");
  return ("Multicast Disabled");
}

string
cli_prompt::print()
{
  return (_prompt);
}

}; // namespace saratoga
