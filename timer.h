/*
 * Timer.h
 *
 * Copyright (C) 2011 IBR, TU Braunschweig
 *
 * Written-by: Johannes Morgenroth <morgenroth@ibr.cs.tu-bs.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef _TIMER_H
#define _TIMER_H

//#include <time.h>
#include <chrono>
#include <string>

#include "saratoga.h"

using namespace std;
using namespace saratoga;

namespace timer_group {

class timer
{
private:
  saratoga::offset_t elapsedTime;           // tempo espera
  chrono::steady_clock::time_point begTime; // tempo inicio
  string name;

public:
  timer();
  timer(saratoga::offset_t);
  timer(string str, saratoga::offset_t t);
  // timer(string *str, saratoga::offset_t t);

  bool elapsed();
  void reset();
  bool timedout();
};

} // namespace

#endif /* TIMER_H_ */
