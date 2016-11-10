## This project is very experimental and it is not ready for a real world scenario use.
In this project, developed in the context of the Communication Networks Engineering course in Instituto Superior Técnico (IST), the proposed challenge was to adapt a working Saratoga protocol implementation to work, not only over the original network stack, but also over AX25, a OSI layer 2 protocol of very popular use over amateur radio communications.

authors:
[Pedro Gameiro](pedro.a.gameiro@tecnico.ulisboa.pt)
Lénio Passos


-- Original description --
===================================================
Saratoga transfer protocol - Vallona implementation
===================================================

This is work in progress on an implementation of the
Saratoga data transfer protocol by Charles Smith,
owner of Vallona Networks.

This will implement most of version 1 of Saratoga,
which is described in:
http://tools.ietf.org/html/draft-wood-tsvwg-saratoga

Further information on Saratoga and its uses is
available from:
http://saratoga.sourceforge.net/

This implementation is free and without warranty,
under a nonrestrictive BSD license, as described in
the accompanying License.txt file.

To build this code, you will need to already have
installed:
- the SCons build package. SCons requires python.
- gcc and g++ compilers, or similar.
- the ncurses terminal library, or similar.

cd saratoga-vallona-dev
scons
