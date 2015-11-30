/*

 Copyright (c) 2014, Charles Smith
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

#ifndef _HOLES_H
#define _HOLES_H

#include <iostream>
#include <cstring>
#include <string>
#include <list>
using namespace std;

#include "saratoga.h"
#include "sarflags.h"
#include "screen.h"
#include "timestamp.h"

namespace saratoga
{
/*
 **********************************************************************
 * HOLES
 **********************************************************************
 */

// A container for a single hole
class hole
{
private:
	offset_t	_starts;	// Where the hole begin
	offset_t	_ends;	// How long the hole is
public:
	// Create a hole begin at s with length l

	hole(offset_t s, offset_t l);

	// Copy Constructor
	hole(const hole& h)
	{
		_starts = h._starts;
		_ends = h._ends;
	};
		
	~hole() { this->zap(); };

	void zap() { _starts = 0; _ends = 0; };

	hole& operator =(const hole &h)
	{
		_starts = h._starts;
		_ends = h._ends;
		return(*this);
	};

	bool operator ==(const hole &rhs)
	{
		return( rhs._starts == _starts && rhs._ends == _ends);
	};

	bool operator !=(const hole &rhs)
	{
		return !( rhs._starts == _starts && rhs._ends == _ends);
	};

	// Comparison of 2 holes
	bool is_equal(const hole &h1, const hole &h2)
	{
		return (h1._starts == h2._starts && h1._ends == h2._ends);
	};
	

	// Set begin and length of an existing hole
	void 		set(offset_t s, offset_t l);
	// Set begin and offset of an existing hole
	void 		setstart(offset_t s);
	// Set end of an existing hole
	void 		setend(offset_t e);
	// Set len (adjust end) of an existing hole
	void		setlength(offset_t l);

	// The begin of the hole
	offset_t		starts() { return _starts; };

	// How long is the hole
	offset_t		length() { if (_ends >= _starts)
					return (_ends - _starts + 1); 
					return 0; };

	// The end of the hole
	offset_t		ends() { return _ends; };

	string	print();
};

// Compare 2 holes to sort a list
bool compare_hole(hole &h1, hole &h2);

// A container for multiple holes

class holes
{
private:
	const size_t	_max = 10000; // A maximum # of holes
	std::list<hole>	_holes;
	void compress(); // Compress list of holes after one added
public:
	// New Empty holes container
	holes() { };

	~holes() { _holes.clear(); };

	// Holes with first entry in list
	holes(offset_t begin, offset_t len);

	// Holes with first entry in list
	holes(hole &h);

	std::list<saratoga::hole>::iterator first() { return(_holes.begin()); };
	std::list<saratoga::hole>::iterator last() { return(_holes.end()); };
	
	// Add a hole to the list of holes
	void add(offset_t begin, offset_t len);

	// Copy a list of holes 
	holes& operator =(const holes& h)
	{
		_holes = h._holes;
		return(*this);
	}
		
	// Shortcut to add a new hole to a list
	holes& operator +=(hole &h)
	{
		this->add(h.starts(), h.length());
		return(*this);
	}

	void clear() { _holes.clear(); };

	// Shortcut to add a new hole list to a list of holes
	holes& operator +=(holes &hl)
	{
		for (std::list<hole>::iterator i = hl._holes.begin(); i != hl._holes.end(); i++)
			this->add(i->starts(), i->length());
		return(*this);
	}

	// Remove a hole from the list of holes
	void remove(offset_t s, offset_t l);

	// Remove a hole from the list of holes
	void remove(hole &h)
	{
		this->remove(h.starts(), h.length());
	}

	holes& operator -=(hole &h)
	{
		this->remove(h.starts(), h.length());
		return(*this);
	}

	// Remove a list of holes from a hole list
	holes& operator -=(holes &hl)
	{
		for (std::list<hole>::iterator i = hl._holes.begin(); i != hl._holes.end(); i++)
			this->remove(i->starts(), i->length());
		return(*this);
	}

	// true if a matching hole exists
	bool exists(const hole &h);
	
	bool exists(offset_t s, offset_t l);

	// How many holes do I have
	size_t count() { return _holes.size(); }

	// Maximum number of concurrent holes
	size_t maxholes() { return _max; };

	// Print the list of holes
	string print();
};

} // Namespace saratoga

#endif // _HOLES_H
