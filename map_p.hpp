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

#ifndef MAPPRIVATE_H
#define MAPPRIVATE_H

#include <vector>
#include <unordered_map>
#include <algorithm>

#include <openvdb/openvdb.h>

#include <boost/chrono.hpp>
#include <boost/functional/hash/extensions.hpp>
#include <boost/functional/hash/hash.hpp>

#include <boost/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>

#include "map.hpp"

namespace ovdb = openvdb::v1_1;

typedef ovdb::Vec3i Vec3Type;

namespace std
{
  template <> struct hash<Vec3Type>
  {
    size_t operator()(const Vec3Type & vec) const
    {
      size_t h = 0;
      boost::hash_combine(h, vec.x());
      boost::hash_combine(h, vec.y());
      boost::hash_combine(h, vec.z());
      return h;
    }
  };
}

namespace ADWIF
{
  typedef ovdb::tree::Tree4<uint64_t, 5, 4, 3>::Type TreeType;
  typedef ovdb::Grid<TreeType> GridType;

  class MapImpl
  {
    friend class Map;

    using clock_type = boost::chrono::steady_clock;
    using time_point = clock_type::time_point;
    using duration_type = clock_type::duration;

    struct Chunk
    {
      Vec3Type pos;
      GridType::Ptr grid;
      std::shared_ptr<GridType::Accessor> accessor;
      time_point lastAccess;
      std::string fileName;
      boost::recursive_mutex lock;
    };

  public:
    MapImpl(Map * parent, std::shared_ptr<MapBank> & bank, const std::string & mapPath, bool load, unsigned int chunkSizeX,
            unsigned int chunkSizeY, unsigned int chunkSizeZ, const MapCell & bgValue = MapCell());
    ~MapImpl();

    const MapCell & get(int x, int y, int z) const;
    void set(int x, int y, int z, const MapCell & cell);

    const MapCell & background() const;

    uint64_t getVoxelCount() const {
      uint64_t count = 0;
      boost::recursive_mutex::scoped_lock guard(myLock);
      for(auto const & i : myChunks)
        if (i.second)
        {
          boost::recursive_mutex::scoped_lock guard(i.second->lock);
          if (i.second->grid)
            count += i.second->grid->activeVoxelCount();
        }
      return count;
    }

    unsigned long int getMemUsage() const {
      unsigned long int count = 0;
      boost::recursive_mutex::scoped_lock guard(myLock);
      for(auto const & i : myChunks)
      {
        if (i.second)
        {
          boost::recursive_mutex::scoped_lock guard(i.second->lock);
          if (i.second->grid && i.second)
            count += i.second->grid->memUsage();
        }
      }
      return count;
    }

    std::shared_ptr<MapBank> bank() const;

    void prune(bool pruneAll = false) const;

  private:
    std::string getChunkName(const Vec3Type & v) const;
    std::shared_ptr<Chunk> & getChunk(int x, int y, int z) const;

    void loadChunk(std::shared_ptr<Chunk> & chunk) const;
    void saveChunk(std::shared_ptr<Chunk> & chunk) const;

    void pruneTask();

  private:


    typedef std::unordered_map<Vec3Type, std::shared_ptr<Chunk>> GridMap;

    class Map * const myMap;
    mutable GridMap myChunks;
    std::shared_ptr<MapBank> myBank;
    Vec3Type myChunkSize;
    unsigned long int myAccessTolerance;
    uint64_t myBackgroundValue;
    std::string myMapPath;
    clock_type myClock;
    mutable unsigned long int myAccessCounter;
    unsigned long int myMemThresholdMB;
    duration_type myDurationThreshold;
    mutable boost::asio::io_service myService;
    boost::thread_group myThreads;
    std::shared_ptr<boost::asio::io_service::work> myServiceLock;
    mutable boost::recursive_mutex myLock;
    mutable boost::atomic_bool myPruningInProgressFlag;

    static bool myInitialisedFlag;
  };
}
#endif // MAPPRIVATE_H
