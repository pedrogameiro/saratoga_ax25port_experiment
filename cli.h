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

#ifndef _CLI_H
#define _CLI_H

#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <list>
#include <sys/types.h>
#include "saratoga.h"
#include "screen.h"
#include "timestamp.h"
#include "ip.h"
// #include "fileio.h"

using namespace std;

namespace saratoga
{

// CLI Interface for exit
class cli_exit
{
private:
	int	_flag; // -1, 0, 1, 2
public:
	cli_exit() {
		_flag = -1;
	};
	
	~cli_exit() {
		_flag = -1;
	};
	void	flag(int x) { _flag = x; };
	int	flag() { return(_flag); };
};

// CLI Interface for beacon
class cli_beacon
{
private:
	bool	_v4flag; // broadcast v4 beacons
	bool	_v6flag; // broadcast v6 beacons
	int64_t	_secs; // send a beacon every x secs
	timestamp	*_timer; // time left to run
	std::list<sarnet::ip>	_ip; // IP Address's to send beacon to
public:
	cli_beacon() {
		_v4flag = false;
		_v6flag = false;
		_secs = 0;
		_timer = new timestamp();
	};
	
	~cli_beacon() {
		_v4flag = false;
		_v6flag = false;
		_secs = 0;
		delete _timer;
		this->zap();
	};

	void zap() 
	{ 
		while (! _ip.empty()) 
		{
			// delete _ip.front();
			_ip.pop_front(); 
		}
	};

	void	v4flag(bool x) { _v4flag = x; };
	bool	v4flag() { return(_v4flag); };
	void	v6flag(bool x) { _v6flag = x; };
	bool	v6flag() { return(_v6flag); };
	void	secs(size_t x) {_secs = x; };
	int64_t	secs() {return(_secs); };
	void	ipadd(string s) { this->iprm(s); _ip.push_back(s); };
	void	iprm(string s) 
	{
		sarnet::ip	tmp(s); 
		for (std::list<sarnet::ip>::iterator i = _ip.begin(); i != _ip.end(); i++)
		{
			if (*i == tmp)
			{
				_ip.remove(*i);
				break;
			}
		}
	};
	std::list<sarnet::ip>::iterator begin() { return(_ip.begin()); };
	std::list<sarnet::ip>::iterator end() { return(_ip.end()); };

	// Are we ready to run a beacon ?
	bool	ready() { return (this->v4flag() || this->v6flag() || !this->_ip.empty()); };
	bool	timerexpired();

	timestamp*	timer() { return _timer; };

	void	settimer(const timestamp &tim) {
		delete _timer;
		_timer = new timestamp(tim);
	};

	string print();
	bool execute(); // Run the beacon
};

class cli_checksum
{
private:
	enum f_csumtype _flag; // What type of checksums
				// F_CSUM_NONE, CRC32, MD5, SHA1
public:
	cli_checksum() {
		_flag = F_CSUM_NONE;
	};
	~cli_checksum() {
		_flag = F_CSUM_NONE;
	};
	void	flag(enum f_csumtype x) { _flag = x; };
	enum f_csumtype	flag() { return(_flag); };
	string print();
};

class cli_debug
{
private:
	int	_level; // 0-9
public:
	cli_debug() { 
		_level = 0;
	};
	~cli_debug() {
		_level = 0;
	};
	void	level(int lev) { 
		if (lev >= 0 && lev <= 9) 
			_level = lev; 
		else 
			_level = 0; };
	int	level() { return _level; };
	string print();
};

class cli_descriptor
{
private:
	enum f_descriptor _flag; // What type of descriptor
				// F_DESCRIPTOR_16, 32, 64, 128
public:
	cli_descriptor() {
		extern enum f_descriptor maxdescriptor();
		_flag = maxdescriptor(); // Set to os max file size
	};
	~cli_descriptor() {
		extern enum f_descriptor maxdescriptor();
		_flag = maxdescriptor();
	};
	void	flag(enum f_descriptor x) { _flag = x; };
	enum f_descriptor	flag() { return(_flag); };
	string print();
};

class cli_eid
{
private:
	string	_eid; // The eid
public:
	cli_eid() {
		int	pid;
		char	s[16];
		pid = getpid();
		sprintf(s, "%d", pid);
		_eid = s;
	};
	~cli_eid() {
		int	pid;
		char	s[16];
		pid = getpid();
		sprintf(s, "%d", pid);
		_eid = s;
	};
	void	eid(string x) { _eid = x; };
	string	eid() { return(_eid); };
	string print();
};

class cli_freespace
{
private:
	enum f_freespace	_flag;
public:
	cli_freespace() {
		_flag = F_FREESPACE_NO;
	};
	~cli_freespace() {
		_flag = F_FREESPACE_NO;
	};
	void	flag(enum f_freespace x) { _flag = x; };
	enum f_freespace	flag() { return(_flag); };
	string print();
};

class cli_get
{
private:
	bool	_ready;
	string	_fname;	// The file to receive into
	sarnet::ip	_peer; // The peer to get from
public:
	cli_get() { };
	~cli_get() { this->zap(); };

	void	peer(sarnet::ip &p) { _peer = p; };
	sarnet::ip	*peer() { return &_peer; };
	void	fname(string s) { _fname = s; };
	string	fname() { return _fname; };
	void zap()
	{
		_fname = "";
		sarnet::ip tmp;
		_peer = tmp;
	}
	bool	ready() { return _ready; };
	bool	ready(bool state) { _ready = state; return _ready; };
	bool execute(); // Run the get
};

class cli_getrm
{
private:
	bool	_ready;
	string	_fname;	// The file to receive into
	sarnet::ip	_peer; // The peer to get from
public:
	cli_getrm() { };
	~cli_getrm() { this->zap(); };

	void	peer(sarnet::ip &p) { _peer = p; };
	sarnet::ip	*peer() { return &_peer; };
	void	fname(string s) { _fname = s; };
	string	fname() { return _fname; };
	void zap()
	{
		_fname = "";
		sarnet::ip tmp;
		_peer = tmp;
	}
	bool	ready() { return _ready; };
	bool	ready(bool state) { _ready = state; return _ready; };
	bool execute(); // Run the getrm
};

class cli_history
{
private:
	static const unsigned int _hsize = 20; // Maximum # history
	std::list<string>	_history;
public:
	cli_history() { }

	~cli_history() { _history.clear(); }

	// Push the command onto the back of the list and remove the
	// first item from the list if > maximum history size
	void push(string s)
	{
		if (s != "")
		{
			if (_history.size() == _hsize)
				_history.pop_front();
			_history.push_back(s);
		}
	}

	string pop()
	{
		string retval = "";
		if (_history.size() != 0)
		{
			retval = _history.back();
			_history.pop_back();
		}
		return retval;
	}

	string back() {
		if (_history.size() != 0)
			return _history.back();
		else
			return "";
	}

	unsigned int size() { return _history.size(); };

	string get(unsigned int);

	string	print();
};

// Set/get the home directory to get/put files
class cli_home
{
private:
	string	_fname; // The home directory for transfers
public:
	cli_home(string homedir) { _fname = homedir; };
	cli_home() { _fname = "."; }; // Default to current directory
	~cli_home() { _fname = "."; };
	void	dir(string x) { _fname = x; };
	void	fname(string x) { _fname = x; };
	string	dir() { return(_fname); };
	string	fname() { return(_fname); };
	string print() { return(_fname); };;
};

class cli_ls
{
private:
	bool	_ready;
	string	_dname;
	sarnet::ip	_peer;
public:
	cli_ls() { };
	~cli_ls() { this->zap(); };

	void	peer(sarnet::ip &p) { _peer = p; };
	sarnet::ip	*peer() { return &_peer; };
	void	dname(string s) { _dname = s; };
	string	dname() { return _dname; };
	void zap()
	{
		_dname = "";
		sarnet::ip tmp;
		_peer = tmp;
	}
	bool	ready() { return _ready; };
	bool	ready(bool state) { _ready = state; return _ready; };
	bool execute(); // Run the ls
};

class cli_maxbuff
{
private:
	size_t	_maxbuff;
public:
	cli_maxbuff()	{ _maxbuff = 10240; };
	~cli_maxbuff()	{ _maxbuff = 10240; };

	size_t	get() { return(_maxbuff); };
	size_t	set() { _maxbuff=10240; return(_maxbuff); };
	size_t	set(size_t s) { _maxbuff = s; return(_maxbuff); };
	string		print();
};

class cli_put
{
private:
	bool	_ready;		// Are we ready to run the command ?
	string	_fname;		// File name in REQUEST packet
	string	_localfname;	// Actual Local File name
	sarnet::ip	*_peer;
public:
	cli_put() { };
	~cli_put() { this->zap(); };

	void	peer(sarnet::ip &p) { _peer = new sarnet::ip(p); };
	sarnet::ip	*peer() { return _peer; };
	void	fname(string s) { 
		_fname = s.substr(s.find_last_of("\\/") + 1); };
	string	fname() { return _fname; };
	void	localfname(string s) { _localfname = s; };
	string	localfname() { return _localfname; };

	void zap()
	{
		_fname.clear();
		delete _peer;
	}

	bool	ready() { return _ready; };
	bool	ready(bool state) { _ready = state; return _ready; };
	bool execute(); // Run the put
};

class cli_putrm
{
private:
	bool	_ready;		// Are we ready to run the command ?
	string	_fname;
	sarnet::ip	_peer;
public:
	cli_putrm() { };
	~cli_putrm() { this->zap(); };

	void	peer(sarnet::ip &p) { _peer = p; };
	sarnet::ip	*peer() { return &_peer; };
	void	fname(string s) { _fname = s; };
	string	fname() { return _fname; };
	void zap()
	{
		_fname = "";
		sarnet::ip tmp;
		_peer = tmp;
	}
	bool	ready() { return _ready; };
	bool	ready(bool state) { _ready = state; return _ready; };
	bool execute(); // Run the putrm
};

class cli_rm
{
private:
	bool	_ready;		// Are we ready to run the command ?
	string	_fname;
	sarnet::ip	_peer;
public:
	cli_rm() { };
	~cli_rm() { this->zap(); };

	void	peer(sarnet::ip &p) { _peer = p; };
	sarnet::ip	*peer() { return &_peer; };
	void	fname(string s) { _fname = s; };
	string	fname() { return _fname; };
	void zap()
	{
		_fname = "";
		sarnet::ip tmp;
		_peer = tmp;
	}
	bool	ready() { return _ready; };
	bool	ready(bool state) { _ready = state; return _ready; };
	bool execute(); // Run the rm
};

class cli_rmdir
{
private:
	bool	_ready;
	string	_dname;
	sarnet::ip	_peer;
public:
	cli_rmdir() { };
	~cli_rmdir() { this->zap(); };

	void	peer(sarnet::ip &p) { _peer = p; };
	sarnet::ip	*peer() { return &_peer; };
	void	dname(string s) { _dname = s; };
	string	dname() { return _dname; };
	void zap()
	{
		_dname = "";
		sarnet::ip tmp;
		_peer = tmp;
	}
	bool	ready() { return _ready; };
	bool	ready(bool state) { _ready = state; return _ready; };
	bool execute(); // Run the rmdir
};

class cli_rx
{
private:
	enum f_rxwilling	_flag;
public:
	cli_rx() { _flag = F_RXWILLING_YES; };

	~cli_rx() { _flag = F_RXWILLING_YES; };
	void	flag(enum f_rxwilling x) { _flag = x; };
	enum f_rxwilling	flag() { return(_flag); };
	string print();
};

class cli_session
{
private:
	session_t	_session;
public:
	cli_session()	{ _session = 0; };
	~cli_session()	{ _session = 0; };

	session_t	get() { return(_session); };
	session_t	set() { _session++; return(_session); };
	session_t	set(session_t s) { _session = s; return(_session); };
	string		print();
};

class cli_stream
{
private:
	enum f_stream	_flag;
public:
	cli_stream() { _flag = F_STREAMS_NO; };

	~cli_stream() { _flag = F_STREAMS_NO; };
	void	flag(enum f_stream x) { _flag = x; };
	enum f_stream	flag() { return(_flag); };
	string print();
};

class cli_timestamp
{
private:
	enum f_reqtstamp _flag; // Do we do timestamps
	enum t_timestamp	_ttype; // What type
public:
	cli_timestamp() {
		_flag = F_TIMESTAMP_NO;
		_ttype = T_TSTAMP_UDEF; };
	~cli_timestamp() {
		_flag = F_TIMESTAMP_NO;
		_ttype = T_TSTAMP_UDEF; };
	void flag(enum f_reqtstamp x) { _flag = x; };
	enum f_reqtstamp flag() { return(_flag); };
	void ttype(enum t_timestamp x) { _ttype = x; };
	enum t_timestamp ttype() { return(_ttype); };
	string print();
};

/*
 * Timer to flag a transfer timer, resend request, resend status, and beacons
 */
class cli_timer
{
private:
	static const offset_t	_default = 300; // Default timer seconds
	offset_t	_transfer; // Transfer timeout 
	offset_t	_request; // Request timeout 
	offset_t	_status; // Status timeout 
	offset_t	_beacon; // Beacon timer
	offset_t	_framedelay; // delay between Tx frame
public:
	cli_timer() {
		_transfer = _default;
		_request = _default; 
		_status = _default; 
		_framedelay = 0; // No delay default
		_beacon = _default; };
	~cli_timer() {
		_transfer = _default;
		_request = _default;
		_status = _default;
		_framedelay = 0;
		_beacon = _default; };
	offset_t def() { return _default; };
	offset_t request() { return _request; };
	void request(offset_t req) { _request = req; };
	offset_t transfer() { return _transfer; };
	void transfer(offset_t tran) { _transfer = tran; };
	offset_t status() { return _status; };
	void status(offset_t stat) { _status = stat; };
	offset_t framedelay() { return _framedelay; };
	void framedelay(offset_t fdelay) { _framedelay = fdelay; };
	offset_t beacon() { return _beacon; };
	void beacon(offset_t b) { _beacon = b; };
	string	print();
	string	print_request();
	string	print_transfer();
	string	print_status();
	string	print_framedelay();
	string	print_beacon();
};

class cli_timezone
{
private:
	enum timezone	_tz; // UTC or LOCAL
public:
	cli_timezone() { _tz = TZ_UTC; };
	~cli_timezone() { _tz = TZ_UTC; };
	void	tz(enum timezone  x) { _tz = x; };
	enum timezone 	tz() { return(_tz); };
	string print();
};

class cli_transfers
{
private:
public:
	cli_transfers() {};
	~cli_transfers() {};
	string	print();
};

class cli_tx
{
private:
	enum f_txwilling 	_flag;
public:
	cli_tx() { _flag = F_TXWILLING_YES; };

	~cli_tx() { _flag = F_TXWILLING_YES; };
	void	flag(enum f_txwilling  x) { _flag = x; };
	enum f_txwilling 	flag() { return(_flag); };
	string print();
};

class cli_multicast
{
private:
	bool	_state; // true or false
public:
	cli_multicast() { _state = true; };
	~cli_multicast() { _state = true; };
	void	on() { _state = true; };
	void	off() { _state = false; };
	bool	state() { return(_state); };
	string print();
};

class cli_prompt
{
private:
	string	_prompt; // The command line prompt
public:
	cli_prompt() { _prompt = "saratoga: "; };
	~cli_prompt() { _prompt = "saratoga: "; };
	void	prompt(string p) { _prompt = p; };
	unsigned int	len() { return _prompt.length(); };
	string print();
};

/*
 *****************************************************************************************
 */

// A command has a name, a usage and a function to call
class cmd 
{
typedef bool (cmd::*cmdptr)();
private:
	string	_cmd;
	string	_usage;
	string	_help;
	cmdptr	_function;
	vector<string>	_args;
public:
	// These are the command line handlers for each function
	bool cmd_help();
	bool cmd_beacon();
	bool cmd_checksum();
	bool cmd_debug();
	bool cmd_descriptor();
	bool cmd_exit();
	bool cmd_eid();
	bool cmd_files();
	bool cmd_freespace();
	bool cmd_get();
	bool cmd_getrm();
	bool cmd_history();
	bool cmd_home();
	bool cmd_ls();
	bool cmd_maxbuff();
	bool cmd_pinfo();
	bool cmd_peers();
	bool cmd_prompt();
	bool cmd_put();
	bool cmd_putrm();
	bool cmd_rm();
	bool cmd_rmdir();
	bool cmd_rx();
	bool cmd_session();
	bool cmd_stream();
	bool cmd_timer();
	bool cmd_timestamp();
	bool cmd_timezone();
	bool cmd_transfers();
	bool cmd_tx();

	cmd() {};
	cmd(string cmd, string usage, string help, cmdptr fp)
	{
		_cmd = cmd; // The base command
		_usage = usage; 
		_help = help;
		_function = fp; 
	};

	~cmd() { };

	cmd& operator =(const cmd &c)
	{
		_cmd = c._cmd;
		_usage = c._usage;
		_help = c._help;
		_function = c._function;
		return(*this);
	}

	bool operator ==(const cmd &rhs) { return(rhs._cmd == _cmd); };
	bool operator ==(string rhs) { return(rhs == _cmd); };
	bool operator !=(const cmd &rhs) { return !(rhs._cmd == _cmd); };
	bool operator !=(string rhs) { return !(rhs == _cmd); };

	string name() { return _cmd; };
	string usage() { return _usage; };
	string help() { return _help; };
	void setargs(vector<string>);
	void delargs();
	bool run() { return((this->*_function)()); };
	void print() { cout << _cmd << " " << _usage << endl; };
	void errprint() { cerr << _cmd << " " << _usage << endl; };
};

class cmds
{
private:

// #define NCMDS (sizeof(_clist) / sizeof(cmd))

	const static int	_ncmds = 32;

	cmd	_clist[_ncmds] = 
	{
		{"?", 
			"", 
			"show valid commands. cmd ? show usage", 
			&cmd::cmd_help},
		{"beacon", 
			"beacon [off] [v4|v6|<ip>...] [secs]", 
			"send a beacon every secs", 
			&cmd::cmd_beacon},
		{"checksum",
			"checksum [off|none|crc32|md5|sha1]",
			"set checksums required and type",
			&cmd::cmd_checksum},
		{"debug",
			"debug [off|0..9]",
			"set debug level 0..9",
			&cmd::cmd_debug},
		{"descriptor",
			"descriptor [off|16|32|64|128]",
			"advertise & set default descriptor size",
			&cmd::cmd_descriptor},
		{"eid",
			"eid [off] <eid>",
			"manually set the eid",
			&cmd::cmd_eid},
	 	{"exit", 
			"exit [0|1]", 
			"exit saratoga", 
			&cmd::cmd_exit},
	 	{"files", 
			"files", 
			"List local files currently open and mode", 
			&cmd::cmd_files},
	 	{"freespace", 
			"freespace [off|on]", 
			"do we advertise freespace", 
			&cmd::cmd_freespace},
		{"get",
			"get <peer> <filename>",
			"Get a file from a peer",
			&cmd::cmd_get},
		{"getrm",
			"getrm <peer> <filename>",
			"Get a file from a peer then remove it from the peer",
			&cmd::cmd_getrm},
		{"help", 
			"help", 
			"show commands", 
			&cmd::cmd_help},
		{"history", 
			"history", 
			"show command history", 
			&cmd::cmd_history},
		{"home",
			"home <dirname>",
			"Set home directory for transfers",
			&cmd::cmd_home},
		{"ls",
			"ls <peer> [<dirname>]",
			"Get a directory listing from a peer",
			&cmd::cmd_ls},
		{"maxbuff",
			"maxbuff [<length>]",
			"Set maximum file read buffer length",
			&cmd::cmd_maxbuff},
		{"pinfo",
			"pinfo",
			"List peer information",
			&cmd::cmd_pinfo},
		{"peers",
			"peers [remove] [<ip>...]",
			"List peers or Add/Remove peer(s)",
			&cmd::cmd_peers},
		{"prompt",
			"prompt [<prompt>]",
			"Set or show current prompt",
			&cmd::cmd_prompt},
		{"put",
			"put <peer> <filename>",
			"Send a file to a peer",
			&cmd::cmd_put},
		{"putrm",
			"putrm <peer> <filename>",
			"Send a file to a peer then remove it from the peer",
			&cmd::cmd_putrm},
 		{"quit", 
			"quit [0|1]", 
			"exit saratoga", 
			&cmd::cmd_exit},
		{"rm",
			"rm <peer> <filename>",
			"Remove a file from a peer",
			&cmd::cmd_rm},
		{"rmdir",
			"rmdir <peer> <dirname>",
			"Remove a directory from a peer",
			&cmd::cmd_rmdir},
		{"rx",
			"rx [on|off]", 
			"Saratoga can or cannot receive",
			&cmd::cmd_rx},
		{"session",
			"session <number>", 
			"Set the session number",
			&cmd::cmd_session},
		{"stream",
			"stream [on|off]", 
			"Saratoga can or cannot handle stream",
			&cmd::cmd_stream},
		{"timer",
			"timer [off|request|transfer|status|framedelay|beacon] secs",
			"Timeout msecs for beacon's, request's, transfers, framebuf tx & status",
			&cmd::cmd_timer},
		{"timestamp",
			"timestamp [off|32|64|32_32|64_32|32_y2k]",
			"Timestamps off or what type to send",
			&cmd::cmd_timestamp},
		{"timezone",
			"timezone [utc|local]",
			"Timezone to use local time or universal time",
			&cmd::cmd_timezone},
		{"transfers",
			"transfers",
			"List current active transfers",
			&cmd::cmd_transfers},
		{"tx",
			"tx [on|off]", 
			"Saratoga can or cannot transmit",
			&cmd::cmd_tx},
	};
public:
	cmds() { };

	~cmds() { };

	string cmatch(string); // Given a string return closest command match
	string cmdmatches(string); // Given string return list commands matching
	string usage(string); // Given a command string print its usage
	string cmdstr(string); // Given a command string print its string (will be same)
	bool runcmd(vector<string>);// Given command & arguments run the function to do it
					// the first vector[0] is the command to run
	string print(); // Print out the list of available commands
	string printusage(string); // Print out the usage of a command or error 
};

}; // Namespace saratoga

#endif // _CLI_H

