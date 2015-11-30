#
# Copyright (c) 2011, Charles Smith
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, 
# are permitted provided that the following conditions are met:
#
#    * Redistributions of source code must retain the above copyright notice, this 
#      list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright notice, this
#      list of conditions and the following disclaimer in the documentation and/or 
#      other materials provided with the distribution.
#    * Neither the name of Vallona Networks nor the names of its contributors 
#      may be used to endorse or promote products derived from this software without 
#      specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
# OF THE POSSIBILITY OF SUCH DAMAGE.
#

SConscript('./checksums/SConstruct')

saratoga_src = Split( """
	sysinfo.cpp
	fileio.cpp
	offsetstr.cpp
	flags.cpp
	sarflags.cpp
	htonll.cpp
	htonlll.cpp
	timestamp.cpp
	dirent.cpp
	frame.cpp
	beacon.cpp
	request.cpp
	status.cpp
	data.cpp
	metadata.cpp
	holes.cpp
	peerinfo.cpp
	tran.cpp
	cli.cpp
	readconf.cpp
	ip.cpp
	screen.cpp
	checksum.cpp
	globals.cpp
	execute.cpp
	""")

Library(target = 'saratoga',
	source = [saratoga_src ],
	cc = 'g++',
	CCFLAGS = ['-g','-std=c++0x', '-O2', '-Wall', '-Werror', '-fno-strict-aliasing', '-D_FILE_OFFSET_BITS=64', '-D_LARGEFILE64_SOURCE', '-D__STDC_FORMAT_MACROS'] )

Program(target = 'test',
	CC = 'g++',
	CCFLAGS = ['-g','-std=c++0x', '-O2', '-Wall', '-Werror', '-fno-strict-aliasing', '-D_FILE_OFFSET_BITS=64', '-D_LARGEFILE64_SOURCE', '-D__STDC_FORMAT_MACROS'],
	LIBPATH = ['.', 'checksums'],
	LIBS = ['saratoga', 'checksums', 'rt', 'ncurses'], 
	source = 'test.cpp' )

Program(target = 'test1',
	CC = 'g++',
	CCFLAGS = ['-g','-std=c++0x', '-O2', '-Wall', '-Werror', '-fno-strict-aliasing', '-D_FILE_OFFSET_BITS=64', '-D_LARGEFILE64_SOURCE', '-D__STDC_FORMAT_MACROS'],
	LIBPATH = ['.', 'checksums'],
	LIBS = ['saratoga', 'checksums', 'rt', 'ncurses'], 
	source = 'test1.cpp' )

Program(target = 'test2',
	CC = 'g++',
	CCFLAGS = ['-g','-std=c++0x', '-O2', '-Wall', '-Werror', '-fno-strict-aliasing', '-D_FILE_OFFSET_BITS=64', '-D_LARGEFILE64_SOURCE', '-D__STDC_FORMAT_MACROS'],
	LIBPATH = ['.', 'checksums'],
	LIBS = ['saratoga', 'checksums', 'rt', 'ncurses'], 
	source = 'test2.cpp' )

Program(target = 'test3',
	CC = 'g++',
	CCFLAGS = ['-g','-std=c++0x', '-O2', '-Wall', '-Werror', '-fno-strict-aliasing', '-D_FILE_OFFSET_BITS=64', '-D_LARGEFILE64_SOURCE', '-D__STDC_FORMAT_MACROS'],
	LIBPATH = ['.', 'checksums'],
	LIBS = ['saratoga', 'checksums', 'rt', 'ncurses'], 
	source = 'test3.cpp' )

Program(target = 'saratoga',
 	CC = 'g++',
 	CCFLAGS = ['-g','-std=c++0x', '-O2', '-Wall', '-Werror', '-fno-strict-aliasing', '-D_FILE_OFFSET_BITS=64', '-D_LARGEFILE64_SOURCE', '-D__STDC_FORMAT_MACROS'],
 	LIBPATH = ['.', 'checksums'],
 	LIBS = ['saratoga', 'checksums', 'rt', 'ncurses'], 
 	source = 'saratoga.cpp' )

