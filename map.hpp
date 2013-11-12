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

#ifndef MAP_H
#define MAP_H

#include <vector>
#include <cstdint>
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <memory>

#include "fileutils.hpp"
#include "mapbank.hpp"

#include <boost/functional/hash/extensions.hpp>
#include <boost/functional/hash/hash.hpp>

namespace ADWIF
{
  class Map
  {
  public:
    Map(const std::shared_ptr<class MapBank> & bank, const std::string & mapPath, bool load, unsigned int chunkSizeX,
        unsigned int chunkSizeY, unsigned int chunkSizeZ, const MapCell & bgValue = MapCell());
    ~Map();

    const MapCell & get(int x, int y, int z) const;
    void set(int x, int y, int z, const MapCell & cell);

    const MapCell & background() const;

    std::shared_ptr<class MapBank> bank() const;

    void prune() const;
    void save() const;

  private:
    class MapImpl * myImpl;
  };
}

#endif // MAP_H
