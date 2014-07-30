/*
 * Copyright (c) 2013, Abdullah A. Hassan <voodooattack@hotmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 *   disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef MAP_CUSTOM_H
#define MAP_CUSTOM_H

#include "map.hpp"
#include "mapbank.hpp"

#include <boost/multi_array.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <tbb/concurrent_unordered_map.h>
#include <fstream>

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/iterators/base.hpp>
#include <boost/geometry/core/cs.hpp>

using vec3 = boost::tuple<int, int, int>;

namespace tbb {
  namespace interface5
  {
    template<>
    inline size_t tbb_hasher( const vec3& v )
    {
      std::size_t seed = 0;
      boost::hash_combine(seed, v.get<0>());
      boost::hash_combine(seed, v.get<1>());
      boost::hash_combine(seed, v.get<2>());
      return seed;
    }
  }
}

std::ostream & operator<< (std::ostream & os, const vec3 & v)
{
  os << v.get<0>() << "x" << v.get<1>() << "x" << v.get<2>();
}

namespace ADWIF
{
  class MapImpl
  {
    friend class Map;

    using clock_type = boost::chrono::steady_clock;
    using time_point = clock_type::time_point;
    using duration_type = clock_type::duration;

    struct Chunk
    {
      vec3 pos;
      uint64_t * data;
      uint64_t size;
      boost::atomic<time_point> lastAccess;
      boost::atomic_bool dirty;
      std::string fileName;
      mutable boost::shared_mutex lock;
    };

  public:
    MapImpl(Map * parent, const std::shared_ptr<class Engine> & engine, const boost::filesystem::path & mapPath,
            bool load, unsigned int chunkSizeX, unsigned int chunkSizeY, unsigned int chunkSizeZ,
            const MapCell & bgValue = MapCell());
    ~MapImpl();

    const MapCell & get(int x, int y, int z) const;
    void set(int x, int y, int z, const MapCell & cell);

    const MapCell & background() const;

    void prune(bool pruneAll = false) const;

  private:
    std::shared_ptr<Chunk> getChunk(const vec3 & index) const;
    std::string getChunkName(const vec3 & v) const;

    void loadChunk(const std::shared_ptr<Chunk> & chunk) const;
    void saveChunk(const std::shared_ptr<Chunk> & chunk) const;
    void freeChunk(const std::shared_ptr<Chunk> & chunk) const;

    void pruneTask();

  private:

    Map * myMap;
    std::weak_ptr<class Engine> myEngine;
    boost::filesystem::path myMapPath;
    unsigned int myChunkSizeX, myChunkSizeY, myChunkSizeZ;
    uint64_t myBackgroundValue;
    std::shared_ptr<MapBank> myBank;
    mutable tbb::interface5::concurrent_unordered_map<vec3, std::shared_ptr<Chunk>> myChunks;
    clock_type myClock;
    std::fstream myIndexStream;

    unsigned long int myMemThresholdMB;
    duration_type myDurationThreshold;
    duration_type myPruningInterval;
    mutable boost::atomic_bool myPruningInProgressFlag;
    boost::thread myPruneThread;
    boost::condition_variable myPruneThreadCond;
    mutable boost::mutex myPruneThreadMutex;
    boost::atomic_bool myPruneThreadQuitFlag;

  };
}

#endif // MAP_CUSTOM_H
