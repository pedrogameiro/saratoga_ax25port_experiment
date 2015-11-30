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

#ifndef _DIRFLAGS_H
#define _DIRFLAGS_H

using namespace std;


#include "saratoga.h"

using namespace saratoga;

namespace sardir 
{

/*
 * Directory Entry Dflag Field Format - 16 bit unsigned integer (uint16_t)
 *
 *             111111
 *  01234567 89012345
 * +--------+--------+
 * |1     AA|BBC     |
 * +--------+--------+
 *
 * 1  == Directory Entry Start of Frame (Bit 0)
 * AA == Directory Entry Properties (Bits 6-7)
 * BB == Directory Entry Descriptor (Bits 8-9)
 * C  == Directory Entry File or Bundle (Bit 10)
 */

// How many bits to shift across to get dflag
template<typename T>
inline T DSHIFT(const T& bits, const T& msb)
{
	return (16 - bits -msb);
}

// How many bits long is the dflag
template<typename T>
inline T DMASK(const T& bits)
{
	return ((1 << bits) - 1);
}

class Dflag
{
protected:
	dflag_t	shift;		// Number of bits to shift 
	dflag_t	set;		// The mask for the bits covered
public:
	inline Dflag(const dflag_t s, const dflag_t m)	{ 
		shift = s; set = m << s; };
	inline ~Dflag()	{ shift = 0; set = 0; };
	inline dflag_t	fget(dflag_t f) { return((f & set) >> shift); };
	inline dflag_t fset(dflag_t c, dflag_t f) { 
		return(((c) & ~set) | ((f) << shift)); };
};

/*
 * Saratoga Directory Entry Descriptor Size - Bits 8-9
 *  METADATA, DATA
 */
enum d_descriptor 
{
	D_DESCRIPTOR_16 = 0x00,
	D_DESCRIPTOR_32 = 0x01,
	D_DESCRIPTOR_64 = 0x02,
	D_DESCRIPTOR_128 = 0x03 
};

class Ddescriptor : private Dflag
{
protected:
	static const dflag_t	bits = 2;
	static const dflag_t	msb = 8;
	enum d_descriptor	descriptor;
public:
	// Constructor set the descriptor.
	Ddescriptor(dflag_t f) : Dflag(DSHIFT(bits, msb), DMASK(bits)) { 
		descriptor = (enum d_descriptor) (fget(f)); };
	Ddescriptor(enum d_descriptor f) : Dflag(DSHIFT(bits, msb), DMASK(bits)) { 
		descriptor = f; };

	// Assignment constructor
	Ddescriptor& operator=(const Ddescriptor &old) {
		descriptor = old.descriptor; return *this; };

	// Given Current Dflag set the bits for this descriptor
	void	set(dflag_t* c) { *c = fset(*c, descriptor); };
	void	set(enum d_descriptor f) { descriptor = f; };
	
	size_t	length();

	// What is the descriptor
	enum d_descriptor	get() { return(descriptor); };
	enum d_descriptor	get(dflag_t f) { return((enum d_descriptor) fget(f)); };

	dflag_t	shift() { return(DSHIFT(bits, msb)); };
	dflag_t	mask() { return(DMASK(bits)); };

	dflag_t setflag(dflag_t f) { f = fset(f, descriptor); return f; };
	
	// Print out the current descriptor
	std::string		print();
};

/*
 *  Saratoga Directory Entry Start of Directory Entry - Bit 0
 *  Must be 1
 */
enum d_sod
{
	D_SOD_NO = 0x00,	// INVALID!!!!
	D_SOD_YES = 0x01
};

class Dsod : private Dflag
{
protected:
	static const dflag_t bits = 1;
	static const dflag_t msb = 0;
	enum d_sod		sod;
public:
	Dsod() : Dflag(DSHIFT(bits, msb), DMASK(bits)) { sod = D_SOD_YES; }; // It is always 1
	Dsod(enum d_sod f) : Dflag(DSHIFT(bits, msb), DMASK(bits)) { sod = D_SOD_YES; }; // It is always 1
	Dsod(dflag_t f) : Dflag(DSHIFT(bits, msb), DMASK(bits)) { 
		sod = D_SOD_YES; }; // It is always 1

	Dsod& operator=(const Dsod &old) { sod = old.sod; return *this; };

	// Given Current Sflag set the bits for this sod
	void	set() { sod = D_SOD_YES; };
	void	set(dflag_t* c) { *c = (fset(*c, sod)); };
	void	set(enum d_sod f ) { sod = f; };

	// What is the sod
	enum d_sod	get() { return(sod); };
	enum d_sod	get(dflag_t f) { return((enum d_sod) fget(f)); };
	
	dflag_t	shift() { return(DSHIFT(bits, msb)); };
	dflag_t	mask() { return(DMASK(bits)); };
	
	dflag_t setflag(dflag_t f) { f = fset(f, sod); return f; };
	
	// Print out the current sod
	string		print();
};

/*
 * Saratoga Directory Entry Properties - Bits 6-7
 *  METADATA, DATA
 */
enum d_prop 
{
	D_PROP_FILE = 0x00,
	D_PROP_DIR = 0x01,
	D_PROP_SPECIALFILE = 0x02,
	D_PROP_SPECIALDIR = 0x03 
};

class Dprop : private Dflag
{
protected:
	static const dflag_t	bits = 2;
	static const dflag_t	msb = 6;
	enum d_prop		prop;
public:
	// Constructor set the prop.
	Dprop(enum d_prop f) : Dflag(DSHIFT(bits, msb), DMASK(bits)) { prop = f; };
	Dprop(dflag_t f) : Dflag(DSHIFT(bits, msb), DMASK(bits)) { prop = (enum d_prop) (fget(f)); };

	Dprop& operator=(const Dprop &old) { prop = old.prop; return *this; };

	// Given Current Sflag set the bits for this prop
	void	set(dflag_t* c) { *c = (fset(*c, prop)); };
	void	set(enum d_prop f ) { prop = f; };
	
	// What is the prop
	enum d_prop	get() { return(prop); };
	enum d_prop	get(dflag_t f) { return((enum d_prop) fget(f)); };
	
	dflag_t	shift() { return(DSHIFT(bits, msb)); };
	dflag_t	mask() { return(DMASK(bits)); };

	dflag_t setflag(dflag_t f) { f = fset(f, prop); return f; };
	
	// Print out the current prop
	string		print();
};

// The Directory Entry 16 bit flag
class DEflag
{
private:
	dflag_t	flag;
public:
	inline DEflag() { flag = 0; };
	inline DEflag(dflag_t f) { flag = f; };
	inline ~DEflag() { flag = 0; };

	dflag_t	get() { return flag; };

	// Copy flags ir set discreet value
	DEflag	operator=(dflag_t f) { flag = f; return *this; };
	DEflag	operator=(DEflag f) { flag = f.get(); return *this; };

	// Set the bits in a flag to the value f
	// This "adds" the flags value to the total flag
	// Perhaps this should of been "|=" but what the hell
	DEflag	operator+=(Dsod f) { flag = f.setflag(flag); return *this; };
	DEflag	operator+=(Ddescriptor f) { flag = f.setflag(flag); return *this; };
	DEflag	operator+=(Dprop f) { flag = f.setflag(flag); return *this; };

	string print();
};

} // Namesapce sardir

#endif // _DIRFLAGS_H

