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

// Command Line Interpretor for saratoga

// #include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <sys/types.h>
#include "screen.h"
#include "ip.h"
#include "cli.h"
#include "sarflags.h"
#include "timestamp.h"
#include "fileio.h"
#include "beacon.h"
#include "request.h"
#include "globals.h"

using namespace std;

// A command has a name, a usage a help and a function to call

namespace saratoga
{

// Read in the configuration file and set the defaults up
// THis can be ANY valid command given to saratoga
void
readconf(string config_file)
{
	saratoga::cmds	cmd;
	string	args = "";
	std::vector<string> arglist;
	char	ch;
	sarfile::fileio	*rfp;
	int	comment = false;

	scr.debug(0, "readconf(): Reading configuration file %s",
		config_file.c_str());
	rfp = new sarfile::fileio(config_file, sarfile::FILE_READ);

	if (!rfp->ok())
	{
		scr.error("readconf: Cannot open saratoga config file ",
			config_file.c_str());
		return;
	}

	while (rfp->read(&ch, 1) == 1)
	{
		switch(ch)
		{
		case '#': // First char # Begins a comment
			if (args.length() == 0)
				comment = true;
			else
				args += '#';
			break;
		case CHAR_CR:
		case CHAR_LF:
			if (comment)
			{
				args = "";
				comment = false;
				break;
			}
			splitargs(args, arglist);
			if (arglist.size() < 1)
				break;
			if (comment)
			{
				comment = false;
				break;
			}
			if (arglist[0] == "updated" )
			{
				args = "";
				break;
			}
			if (cmd.cmdstr(arglist[0]) == "" || !(cmd.runcmd(arglist)))
				scr.error("readconf: Invalid config line \"%s\"", args.c_str());
			args = "";
			break;
		default:
			if (isprint(ch))
				args += (char) ch;
			else
				scr.error("readconf: Invalid character <%d>", (int) ch);
			break;
		}
	}
	delete rfp;
	scr.debug(0, "readconf(): Configuration file %s applied", config_file.c_str());
}

void
writeconf(string config_file)
{
	saratoga::cmds	cmd;
	string	args = "";
	std::vector<string> arglist;
	char	ch;
	sarfile::fileio	*rfp;
	sarfile::fileio	*wfp;
	char	tmp[128];
	string	tmpfile = config_file + ".tmp";

	scr.debug(3, "writeconf: Writing configuration file %s", config_file.c_str());
	rfp = new sarfile::fileio(config_file.c_str(), sarfile::FILE_READ);
	wfp = new sarfile::fileio(tmpfile.c_str(), sarfile::FILE_WRITE);

	if (!rfp->ok() || !wfp->ok())
	{
		scr.error("writeconf: Cannot copy saratoga config file %s",
			config_file.c_str());
		return;
	}

	while (rfp->read(&ch, 1) == 1)
	{
		switch(ch)
		{
		case CHAR_CR:
		case CHAR_LF:
			splitargs(args, arglist);
			if (arglist[0] == "session")
			{
				// Increment the Session # so it is unique next time around
				sprintf(tmp, "session %" PRIu32 "\n", c_session.set());
				wfp->fwrite(tmp, strlen(tmp));
			}
			else if (arglist[0] == "updated")
			{
				// Get latest timestamp and put it in config
				// This way we know when file was last updated
				timestamp ts = timestamp();
				sprintf(tmp, "updated %s\n", ts.printshort().c_str());
				wfp->fwrite(tmp, strlen(tmp));
			}
			else
			{
				wfp->fwrite(args.c_str(), args.length());
				wfp->fwrite("\n", 1);
			}
			wfp->fflush(); // Actually write it
			args = "";
			break;
		default:
			args += ch;
			break;
		}
	}
	wfp->fflush();
	wfp->clear();
	delete rfp;
	delete wfp;
	
	// Move the tmp file to the original
	if (rename(tmpfile.c_str(), config_file.c_str()) != 0)
	{
		scr.perror(errno, "Can't move %s to %s",
			tmpfile.c_str(), config_file.c_str());
		return;
	}
	scr.debug(3, "writeconf(): Configuration file %s written",
		config_file.c_str());
}

}; // namespace saratoga

