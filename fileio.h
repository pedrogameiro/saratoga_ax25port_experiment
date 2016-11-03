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

#ifndef _FILEIO_H
#define _FILEIO_H

#include <errno.h>
#include <fcntl.h>
#include <limits>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
// #include <stdio.h>
#include <string>

using namespace std;

#include "checksum.h"
#include "dirent.h"
#include "saratoga.h"
#include "screen.h"

// #include "globals.h"

/*
 * Handle debug and error output.
 * Flexible as you can set the level and also what file you want to talk to
 * defaults are stderr of course. When setting a new fp make sure we dont close
 * stdout and stderr
 */
namespace sarfile {

extern bool fexists(string);

enum rorw
{
  FILE_UNDEF = -1,
  FILE_READ = 0,
  FILE_WRITE = 1,
  FILE_EXCL = 2
};

class fileio
{
private:
  bool _ok;                         // Did the file open OK ?
  string _fname;                    // The file name
  enum rorw _rorw;                  // Open for reading or writing
  sardir::dirent _dir;              // Directory Entry for the file
  checksums::checksum _csum;        // The checksum of the file
  bool _csum_done;                  // Has the checksum been calculated
  int _fd;                          // Local file descriptor
  std::list<saratoga::buffer> _buf; // Data queued to read or write to file
  bool _ready;      // Are we ready to read/write used by select()
  bool _sequential; // Do we read/write sequenially
                    // Or do we lseek then read/write
  // This actually does a write to a file of all of the buffers
  // Only called by fflush()
  ssize_t write();

public:
  fileio(string fname, enum rorw rw);
  ~fileio() { this->clear(); };

  void clear()
  {
    this->fflush();
    close(_fd);
    _fd = -1;
    _rorw = FILE_UNDEF;
    _fname.clear();
    _buf.clear();
    _dir.clear();
    _csum.clear();
    _csum_done = false;
    _ready = false;
    _ok = false;
  }

  // Copy constructor
  fileio(const fileio& f)
  {
    _ok = f._ok;
    _fname = f._fname;
    _rorw = f._rorw;
    _dir = f._dir;
    _csum = f._csum;
    _csum_done = f._csum_done;
    _fd = f._fd;
    _buf = f._buf;
    _ready = f._ready;
  }

  fileio& operator=(const fileio& f)
  {
    _ok = f._ok;
    _fname = f._fname;
    _rorw = f._rorw;
    _dir = f._dir;
    _csum = f._csum;
    _csum_done = f._csum_done;
    _fd = f._fd;
    _buf = f._buf;
    _ready = f._ready;
    return (*this);
  }

  int funlink();
  int fd() { return (_fd); };
  string fname() { return (_fname); };

  offset_t fseek(offset_t offset);
  offset_t flen();

  // Sequential write to EOF
  ssize_t fwrite(const char*, const size_t);
  // lseek then write
  ssize_t fwrite(const char*, const size_t, const offset_t);
  // Sequenitial or lseek write
  ssize_t fwrite(const saratoga::buffer&, bool);

  // Get a string from the fileio buffers and remove it
  // from the buffers. You need to allocate the char *
  size_t fget(char*, size_t);

  void fflush()
  {
    if (this->rorw() == sarfile::FILE_WRITE ||
        this->rorw() == sarfile::FILE_EXCL)
      this->write();
  }

  // This actually does a sequential read from a file to a buffer of length
  ssize_t read(size_t);
  ssize_t read(void*, size_t);

  bool ok() { return _ok; };

  // Is this a directory
  bool isdir();

  bool isfile();

  // Is the file to be read from or written to ready for that
  // used in select() loop for DF_CLR, FD_SET, FD_ISSET
  inline bool ready() { return _ready; };

  inline bool ready(bool f)
  {
    _ready = f;
    return _ready;
  };

  // Is a file being read from or written to
  inline enum rorw rorw() { return _rorw; };

  // Return direntory entry info
  inline sardir::dirent dir() { return _dir; };
  inline sardir::dirent* dirent() { return &_dir; };

  // Return checksum entry info
  inline checksums::checksum csum() { return _csum; };

  // Reset the directory entry
  inline void setdir(const sardir::dirent& newdir) { _dir = newdir; };

  // Reset the checksum entry
  inline void setcsum(const checksums::checksum& newcsum) { _csum = newcsum; };

  // File size
  inline offset_t filesize() { return _dir.filesize(); };

  inline std::list<saratoga::buffer>* buffers() { return &_buf; };

  string print();
};

// List of the currently open files
class files
{
private:
  const size_t _max = 100;  // Maximum number of files open
  std::list<fileio> _files; // List of open files
  bool _fdchange; // We have added/removed fd for select() to get the new one
public:
  files(){};
  ~files()
  {
    _fdchange = true;
    for (std::list<fileio>::iterator i = this->begin(); i != this->end(); i++)
      i->clear();
    _files.clear();
  };

  void add(sarfile::fileio* f); // Once we opened the fileio put it in list
  void remove(int fd);          // Close the fd and remove from list
  void funlink(int fd);         // Remove the file and remove from list

  inline std::list<fileio>::iterator begin() { return (_files.begin()); };
  inline std::list<fileio>::iterator end() { return (_files.end()); };

  // What is the largest fd in the list (for select())
  int largestfd();

  // Is the fd in the current list
  inline bool exists(int fd);

  inline bool fdchange() { return _fdchange; };
  inline bool fdchange(bool x)
  {
    _fdchange = x;
    return x;
  };

  // Print all the open files info
  //	string	print();
};

} // namespace sarfile

#endif // _FILEIO_H
