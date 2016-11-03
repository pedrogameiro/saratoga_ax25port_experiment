/*

 Copyright (c) 2014, Charles Smith
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

#include "fileio.h"
#include "checksum.h"
#include "globals.h"
#include "screen.h"
#include "sysinfo.h"
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <limits>
#include <string>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>

using namespace std;

namespace sarfile {

// True if the file exists, false if not and yes it has to be a file
bool
fexists(string fname)
{
  sarfile::fileio tmp(fname, sarfile::FILE_READ);
  if (tmp.ok() && tmp.isfile()) {
    scr.debug(7, "fexists(%s): File exists", fname.c_str());
    return (true);
  }
  scr.debug(7, "fexists(%s) File does not exist or is not a file",
            fname.c_str());
  return (false);
}

// Open a local file for reading or writing
fileio::fileio(string fname, enum rorw rwx)
{
  string s;

  _fname = fname;
  switch (rwx) {
    case FILE_EXCL:
      _rorw = FILE_EXCL;
      // We keep O_EXCL as we do not want to open it if it already exists
      // We do not truncate it
      // ie Only create if it does not already exist
      _fd = open(fname.c_str(),
                 O_CREAT | O_WRONLY | O_EXCL | O_LARGEFILE | O_SYNC | O_TRUNC,
                 S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
      if (_fd < 0) {
        scr.perror(errno, "fileio::fileio(%s): Cannot create file",
                   fname.c_str());
        _rorw = FILE_UNDEF;
        _fname.clear();
        _ok = false;
        return;
      }
      _ready =
        false; // We are not ready till we actually want to write something
      _ok = true;
      _dir = sardir::dirent(fname);
      scr.debug(7, "fileio: FILE_EXCL Dirent is: %s", _dir.print().c_str());
      // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      // For the moment we don't do checksums!!!!!!
      _csum = checksums::checksum(checksums::CSUM_NONE, fname);
      scr.debug(7, "fileio:FILE_EXCL Checksum is: %s", _csum.print().c_str());
      _csum_done = true;
      // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      scr.debug(7, "fileio: Created File(%d) %s for FILE_EXCL", _fd,
                fname.c_str());
      return;
    case FILE_WRITE:
      _rorw = FILE_WRITE;
      // We remove O_EXCL if it exists then we truncate it
      _fd =
        open(fname.c_str(), O_CREAT | O_WRONLY | O_LARGEFILE | O_SYNC | O_TRUNC,
             S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
      if (_fd < 0) {
        scr.perror(errno, "fileio::fileio: Cannot create file %s",
                   fname.c_str());
        _rorw = FILE_UNDEF;
        _fname.clear();
        _ok = false;
        return;
      }
      _dir = sardir::dirent(fname);
      // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      _csum = checksums::checksum(checksums::CSUM_NONE, fname);
      _csum_done = true;
      // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      scr.debug(7, "fileio::fileio: Created File(%d) %s for FILE_WRITE", _fd,
                fname.c_str());
      _ok = true;
      _ready =
        false; // We are not ready till we actually want to write something
      return;
    case FILE_READ:
      // open a file for reading
      _rorw = FILE_READ;
      _fd = open(fname.c_str(), O_RDONLY | O_LARGEFILE | O_SYNC);
      if (_fd < 0) {
        scr.perror(errno, "fileio: Cannot open file %s for FILE_READ",
                   fname.c_str());
        _rorw = FILE_UNDEF;
        _fname.clear();
        _ok = false;
        return;
      }
      _dir = sardir::dirent(fname);
      scr.debug(7, "fileio: FILE_READ Dirent is: %s", _dir.print().c_str());
      // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      _csum = checksums::checksum(checksums::CSUM_NONE, fname);
      scr.debug(7, "fileio: FILE_READ Checksum is: %s", _csum.print().c_str());
      _csum_done = true;
      // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      scr.debug(7, "fileio: Opened File(%d) %s for FILE_READ", _fd,
                fname.c_str());
      _ok = true;
      _ready = true; // We are ready to read
      return;
    default:
      scr.error("fileio: Undefined File open mode for %s", fname.c_str());
      _rorw = FILE_UNDEF;
      _fname.clear();
      _dir.clear();
      _csum.clear();
      _fd = -1;
      _ok = false;
      _ready = false;
      return;
  }
  // Never get here
  return;
}

int
fileio::funlink()
{
  char* sfname = new char[_fname.length() + 1];
  int ret;

  // Get a copy of fname first as fclose zaps it
  strcpy(sfname, _fname.c_str());
  if ((ret = unlink(sfname)) != 0)
    scr.perror(errno, "fileio::funlink(%s): ", sfname);
  delete sfname;
  return ret;
}

offset_t
fileio::flen()
{
  offset_t ret = 0;
  sysinfo fs(_fname);

  offset_t current = ::lseek64(_fd, 0, SEEK_CUR);
  ret = ::lseek64(_fd, 0, SEEK_END);
  if (ret < 0)
    scr.perror(errno, "fileio::flen(%d):", _fd);
  else if (ret > fs.diskfree())
    scr.error("fileio::flen(%d): Cannot seek beyond size of file system\n",
              _fd);
  if (::lseek64(_fd, 0, current) != current)
    scr.error("fileio::flen(%d): Cannot seek back to current position\n", _fd);
  return (ret);
}

// We dont actually write it we put it in the list of buffers and
// then set _ready to true so that the select() loop can handle the
// actual writing by calling write()
// This does a sequential write
ssize_t
fileio::fwrite(const char* b, const size_t cnt)
{
  if (cnt <= 0)
    return cnt;
  // Sequential Write to eof
  saratoga::buffer* tmp = new saratoga::buffer(b, cnt);
  _buf.push_back(*tmp);
  _sequential = true; // We write to current eof
  _ready = true;
  return (cnt);
}

// This does a lseek to offset o and then writes
ssize_t
fileio::fwrite(const char* b, const size_t len, const offset_t o)
{
  if (len <= 0)
    return len;
  // Random lseek to o then write
  saratoga::buffer* tmp = new saratoga::buffer(b, len, o);
  _buf.push_back(*tmp);
  _sequential = false; // We seek to offset then write
  _ready = true;
  return (len);
}

// As above but just send me an existing buffer
ssize_t
fileio::fwrite(const saratoga::buffer& b, bool sequential)
{
  saratoga::buffer* tmp = new saratoga::buffer(b);
  _ready = true;
  _sequential = sequential; // Either write to EOF or lseek to offset
  _buf.push_back(*tmp);
  return (tmp->len());
}

// Actually write buffers to a file
ssize_t
fileio::write()
{
  ssize_t nwritten;
  sysinfo fs(_fname);
  ssize_t totwritten = 0;

  if (_buf.empty()) {
    scr.debug(7, "write: Nothing written to %s buffers are empty",
              this->fname().c_str());
    return 0;
  }
  while (!_buf.empty()) {
    saratoga::buffer* tmp = &(_buf.front());
    char* b = tmp->buf();
    ssize_t blen = tmp->len();
    offset_t offset = tmp->offset();

    if (blen == 0 || b == nullptr) {
      _buf.pop_front();
      scr.error("write: Buffer Contains no information to %s",
                this->fname().c_str());
      continue;
    }
    if (!_sequential) {
      // We lseek to the offset then write otherwise just write to end
      offset_t seekpos;
      if ((seekpos = ::lseek64(_fd, offset, SEEK_SET)) != offset) {
        scr.error("fileio::write(%d) Seekpos=%" PRIu64
                  " Not written Cannot lseek64 to %" PRIu64 " in %s",
                  _fd, seekpos, offset, _fname.c_str());
        // Throw it away
        _buf.pop_front();
        continue;
      }
    }
    nwritten = ::write(_fd, b, blen);
    if (_fname != sarlog->fname())
      scr.debug(5, "fileio::write: Wrote %d bytes at offset %" PRIu64 " to %s",
                nwritten, offset, _fname.c_str());
    if (nwritten < 0) {
      int err = errno;
      scr.perror(err, "fileio::write(%d) Cannot write %d bytes to %s\n", _fd,
                 blen, _fname.c_str());
      // Throw it away
      _buf.pop_front();
      continue;
    }
    if (nwritten != blen) {
      scr.error("fileio::write(%d): Only wrote %d bytes of %d to %s\n", _fd,
                nwritten, blen, _fname.c_str());
      totwritten += nwritten;
      // Throw it away
      _buf.pop_front();
      // Pus the what is not written back into buffers
      b += nwritten;
      saratoga::buffer* buf =
        new saratoga::buffer(b, blen - totwritten, offset + nwritten);
      _buf.push_back(*buf);
      continue;
    }
    totwritten += nwritten;
    _buf.pop_front();
  }
  // This flag is used in the main select() loop to signal if we can SET cwfd's
  _ready = false; // We have done writing buffers so we are no longer ready
  return (totwritten);
}

// Just a raw read into a pre-allocated b
// Used for readng config files, NOT transfer files
// use the read(size_t blen) for that!
// as it reads into the fileio buffers
ssize_t
fileio::read(void* b, size_t len)
{
  ssize_t nread;
  //	sysinfo	fs(_fname);

  _ready = true;
  nread = ::read(_fd, b, len);
  if (nread < 0) {
    int err = errno;
    scr.perror(err, "fileio::read(%d) Cannot read from %s\n", _fd,
               _fname.c_str());
    return -1;
  }
  //	scr.debug(2, "fileio::read(): Raw read called and read %d bytes",
  //nread);
  return (nread);
}

// Actually read into the filio _buf list of buffers from a file
// Use this for all saratoga transfers
ssize_t
fileio::read(size_t blen)
{
  ssize_t nread;
  sysinfo fs(_fname);
  offset_t curoffset;

  char* b = new char[blen];

  _ready = true; // We are always ready to read
  // Where are we currently located in the file
  curoffset = ::lseek64(_fd, 0, SEEK_CUR);
  nread = ::read(_fd, b, blen);
  if (nread < 0) {
    int err = errno;
    scr.perror(err, "fileio::read(%d) Cannot read from %s\n", _fd,
               _fname.c_str());
    delete b;
    return (-1);
  }
  if (nread == 0)
    scr.debug(9, "fileio::read(%s): Buffered Read Nothing read!!!",
              this->fname().c_str());
  else {
    saratoga::buffer* buf = new saratoga::buffer(b, nread, curoffset);
    _buf.push_back(buf);

    scr.debug(9, "fileio::read(%s): Buffer Read %ld Bytes at offet %" PRIu64 "",
              this->fname().c_str(), nread, curoffset);
  }
  delete b;
  return (nread);
}

// This returns a buffers contents as a char *
// and alters/removes the buffers from the _buf list
// Handles smaller or multiple spans of _buf list
// If smaller wanted then push the remainder back to the
// front of the list.
// !!!!!! This is to ONLY USED for sequential file i/o !!!!!!!!
size_t
fileio::fget(char* s, const size_t slen)
{
  ssize_t tmpslen = slen;
  if (slen <= 0)
    return 0;
  if (_buf.empty()) {
    scr.error("fget: Nothing read buffers are empty");
    return (0);
  }
nextbuff:
  while (!_buf.empty()) {
    saratoga::buffer* tmp = &(_buf.front());
    char* b = tmp->buf();
    ssize_t blen = tmp->len();

    if (blen == 0 || b == nullptr) {
      scr.error("fget: Buffer Contains no information!!!");
      _buf.pop_front();
      goto nextbuff;
    }
    if (blen >= tmpslen) {
      memcpy(s, tmp->buf(), tmpslen);
      _buf.pop_front();
      saratoga::buffer* newbuf = new saratoga::buffer(s, tmpslen);
      if (newbuf->len() != 0)
        _buf.push_front(newbuf);
      else
        delete newbuf;
      return slen;
    }
    memcpy(s, tmp->buf(), blen);
    _buf.pop_front();
    s += blen;
    tmpslen -= blen;
  }
  // We have asked for more than was in the buffers return length
  // of what we have actually got.
  // This flag is used in the main select() loop to signal if we can SET cwfd's
  return (slen - tmpslen);
}

// Are we a regular file
bool
fileio::isfile()
{
  struct stat st;

  if (stat(_fname.c_str(), &st) == -1) {
    scr.error("fileio::isfile(), Cannot stat %s\n", _fname.c_str());
    return (false);
  }
  return ((st.st_mode & S_IFMT) == S_IFREG);
}

// Are we a directory
bool
fileio::isdir()
{
  struct stat st;

  if (stat(_fname.c_str(), &st) == -1) {
    scr.error("fileio::isdir(), Cannot stat %s\n", _fname.c_str());
    return (false);
  }
  return ((st.st_mode & S_IFMT) == S_IFDIR);
}

string
fileio::print()
{
  char tmp[1024];

  sprintf(tmp, "File: %s FD:%d", _fname.c_str(), _fd);
  string s = tmp;
  switch (_rorw) {
    case sarfile::FILE_READ:
      s += " Mode:READ";
      break;
    case sarfile::FILE_WRITE:
      s += " Mode:WRITE";
      break;
    case sarfile::FILE_EXCL:
      s += " Mode:EXCL";
      break;
    default:
      s += " Mode:Undefined";
      break;
  }
  /*
          // Don;t need to know this
          if (_ready)
                  s += " Ready:YES";
          else
                  s += " Ready:NO";
   */
  return (s);
}

void
files::remove(int fd)
{
  for (std::list<fileio>::iterator i = _files.begin(); i != _files.end(); i++) {
    if (i->fd() == fd) {
      string fname = i->fname();
      scr.debug(3, "files::remove(%d): Removing file %s from file list", fd,
                fname.c_str());
      _files.erase(i);
      _fdchange = true;
      return;
    }
  }
  scr.error("files::remove(%d) does not exist in files", fd);
}

void
files::funlink(int fd)
{
  for (std::list<fileio>::iterator i = _files.begin(); i != _files.end(); i++) {
    if (i->fd() == fd) {
      i->funlink();
      _files.erase(i);
      _fdchange = true;
      return;
    }
  }
  scr.error("files::funlink(%d) does not exist in files", fd);
}

int
files::largestfd()
{
  int largest = 2; // Take into account stdin, stdout, stderr

  for (std::list<fileio>::iterator i = _files.begin(); i != _files.end(); i++) {
    if (i->fd() > largest)
      largest = i->fd();
  }
  return (largest);
}

inline bool
files::exists(int fd)
{
  for (std::list<fileio>::iterator i = _files.begin(); i != _files.end(); i++)
    if (i->fd() == fd)
      return true;
  return (false);
}

void
files::add(sarfile::fileio* f)
{
  if (f == nullptr) {
    scr.error("files::add() Cannot add null fileio");
    _fdchange = false;
    return;
  }
  if (f->fd() <= 2) {
    scr.error("files::add() Cannot add fd <= 2, fd=%d", f->fd());
    _fdchange = false;
    return;
  }
  if (this->exists(f->fd())) {
    scr.error("files::add: File %s fd=%d already added to open files",
              f->print().c_str(), f->fd());
    _fdchange = false;
    return;
  }
  _fdchange = true;
  _files.push_back(*f);
}

}; // namespace sarfile
