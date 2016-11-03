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

#include <fcntl.h>
#include <iostream>
#include <limits>
#include <list>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

using namespace std;

class mybuffer
{
  char* _b;
  size_t _len;

public:
  mybuffer(const char* b, size_t len)
  {
    _b = new char[len];
    _len = len;
    bcopy(b, _b, len);
  }

  mybuffer(mybuffer* b)
  {
    _len = b->_len;
    _b = new char[_len];
    bcopy(b->buf(), _b, _len);
  }

  // THIS IS ABSOLUTELY VITAL TO WRITE SO THAT
  // a constructor does exactly what you want
  // otherwise the compilor will do it and get it wrong!!!
  mybuffer(const mybuffer& b)
  {
    _len = b._len;
    _b = new char[_len];
    bcopy(b._b, _b, _len);
  }

  ~mybuffer()
  {
    if (_b != nullptr) {
      cout << "Freeing mybuffer " << this->print() << endl;
      delete _b;
      _b = nullptr;
    }
    _len = 0;
  }

  // Copy
  const mybuffer& operator=(const mybuffer& b1)
  {
    _len = b1._len;
    if (_b != nullptr)
      delete _b;
    _b = new char[b1._len];
    bcopy(b1._b, _b, b1._len);
    return (*this);
  }

  // Equality
  bool operator==(const mybuffer& rhs)
  {
    if (_len != rhs._len)
      return (false);
    for (size_t i = 0; i < _len; i++)
      if (_b[i] != rhs._b[i])
        return (false);
    return true;
  }

  // Inequality
  bool operator!=(const mybuffer& rhs)
  {
    if (_len != rhs._len)
      return (true);
    for (size_t i = 0; i < _len; i++)
      if (_b[i] != rhs._b[i])
        return true;
    return false;
  }

  // Add a new mybuffer to the end of this mybuffer
  const mybuffer& operator+=(const mybuffer& b1)
  {
    mybuffer tmp(_b, _len);
    if (_b != nullptr)
      delete _b;
    _len = tmp._len + b1._len;
    _b = new char[_len];
    bcopy(&tmp._b[0], &(this->_b[0]), tmp._len);
    bcopy(&b1._b[0], &(this->_b[tmp._len]), b1._len);
    return (*this);
  }

  char* buf() { return (_b); };
  size_t len() { return (_len); };

  string print()
  {
    string s;
    s.assign(_b, _len);
    return s;
  };
};

class mybuffers
{
private:
  std::list<mybuffer> _bufs;

public:
  mybuffers() { _bufs.empty(); }

  ~mybuffers()
  {
    cout << "Clearing bufs" << endl;
    _bufs.clear();
    cout << "Cleared bufs" << endl;
  }

  void add(const char* b, size_t l)
  {
    mybuffer* newb = new mybuffer(b, l);
    _bufs.push_back(*newb);
  }

  void add(const mybuffer& b)
  {
    mybuffer* newb = new mybuffer(b);
    _bufs.push_back(*newb);
  }

  void pop()
  {
    if (!_bufs.empty())
      _bufs.pop_front();
  }

  void print()
  {
    for (std::list<mybuffer>::iterator i = _bufs.begin(); i != _bufs.end(); i++)
      cout << i->print() << endl;
  }
};

void
testbufs()
{
  mybuffers bufs2;

  extern mybuffers bufs;

  string s = "X3";
  bufs.add(s.c_str(), s.length());
  s = "buffs2 1";
  bufs2.add(s.c_str(), s.length());

  s = "X4";
  bufs.add(s.c_str(), s.length());
  s = "buffs2 2";
  bufs2.add(s.c_str(), s.length());

  s = "X5";
  bufs.add(s.c_str(), s.length());
  s = "buffs2 3";
  bufs2.add(s.c_str(), s.length());

  cout << "Now buffs2 shoul be all cleared after this!!" << endl;
}

mybuffers bufs;

// Test out the checksum logic and == and != checking
#ifdef TEST2
int
main(int argc, char** argv)
{
  mybuffer ba("abc", 3);
  mybuffer bb("def", 3);
  cout << "ba=" << ba.print() << " bb=" << bb.print() << endl;

  ba += bb;
  cout << "ba += bb ba=" << ba.print() << " bb=" << bb.print() << endl;

  mybuffer bc = ba;
  cout << "bc = ba " << bc.print() << endl;

  bufs.add(ba);
  bufs.add(bb);
  bufs.add(bc);

  string s = "X1";
  bufs.add(s.c_str(), s.length());

  s = "X2";
  bufs.add(s.c_str(), s.length());

  s = "X6";
  mybuffer b6(s.c_str(), s.length());
  bufs.add(b6);

  cout << "Before testbufs" << endl;
  bufs.print();
  testbufs();
  cout << "After testbufs" << endl;
  bufs.print();
}
#endif
