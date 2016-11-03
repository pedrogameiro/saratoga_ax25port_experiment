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

#ifndef _SCREEN_H
#define _SCREEN_H

#include <errno.h>
#include <ncurses.h>
#include <stdio.h>
#include <string>
//#include <ncurses.h>
#include "cli.h"
#include "saratoga.h"

using namespace std;
using namespace saratoga;

// The color pairs
static const short RED_BLACK = 1;
static const short GREEN_BLACK = 2;
static const short YELLOW_BLACK = 3;
static const short BLUE_BLACK = 4;
static const short MAGENTA_BLACK = 5;
static const short CYAN_BLACK = 6;
static const short WHITE_BLACK = 7;
static const short BLACK_RED = 8;
static const short BLACK_GREEN = 9;
static const short BLACK_YELLOW = 10;
static const short BLACK_BLUE = 11;
static const short BLACK_MAGENTA = 12;
static const short BLACK_CYAN = 13;
static const short BLACK_WHITE = 14;

/*
 * Handle debug and error output.
 * Flexible as you can set the level and also what file you want to talk to
 * defaults are stderr of course. When setting a new fp make sure we dont close
 * stdout and stderr
 */

namespace sarwin {

extern void initwindows();
extern void endwindows();

class screen
{
private:
  WINDOW* errwin;         // Subwindow of errbox
  WINDOW* errbox;         // Subwindow of stdwin
  WINDOW* cmdwin;         // Subwindow of cmdbox
  WINDOW* cmdbox;         // Subwindow of stdwin
  FILE* _fp;              // File debug output
  int _first = true;      // Only run this once to start up ncurses
  unsigned int _cmdwidth; // # width of cmdwin
  unsigned int _errwidth; // # width of errwin

  // Height Ratio of the size of the command vs error windows
  // Here the error window is 4 times longer than the command window
  // Change to suit your preference at will.
  static const int ratio = 4;

public:
  screen();              // Create a ncurses screen
  screen(const string&); // Create screen and open debug file
  ~screen();             // Close em all down

  // Prntwindow id the only curses dependednt dunction here. All of the
  // others call it to do some form of output.
  // Change this and you can replace ncurses with any gui you like
  void printwindow(WINDOW*, const char*, short);

  void std(char c);
  void std(const string& fmt, ...);
  void stdnonl(const string& fmt, ...);
  void prompt();
  void info(const string& fmt, ...);
  void msg(const string& fmt, ...);
  void msgnonl(const string& fmt, ...);
  void msgout(const string& fmt, ...);
  void msgin(const string& fmt, ...);
  void debug(int lev, const string& fmt, ...);
  void error(const string& fmt, ...);
  void warning(const string& fmt, ...);
  void perror(int err, const string& fmt, ...);
  void fatal(const string& fmt, ...);
  void resize();
  void larrow(const string&);
  void rarrow(const string&);
  void darrow(); // Clear command
  string uarrow();
  void moveeol(const string&);     // Move to the end of the string + prompt
  string backspace(const string&); // Backspace handler
  string insertkey(const string&, char); // Character insert handler
  unsigned int cmdwidth() { return _cmdwidth; };
  unsigned int errwidth() { return _errwidth; };
};

}; // namespace sarwin

#endif // _SCREEN_H
