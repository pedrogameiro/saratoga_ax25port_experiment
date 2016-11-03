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
#include <iostream>
#include <limits>
#include <list>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#include "frame.h"
#include "globals.h"
#include "ip.h"

using namespace std;
using namespace sarnet;

// Test out the ip stuff
#ifdef TEST3
int
main(int argc, char** argv)
{
  sarwin::screen scr;
  c_debug.level(9);
  sarnet::netifaces interfaces;

  sarnet::udp* v4in = new sarnet::udp(AF_INET, 1234);
  sarnet::udp* v6in = new sarnet::udp(AF_INET6, 1234);
  scr.msg("UDP V4IN=%s", v4in->print().c_str());
  scr.msg("UDP V6IN=%s", v6in->print().c_str());

  sarnet::netiface* v4first = interfaces.first(AF_INET);
  sarnet::ip* fv4 = new sarnet::ip(v4first->ifip());
  sarnet::netiface* v6first = interfaces.first(AF_INET6);
  sarnet::ip* fv6 = new sarnet::ip(v6first->ifip());
  scr.msg("IP FIRST V4OUT=%s", fv4->print().c_str());
  scr.msg("IP FIRST V6OUT=%s", fv6->print().c_str());

  sarnet::udp* v4out = new sarnet::udp(fv4->straddr(), 1234);
  scr.msg("UDP V4OUT=%s", v4out->print().c_str());
  sarnet::udp* v6out = new sarnet::udp(fv6->straddr(), 1234);
  scr.msg("UDP V6OUT=%s", v6out->print().c_str());

  sarnet::udp* v4mo = new sarnet::udp(sarnet::MCAST_OUT, v4out, if_mcast, 1234);
  scr.msg("UDP MCASTV4OUT=%s", v4mo->print().c_str());
  sarnet::udp* v6mo =
    new sarnet::udp(sarnet::MCAST_OUT, v6out, if6_mcast, 1234);
  scr.msg("UDP MCASTV6OUT=%s", v6mo->print().c_str());

  sarnet::udp* v4mi = new sarnet::udp(sarnet::MCAST_IN, v4in, if_mcast, 1234);
  scr.msg("UDP MCASTV4in=%s", v4mi->print().c_str());
  sarnet::udp* v6mi = new sarnet::udp(sarnet::MCAST_IN, v6in, if6_mcast, 1234);
  scr.msg("UDP MCASTV6in=%s", v6mi->print().c_str());

  sleep(10);
}
#endif
