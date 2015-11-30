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

#include <iostream>
#include <cstring>
#include <string>
#include <limits>
using namespace std;

#include "saratoga.h"
#include "sarflags.h"
#include "screen.h"
#include "globals.h"
#include "timestamp.h"
#include "holes.h"


namespace saratoga
{

// Compare two holes used in sort
bool
compare_hole(hole &h1, hole &h2)
{
	if (h1.starts() > h2.starts())
		return false;
	if (h1.starts() == h2.starts() && h1.ends() > h2.ends())
		return false;
	return true;
}

/*
 **********************************************************************
 * HOLES
 **********************************************************************
 */

// Create a hole, make sure it is valid and won;t exceed limits of unsigned integer
// Holes are defined by a begin (0 being the first byte) and a length
// They are actually stored as an encopassed begin & end
hole::hole(offset_t s, offset_t l)
{
	offset_t        max = std::numeric_limits<offset_t>::max();
	
	_starts = s;
	if ( l == 0)
	{
		scr.error("hole(%" PRIu64 ", %" PRIu64 "): Hole size of 0",
			(uint64_t) s, (uint64_t) l);
		_ends = 0;
		return;
	}
	if ((s + l) > max || (s + l) < 0) // We have wrapped offset
	{
		scr.error("hole(%" PRIu64 ", %" PRIu64"): Hole size wraps offset_t",
			(uint64_t) s, (uint64_t) l);
		_ends = 0;
		return;
	}
	_ends = _starts + l - 1;
}

// Reset an existing hole to different begin & length
void
hole::set(offset_t s, offset_t l) 
{
	offset_t        max = std::numeric_limits<offset_t>::max();

	if ( l == 0)
	{
		scr.error("hole.set(%" PRIu64 ", %" PRIu64 "): Hole size of 0",
			(uint64_t) s, (uint64_t) l);
		_starts = 0;
		_ends = 0;
		return;
	}
	if ((s + l) > max || (s + l) < s) // We have wrapped offset
	{
		scr.error("hole.set(%" PRIu64 ", %" PRIu64"): Hole size wraps offset_t",
			(uint64_t) s, (uint64_t) l);
		_starts = 0;
		_ends = 0;
		return;
	}
	_starts = s;
	_ends = _starts + l - 1;
}

// Set the begin offset of an existing hole
void
hole::setstart(offset_t s)
{
	offset_t        max = std::numeric_limits<offset_t>::max();

	if (_ends < s)
	{
		scr.error("Invalid hole.setstarts(%" PRIu64 ", %" PRIu64"): End < Start",
			(uint64_t) s, (uint64_t) _ends);
		return;
	}
	if ((_ends - s) > max || (s - _ends) > s) // We have wrapped offset
	{
		scr.error("Invalid hole.setstarts(%" PRIu64 ", %" PRIu64"): Hole size wraps offset_t",
			(uint64_t) s, (uint64_t) _ends);
		return;
	}
	_starts = s;
}

// Set's the end offset of an existing hole
void
hole::setend(offset_t e)
{
	offset_t        max = std::numeric_limits<offset_t>::max();

	if (_starts > e)
	{
		scr.error("Invalid hole.setend(%" PRIu64 ", %" PRIu64"): Start > End",
			(uint64_t) _starts, (uint64_t) e);
		return;
	}
	if (e > max) // We have wrapped offset
	{
		scr.error("Invalid hole.setend(%" PRIu64 ", %" PRIu64"): Hole size wraps offset_t",
			(uint64_t) _starts, (uint64_t) e);
		return;
	}
	if (e == 0)
	{
		scr.warning("hole.setend(%" PRIu64 ", %" PRIu64"): End is 0",
			(uint64_t) _starts, (uint64_t) e);
		_starts = 0;
		_ends = 0;
		return;
	}
	_ends = e;
}

void
hole::setlength(offset_t l)
{
	offset_t        max = std::numeric_limits<offset_t>::max();

	if (l == 0)
	{
		_starts = 0;
		_ends = 0;
	}
	if ( _ends - _starts == 0)
	{
		scr.warning("hole.setlength(%" PRIu64 ", %" PRIu64 "): Hole size of 0",
			(uint64_t) _starts, (uint64_t) l);
		return;
	}
	if ((_starts + l) > max || (_starts + l) < _starts) // We have wrapped offset
	{
		scr.error("hole.setgth(%" PRIu64 ", %" PRIu64"): Hole size wraps offset_t",
			(uint64_t) _starts, (uint64_t) l);
		return;
	}
	_ends = _starts + l - 1;
}

string
hole::print() 
{
	char	tmp[128];
	string s = "";

	sprintf(tmp, "From %" PRIu64 " to %" PRIu64 " Len %" PRIu64 "",
		(uint64_t) this->starts(), (uint64_t) this->ends(), this->length());
	s += tmp;
	return(s);
}

/*
 ***********************************************************************************
 * Handle lists of holes
 ***********************************************************************************
 */

// Create a holes list with a first member
holes::holes(offset_t begin, offset_t len) 
{
	
	hole h(begin, len);
	if (h.length() != 0)
	{
		scr.error("holes(%" PRIu64 ", %" PRIu64 "): Maximum # %zu holes in list already)",
			begin,
			len,
			_max);
		return;
	}
	scr.debug(1, "holes::holes(): Created First Hole: ");
	scr.debug(1, h.print());
	if (len > 0)
		_holes.push_back(h);
}

// Given a hole Create a holes list with a first member
holes::holes(hole &h) 
{

	if (_holes.size() != _max)
	{
		scr.error("holes(%" PRIu64 ", %" PRIu64 "): Maximum # %zu holes in list already)",
			h.starts(),
			h.length(),
			_max);
		return;
	}
	scr.debug(1, "holes::holes(): Created First Hole: ");
	scr.debug(1, h.print());
	if (h.length() != 0)
		_holes.push_back(h);
}

// Print list of holes
string
holes::print()
{
	string s = "Holes:\n";
	for (std::list<hole>::iterator i = _holes.begin(); i != _holes.end(); i++)
	{
		s += "\t";
		s += i->print();
		s += "\n";
	}
	return(s);
};

// If a hole exists then yes
bool
holes::exists(const hole &h)
{
	for (std::list<hole>::iterator i = _holes.begin(); i != _holes.end(); i++)
	{
		if ( *i == h )
			return(true);
	}
	return (false);
}

// If a hole exists then yes
bool
holes::exists(offset_t s, offset_t l)
{
	hole h(s, l);
	for (std::list<hole>::iterator i = _holes.begin(); i != _holes.end(); i++)
	{
		if ( *i == h )
			return(true);
	}
	return (false);
}

// Fix up a sorted list of holes removing
// duplicate and holes that are redundant
// Keeps the list of holes as small as possible
void
holes::compress()
{
	string	s;

	scr.debug(1, "holes::compress(): Called with current list");
	// We can't compress zero or one holes so we are done
	if (_holes.size() < 2)
		return;

	// We must begin with a sorted list of holes
	_holes.sort(compare_hole);

	// Keep on traversing the list till we are optimised		
scanagain:
	for (std::list<hole>::iterator curhole = _holes.begin(); curhole != _holes.end(); curhole++)
	{
		s = "Current hole is: " + curhole->print();
		scr.debug(1, s);
		// We can't compress zero or one holes so we are done
		if (_holes.size() < 2)
			return;
		// Do we have a previous hole in the list
		std::list<hole>::iterator nexthole = curhole;
		nexthole++; // Point to the next hole
		s = "Next hole is: " + nexthole->print();
		scr.debug(1, s);

		// We have two holes that begin at the same place
		if (curhole->starts() == nexthole->starts())
		{
			s = "curbegin == nextbegin " + curhole->print() + " " + nexthole->print();
			scr.debug(1, s);
			if (curhole->length() == nexthole->length())
			{
				// The holes are the same so remove dups
				_holes.unique(compare_hole);
				s = "Duplicate hole removed " + curhole->print();
				scr.debug(1, s);
				goto scanagain;
			}
			else if (curhole->length() < nexthole->length())
			{
				// The current hole is encapsulated by the next so remove current
				_holes.remove(*curhole);
				goto scanagain;
			}
			else // curhole->length() > nexthole->length() 
			{
				// The next hole is encapsulated by the first so remove next
				_holes.remove(*nexthole);
				goto scanagain;
			}
		}

		// Our current hole extends into the next hole so fix the next hole
		if (curhole->ends() >= nexthole->starts())
		{
			s = "Join holes curend >= nextbegin " + curhole->print() + " " + nexthole->print();
			scr.debug(1, s);
			// Our current hole extends beyond the next hole
			if (nexthole->ends() <= curhole->ends() + 1)
			{
				_holes.remove(*nexthole);
				goto scanagain;
			}
			offset_t newlen = nexthole->ends() - curhole->ends();
			nexthole->set(curhole->ends() + 1, newlen);
			goto scanagain;
		}

		// Merge adjacent holes as required
		if (curhole->ends() + 1 >= nexthole->starts())
		{
			offset_t tmp = nexthole->ends() - curhole->starts() + 1;
			curhole->setlength(tmp);
			_holes.remove(*nexthole);
			goto scanagain;
		}	
	}
}

// Add a hole to the list
void
holes::add(offset_t begin, offset_t len)
{
	hole newhole(begin, len);

	// We don't add holes of length 0
	if (newhole.length() == 0)
		return;

	size_t numholes = this->count();
	// We are the first hole in the list just push it on
	if (numholes == 0)
	{
		scr.debug(1, "holes::add() Added First Hole: ");
		scr.debug(1,  newhole.print());
		_holes.push_back(newhole);
		return;
	}
	if (numholes >= holes::_max) // check to see if we have not exceeded the max # holes
	{
		scr.error("Maximum number of holes reached - Hole not added");
		return;
	}
	// Put the hole at the end of the list
	scr.debug(1, "holes::add() Added Next Hole: ");
	scr.debug(1, newhole.print());
	_holes.push_back(newhole);
	// Now optimse the holes list
	this->compress();
	return;
}

void
holes::remove(offset_t begin, offset_t len)
{
	hole fill(begin, len);

	if (fill.length() == 0)
		return;

	scr.debug(1,"holes::remove(): Removing Hole: ");
	scr.debug(1, fill.print());
	// No holes in the current list
	if (_holes.size() == 0)
	{
		scr.debug(1,"holes::remove(): No holes in list: ");
		return;
	}

	// We have a list of holes
	std::list<hole>::iterator curhole = _holes.begin();
	while (curhole != _holes.end())
	{
		scr.debug(1, "holes::remove(): Current Hole is: ");
		scr.debug(1, curhole->print());
		// The hole is further along on the list
		// -------------			Curhole
		//              ------------->		Hole
		if (fill.starts() > curhole->ends()) // Try next
		{
			scr.debug(2, "A");
			curhole++;
			continue;
		}
		if (fill.ends() < curhole->starts())
		{
			scr.debug(2, "B");
			break;
		}

		if (fill.starts() <= curhole->starts())
		{
			scr.debug(2, "C");
			// Adjust fill if required
			if (fill.starts() != curhole->starts())
			{
				scr.debug(2, "0");
				fill.set(curhole->starts(), fill.ends() - curhole->starts() + 1);
			}
			if (fill.ends() == curhole->ends()) // Just remove the hole
			{
				// Just delete the current hole
				// -------------	Curhole
				// -------------	Fill
				// 			Curhole After
				// So just delete it
				scr.debug(2, "1");
				_holes.remove(*curhole);
				break;
			}
			if (fill.ends() > curhole->ends())
			{
				// Fill extends beyond current hole
				// delete it and adjust fill
				// -------------	Curhole
				// -------------->	Fill
				// 			Curhole After
				//              ->	Fill After
				// So delete it and move on to the next hole
				scr.debug(2, "2");
				hole	tmp = *curhole;
				curhole++;
				_holes.remove(tmp);
				fill.set(tmp.ends() + 1, fill.length() - (tmp.ends() - fill.starts()) - 1); 
				continue;
			}
			if (fill.ends() < curhole->ends())
			{
				// Fill is at the beginning of this hole
				// Adjust the hole and we are done
				// 01234567890
				// ----------   Curhole curhole
				// ---          Fill
				//    -------     Newholes
				scr.debug(2, "3");
				curhole->set(fill.ends() + 1, curhole->ends() - fill.ends());
				break;
			}
			scr.error("holes.remove(%" PRIu64 ", %" PRIu64"): INVALID CAN'T GET HERE\n", 
				fill.starts(), fill.ends());
		}
		if (fill.starts() > curhole->starts())
		{
			scr.debug(2, "D");
			if (fill.ends() == curhole->ends())
			{
				scr.debug(2, "1");
				// 012345678901
				// ------------		Curhole  12 - (11 - 6) = 6
				//       ------		Fill
				// ------		Curhole After
				curhole->setlength(curhole->length() - (fill.ends() - fill.starts()));
				break;
			}
			if (fill.ends() > curhole->ends())
			{
				scr.debug(2, "2");
				// 012345678901234567
				// x          y
				// ------------         Curhole
				//      a          b
				//      ------------     Fill
				//     c                        
				// -----                Curhole After
				//             d   e
				//             -----	Fill After

				// c = a -1
				// d = y + 1
				// b = e
				// Fill len = e - d + 1
				// curhole len = a - x
				offset_t tmp = curhole->ends();
				curhole->setlength(fill.starts() - curhole->starts());
				fill.set(tmp + 1, fill.ends() - tmp);
				curhole++;
				continue;
			}
			if (fill.ends() < curhole->ends())
			{
				scr.debug(2, "3");
				hole newfront(curhole->starts(), fill.starts() - curhole->starts());
				hole newend(fill.ends() + 1, curhole->ends() - fill.ends());
				// Change the current holes end position
				curhole->setlength(fill.starts() - curhole->starts());
				// And add a new one to the end part
				this->add(newend.starts(), newend.length());
				break;
			}
		}
		scr.debug(2, "E");
		curhole++;	
	}
	scr.debug(2, "F");
	scr.debug(2, "\n");
}

}; // namespace saratoga
