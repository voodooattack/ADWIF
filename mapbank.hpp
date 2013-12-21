/*  Copyright (c) 2013, Abdullah A. Hassan <voodooattack@hotmail.com>
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 *  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 *  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MAPBANK_H
#define MAPBANK_H

#include "mapcell.hpp"

#include <algorithm>
#include <chrono>
#include <unordered_map>

#include <boost/thread/recursive_mutex.hpp>

namespace ADWIF
{
  class MapBank
  {
    using clock_type = std::chrono::steady_clock;
    using time_point = clock_type::time_point;
    using duration_type = clock_type::duration;

  public:
    MapBank(std::iostream & stream);

    const MapCell & get(uint64_t hash);
    uint64_t put(const MapCell & cell);
    void prune(bool pruneAll = false);

  private:
    MapCell loadCell(uint64_t hash);

  private:
    std::iostream & myStream;
    boost::recursive_mutex myMutex;
    std::unordered_map<uint64_t, MapCell> myCache;
    std::unordered_map<uint64_t, time_point> myAccessTimes;
    clock_type myClock;
  };
}

#endif // MAPBANK_H
