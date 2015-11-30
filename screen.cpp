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
 * Handle formatted (printf style) error and debug outputs
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <cstring>

#include "saratoga.h"
#include "cli.h"
#include "screen.h"
#include "globals.h"


using namespace std;

namespace sarwin
{

// Draw a box for a window
// This is actually a subwindow of stdscr
// We have to do this because we are using stdscr for getch()
// otherwise the box gets blown away as soon as you call getch()
WINDOW *
create_box(int height, int width, int starty, int startx)
{
	WINDOW	*box_win;

	box_win = subwin(stdscr, height, width, starty, startx);
	wattron(box_win, COLOR_PAIR(MAGENTA_BLACK));
	box(box_win, 0, 0);
	wrefresh(box_win);
	wattroff(box_win, COLOR_PAIR(MAGENTA_BLACK));
	return(box_win);
}

// Now create a sub-window inside that box that was drawn
WINDOW *
create_subwin(WINDOW *box, int height, int width, int starty, int startx)
{
	WINDOW	*my_win;

	my_win = subwin(box, height - 2, width - 2, starty + 1, startx + 1);
	wrefresh(my_win);
	return(my_win);
}

// Zap the box and window
void
destroy_win(WINDOW *local_win)
{
	wborder(local_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
	wrefresh(local_win);
	delwin(local_win);
}

// Send a string to a window and wrap the text as required 
// Change the output colour to what is specified and wrefresh
// Write waht we have sent to the screen to the log file and timestamp it
void
screen::printwindow(WINDOW *w, const char *s, short colorpair)
{
	// We send all errwin to the log file
	// We have to make sure that we have the log file
	// opered so we can write to it
	if (w == errwin && sarlog)
	{
		string timestr = s;
		if (timestr != "\n" && timestr != "\r") //Not interested in blank lines
		{
			timestamp	curtime;
			timestr = curtime.printshort() + ":" + timestr;
			sarlog->fwrite(timestr.c_str(), timestr.size());
			sarlog->fflush(); // Make sure it is written
		}
	}
	wattron(w, COLOR_PAIR(colorpair));
	for (unsigned int i = 0; i < strlen(s); i++)
	{
		if (s[i] == '\n' || s[i] == '\r')
			waddch(w, '\n');
		else
			waddch(w, s[i]);
	}
	wattroff(w, COLOR_PAIR(colorpair));
	wrefresh(w);
}

void
initwindows()
{
	// Initialise curses
	use_env(true); // So we can change the screen size
	use_tioctl(true); // So we can change the screen size
	initscr();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);
	scrollok(stdscr, FALSE);
	// Set up the colours
	if (has_colors() != FALSE)
	{
		start_color();
		// Colour on black
		init_pair(1, COLOR_RED, COLOR_BLACK);
		init_pair(2, COLOR_GREEN, COLOR_BLACK);
		init_pair(3, COLOR_YELLOW, COLOR_BLACK);
		init_pair(4, COLOR_BLUE, COLOR_BLACK);
		init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(6, COLOR_CYAN, COLOR_BLACK);
		init_pair(7, COLOR_WHITE, COLOR_BLACK);

		// Black on colour
		init_pair(8, COLOR_BLACK, COLOR_RED);
		init_pair(9, COLOR_BLACK, COLOR_GREEN);
		init_pair(10, COLOR_BLACK, COLOR_YELLOW);
		init_pair(11, COLOR_BLACK, COLOR_BLUE);
		init_pair(12, COLOR_BLACK, COLOR_MAGENTA);
		init_pair(13, COLOR_BLACK, COLOR_CYAN);
		init_pair(14, COLOR_BLACK, COLOR_WHITE);
	}
	else
	{
		fprintf(stderr, "Colours not supported\n");
		endwin();
		exit(1);
	}
	curs_set(1); // set the cursor visible
}

// *******************************************************************

/*
 * Window/subwindow heirachy is:
 *
 *            ---->errbox---->errwin
 *           /
 * stdscr--->
 *           \
 *            ---->cmdbox---->cmdwin
 *
 * stdscr - window
 * errbox subwin of stdscr
 * errwin subwin of errbox
 * cmdbox subwin of stdscr
 * cmdwin subwin of cmdbox
 */
screen::screen()
{
	if (_first)
	{
		initwindows();
		_first = false;
	}

	// Command is 1/ratio of screen at bottom
	int cmdwidth = COLS;
	int cmdheight = LINES / screen::ratio;
	int cmdstarty = LINES - (LINES / screen::ratio);
	int cmdstartx = 0;

	// Error is raio of screen at top
	int errwidth = COLS;
	int errheight = LINES - cmdheight;
	int errstarty = 0;
	int errstartx = 0;

	// Set up the command line window - bottom half of scr
	cmdbox = create_box(cmdheight, cmdwidth, cmdstarty, cmdstartx);
	cmdwin = create_subwin(cmdbox, cmdheight, cmdwidth, cmdstarty, cmdstartx);
	scrollok(cmdwin, TRUE); // We can scroll a window
	wrefresh(cmdbox);
	wrefresh(cmdwin);
	// Set up the error window - top half of scr
	errbox = create_box(errheight, errwidth, errstarty, errstartx);
	errwin = create_subwin(errbox, errheight, errwidth, errstarty, errstartx);
	scrollok(errwin, TRUE); // We can scroll the window
	wrefresh(errbox);
	wrefresh(errwin);

	_cmdwidth = cmdwidth - 2; // -2 For the border	
	_errwidth = errwidth - 2; // -2 for the border

	_fp = stderr;
}

screen::screen(const string& fname)
{
	if (_first)
	{
		initwindows();
		_first = false;
	}
// height, width, starty, startx

	// Command is 1/ratio of screen
	int cmdheight = LINES / screen::ratio ;
	int cmdwidth = COLS;
	int cmdstarty = LINES /  screen::ratio;
	int cmdstartx = 0;

	// Error is ratio of screen
	int errheight = LINES - cmdheight;
	int errwidth = COLS;
	int errstarty = 0;
	int errstartx = 0;

	// Set up the command line window - bottom half of scr
	cmdbox = create_box(cmdheight, cmdwidth, cmdstarty, cmdstartx);
	cmdwin = create_subwin(cmdbox, cmdheight, cmdwidth, cmdstarty, cmdstartx);
	scrollok(cmdwin, TRUE); // We can scroll a window
	wrefresh(cmdbox);
	wrefresh(cmdwin);
	// Set up the error window - top half of scr
	errbox = create_box(errheight, errwidth, errstarty, errstartx);
	errwin = create_subwin(errbox, errheight, errwidth, errstarty, errstartx);
	scrollok(errwin, TRUE); // We can scroll the window
	wrefresh(errbox);
	wrefresh(errwin);
/*
 * Close existing debug/error output file and open new one
 * but don't close an existing if its stderr or stdout
 */
	FILE	*newfp = nullptr;

	char *cname = new char[fname.size() + 1];
	strcpy(cname, fname.c_str());
	if ((newfp = fopen(cname, "a")) == nullptr)
	{
		wprintw(errwin, "screen::setout(%s) Can't open for output\n", cname);
		wrefresh(errwin);
	}
	else
		if (_fp != nullptr && _fp != stderr && _fp != stdout)
			fclose(_fp);
	_fp = newfp;
	delete[] cname;
}

screen::~screen()
{ 
	sleep(3);
	if (_fp != nullptr && _fp != stderr && _fp != stdout) fclose(_fp); 
	destroy_win(errwin);
	destroy_win(errbox);
	destroy_win(cmdwin);
	destroy_win(cmdbox);
	endwin();
};

// Resize the screen. So blow the windos away and recreate them to the new LINES & COLS
void
screen::resize()
{
	endwin();
	refresh();
	clear();

	// Command is 1/ratio of screen at bottom
	int cmdwidth = COLS;
	int cmdheight = LINES / screen::ratio;
	int cmdstarty = LINES - (LINES / screen::ratio);
	int cmdstartx = 0;

	// Error is raio of screen at top
	int errwidth = COLS;
	int errheight = LINES - cmdheight;
	int errstarty = 0;
	int errstartx = 0;

	destroy_win(errwin);
	destroy_win(errbox);
	destroy_win(cmdwin);
	destroy_win(cmdbox);

	wresize(stdscr, LINES, COLS);

	// Set up the command line window - bottom half of scr
	cmdbox = create_box(cmdheight, cmdwidth, cmdstarty, cmdstartx);
	cmdwin = create_subwin(cmdbox, cmdheight, cmdwidth, cmdstarty, cmdstartx);
	scrollok(cmdwin, TRUE); // We can scroll a window
	wrefresh(cmdbox);
	wrefresh(cmdwin);
	// Set up the error window - top half of scr
	errbox = create_box(errheight, errwidth, errstarty, errstartx);
	errwin = create_subwin(errbox, errheight, errwidth, errstarty, errstartx);
	scrollok(errwin, TRUE); // We can scroll the window
	wrefresh(errbox);
	wrefresh(errwin);

	_cmdwidth = cmdwidth - 2; // -2 For the border	
	_errwidth = errwidth - 2; // -2 for the border
}
	
void
screen::moveeol(const string& arg)
{
	unsigned int	y, x;

	getyx(cmdwin, y, x);
	if (y) ;
	wmove(cmdwin, y, arg.size() + c_prompt.len());
}

// Insert the char c into the current postion
// on the screen and in curarg
string
screen::insertkey(const string& arg, char c)
{
	string		newarg = "";
	string charstr;
	unsigned int	y, x;

	// Make sure we dont run off the end of a line for a command
	if ((arg.size() + c_prompt.len()) >= this->cmdwidth() - 1)
		return arg;
	getyx(cmdwin, y, x);
	charstr += c;
	if (y) ;
	// Normal case we have entered a character at the end of the arg
	if (x == (arg.size() + c_prompt.len()))
	{
		saratoga::scr.std(c);
		newarg = arg + charstr;
		return(newarg);
	}

	// We have to insert the character into the current position
	newarg = arg;
	newarg.insert(x - c_prompt.len(), charstr);

	// Move to beginning of line, erase it then print the new arguments out
	// Finally move back to where we inserted the new character + 1
	winsch(cmdwin, c);
	getyx(cmdwin, y, x);
	wmove(cmdwin, y, x + 1);
	wrefresh(cmdwin);
	return(newarg);
}

/*
 * Handle a backspace correctly
 */
string
screen::backspace(const string& arg)
{
	string		newarg = "";
	unsigned int	y, x;

	getyx(cmdwin, y, x);
	// We are the the beginning of arcand cannot backspace any further
	if ( x == c_prompt.len())
		return("");

	// We have to delete the character from the current position
	newarg = arg;
	newarg.erase(x - c_prompt.len() - 1, 1);

	// Move to beginning of line, erase it then print the new arguments out
	// Finally move back to where we inserted the new character + 1
	wmove(cmdwin, y, x - 1);
	wdelch(cmdwin);
	wrefresh(cmdwin);
	return(newarg);
}

/*
 * Left arrow - move left within command line
 */
void
screen::larrow(const string& arg)
{
	unsigned int	y, x;

	getyx(cmdwin, y, x);
	if (x > c_prompt.len())
		wmove(cmdwin, y, x - 1);
	wrefresh(cmdwin);
}

/*
 * Right arrow - move right within command line
 */
void
screen::rarrow(const string& arg)
{
	unsigned int	y, x;

	getyx(cmdwin, y, x);
	if (x < (arg.size() + c_prompt.len()))
		wmove(cmdwin, y, x + 1);
	wrefresh(cmdwin);
}

string
screen::uarrow()
{
	unsigned y, x;
	string retval = c_history.pop();

	getyx(cmdwin, y, x);
	wmove(cmdwin, y, c_prompt.len());
	wclrtoeol(cmdwin);
	saratoga::scr.stdnonl(retval);
	wrefresh(cmdwin);
	return retval;
}

/*
 * Down arrow - clear the command line
 */
void
screen::darrow()
{
	unsigned int y, x;

	getyx(cmdwin, y, x);
	if (x) ;
	if (x >= c_prompt.len())
	{
		wmove(cmdwin, y, c_prompt.len());
		wclrtoeol(cmdwin);
	}
	wrefresh(cmdwin);
}

/*
 * print a single character to command window - white on black
 */
void
screen::std(char c)
{
	char		cfmt[2];

	cfmt[0] = c;
	cfmt[1] = '\0';
	this->printwindow(cmdwin, cfmt, WHITE_BLACK);
}

/*
 * print formatted message to command window - white on black
 * with no nl
 */
void
screen::stdnonl(const string& fmt, ...)
{
	char	tmp[2048];
	va_list		args;
	string		sfmt(fmt);

	char *cfmt = new char[sfmt.size() + 1];
	strcpy(cfmt, sfmt.c_str());
	va_start(args, fmt);
	vsprintf(tmp, cfmt, args);
	va_end(args);
	this->printwindow(cmdwin, tmp, WHITE_BLACK);
	delete[] cfmt;
}

/*
 * print formatted message to command window - white on black
 * with nl at end
 */
void
screen::std(const string& fmt, ...)
{
	char	tmp[2048];
	va_list		args;
	string		sfmt(fmt);

	sfmt += '\n';
	char *cfmt = new char[sfmt.size() + 1];
	strcpy(cfmt, sfmt.c_str());
	va_start(args, fmt);
	vsprintf(tmp, cfmt, args);
	va_end(args);
	this->printwindow(cmdwin, tmp, WHITE_BLACK);
	delete[] cfmt;
}

/*
 * print formatted prompt message to command window (no cr at end)- white on black
 */
void
screen::prompt()
{
	string		s;

	s = c_prompt.print();

	this->printwindow(cmdwin, s.c_str(), WHITE_BLACK);
}

/*
 * print formatted information message to command window - yellow on black
 */
void
screen::info(const string& fmt, ...)
{
	char	tmp[2048];
	va_list		args;
	string		sfmt(fmt);

	sfmt += "\n";
	char *cfmt = new char[sfmt.size() + 1];
	strcpy(cfmt, sfmt.c_str());
	va_start(args, fmt);
	vsprintf(tmp, cfmt, args);
	va_end(args);
	this->printwindow(cmdwin, tmp, YELLOW_BLACK);
	delete[] cfmt;
}

/*
 * print formatted information message to error window - white on black
 */
void
screen::msg(const string& fmt, ...)
{
	char	tmp[2048];
	va_list		args;
	string		sfmt(fmt);

	sfmt += "\n";
	char *cfmt = new char[sfmt.size() + 1];
	strcpy(cfmt, sfmt.c_str());
	va_start(args, fmt);
	vsprintf(tmp, cfmt, args);
	va_end(args);
	this->printwindow(errwin, tmp, WHITE_BLACK);
	delete[] cfmt;
}

/*
 * print formatted information message to error window with no nl - yellow on black
 */
void
screen::msgnonl(const string& fmt, ...)
{
	char	tmp[2048];
	va_list		args;
	string		sfmt(fmt);

	char *cfmt = new char[sfmt.size() + 1];
	strcpy(cfmt, sfmt.c_str());
	va_start(args, fmt);
	vsprintf(tmp, cfmt, args);
	va_end(args);
	this->printwindow(errwin, tmp, YELLOW_BLACK);
	delete[] cfmt;
}

/*
 * print formatted message of an output frame - black on yellow
 */
void
screen::msgout(const string& fmt, ...)
{
	char	tmp[2048];
	va_list		args;
	string		sfmt(fmt);

	sfmt += "\n";
	char *cfmt = new char[sfmt.size() + 1];
	strcpy(cfmt, sfmt.c_str());
	va_start(args, fmt);
	vsprintf(tmp, cfmt, args);
	va_end(args);
	this->printwindow(errwin, tmp, BLACK_YELLOW);
	delete[] cfmt;
}

/*
 * print formatted message of an input frame - black on green
 */
void
screen::msgin(const string& fmt, ...)
{
	char	tmp[2048];
	va_list		args;
	string		sfmt(fmt);

	sfmt += "\n";
	char *cfmt = new char[sfmt.size() + 1];
	strcpy(cfmt, sfmt.c_str());
	va_start(args, fmt);
	vsprintf(tmp, cfmt, args);
	va_end(args);
	this->printwindow(errwin, tmp, BLACK_GREEN);
	delete[] cfmt;
}

/*
 * Print out debug msg if the level is >= setlev to error window - cyan on black
 */
void
screen::debug(int level, const string& fmt, ...)
{
	char	tmp[2048];
	char	header[128];
	va_list		args;
	string		sfmt;

	using namespace saratoga;

	// Only output debug's less than the level set
	// e.g. If the level set is 2 then only 2,1,0 debug's are output
	if (level <= c_debug.level())
	{
		sprintf(header, "DEBUG %d: ", level);
		sfmt = header;
		sfmt += fmt;
		sfmt += "\n";
		char *cfmt = new char[sfmt.size() + 1];
		strcpy(cfmt, sfmt.c_str());
		va_start(args, fmt);
		vsprintf(tmp, cfmt, args);
		va_end(args);
		this->printwindow(errwin, tmp, CYAN_BLACK);
		delete[] cfmt;
	}
}

/*
 * Print out error msg to error window - red on black
 */
void
screen::error(const string& fmt, ...)
{
	char	tmp[2048];
	va_list		args;
	string		sfmt("ERROR: ");

	sfmt += fmt;
	sfmt += "\n";
	char *cfmt = new char[sfmt.size() + 1];
	strcpy(cfmt, sfmt.c_str());
	va_start(args, fmt);
	vsprintf(tmp, cfmt, args);
	va_end(args);
	
	this->printwindow(errwin, tmp, RED_BLACK);
	delete[] cfmt;
}

/*
 * Print out warning msg to error window - magenta on black
 */
void
screen::warning(const string& fmt, ...)
{
	va_list		args;
	string		sfmt("WARNING: ");
	char	tmp[2048];

	sfmt += fmt;
	sfmt += "\n";
	char *cfmt = new char[sfmt.size() + 1];
	strcpy(cfmt, sfmt.c_str());
	va_start(args, fmt);
	vsprintf(tmp, cfmt, args);
	va_end(args);

	this->printwindow(errwin, tmp, MAGENTA_BLACK);
	delete[] cfmt;
}

/*
 * Print out formatted perror to error window - red on black
 * we pass in errno
 */
void
screen::perror(int err, const string& fmt, ...)
{
	va_list		args;
	string		sfmt("PERROR: ");
	char		s[1024];
	char		pe[1024];
	char		tmp[2048];

	sfmt += fmt;
	char *cfmt = new char[sfmt.length() + 1];
	strcpy(cfmt, sfmt.c_str());
	va_start(args, fmt);
	vsprintf(s, cfmt, args);
	va_end(args);
	errno = err;
	strcpy(pe, strerror(errno));
	sprintf(tmp, "%s %s\n", s, pe);
	this->printwindow(errwin, tmp, RED_BLACK);
	delete[] cfmt;
}

/*
 * Print out a fatal error to error window and die gracefully - black on red
 */
void
screen::fatal(const string& fmt, ...)
{
	timestamp	curtime;

	va_list		args;
	string		sfmt("FATAL ERROR: ");
	char		tmp[2048];

	sfmt += fmt;
	sfmt += "\n";
	char *cfmt = new char[sfmt.size() + 1];
	strcpy(cfmt, sfmt.c_str());
	va_start(args, fmt);
	vsprintf(tmp, cfmt, args);
	va_end(args);
	this->printwindow(errwin, tmp, BLACK_RED);
	delete[] cfmt;
	char	str[128];
	sprintf(str, "\n#################SARATOGA FATAL ERROR###################\n%s", 
		curtime.asctime().c_str());
	saratoga::scr.msg(str);
	sprintf(str, "########################################################\n"); 
	saratoga::scr.msg(str);
	sarlog->fflush();
	sleep(3);
	endwin();
	exit(1);
}

}; // namespace sarwin

