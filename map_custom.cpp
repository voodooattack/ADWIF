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

#include "map.hpp"
#include "map_custom.hpp"
#include "engine.hpp"

#include <boost/format.hpp>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/lexical_cast.hpp>

namespace ADWIF
{
  MapImpl::MapImpl(Map * parent, const std::shared_ptr<class Engine> & engine, const boost::filesystem::path & mapPath, bool load,
                   unsigned int chunkSizeX, unsigned int chunkSizeY, unsigned int chunkSizeZ, const MapCell & bgValue):
                   myMap(parent), myEngine(engine), myMapPath(mapPath), myChunkSizeX(chunkSizeX), myChunkSizeY(chunkSizeY),
                   myChunkSizeZ(chunkSizeZ), myBackgroundValue(0), myClock(), myIndexStream(), myMemThresholdMB(2048),
                   myDurationThreshold(boost::chrono::minutes(1)), myPruningInterval(boost::chrono::seconds(10)),
                   myPruningInProgressFlag(false), myPruneThread(), myPruneThreadCond(), myPruneThreadMutex(), myPruneThreadQuitFlag(false)
  {
    if (!load)
    {
      boost::filesystem::remove_all(myMapPath);
      boost::filesystem::create_directory(myMapPath);
      myIndexStream.open((mapPath / "index").native(),
                         std::ios_base::out | std::ios_base::trunc);
      myIndexStream.close();
    }
    else
    {
      myIndexStream.open((mapPath / "index").native(),
                         std::ios_base::out | std::ios_base::app);
      myIndexStream.close();
    }

    myIndexStream.open((mapPath / "index").native(),
                       std::ios_base::binary | std::ios_base::in | std::ios_base::out);

    myBank.reset(new MapBank(myIndexStream));
    myBackgroundValue = myBank->put(bgValue);

    myPruningInProgressFlag.store(false);
    myPruneThreadQuitFlag.store(false);
    myPruneThread = boost::thread(boost::bind(&MapImpl::pruneTask, this));
  }

  MapImpl::~MapImpl() {

  }

  const MapCell & MapImpl::get(int x, int y, int z) const {
    int chunkX = x / myChunkSizeX, chunkY = y / myChunkSizeY, chunkZ = z / myChunkSizeZ;
    int localX = x % myChunkSizeX, localY = y % myChunkSizeY, localZ = z % myChunkSizeZ;
    std::shared_ptr<Chunk> chunk = getChunk(vec3(chunkX, chunkY, chunkZ));
    uint64_t hash = chunk->data[localZ * myChunkSizeX * myChunkSizeY + localY * myChunkSizeY + localX];
    chunk->lock.unlock_shared();
    return myBank->get(hash);
  }

  void MapImpl::set(int x, int y, int z, const MapCell & cell) {
    int chunkX = x / myChunkSizeX, chunkY = y / myChunkSizeY, chunkZ = z / myChunkSizeZ;
    int localX = x % myChunkSizeX, localY = y % myChunkSizeY, localZ = z % myChunkSizeZ;
    std::shared_ptr<Chunk> chunk = getChunk(vec3(chunkX, chunkY, chunkZ));
    chunk->data[localZ * myChunkSizeX * myChunkSizeY + localY * myChunkSizeY + localX] = myBank->put(cell);
    chunk->dirty = true;
    chunk->lock.unlock_shared();
  }

  const MapCell & MapImpl::background() const {
    return myBank->get(myBackgroundValue);
  }

  void MapImpl::pruneTask()
  {
    while (!myPruneThreadQuitFlag)
    {
      boost::unique_lock<boost::mutex> lock(myPruneThreadMutex);
      myPruneThreadCond.wait_for(lock, myPruningInterval);
      if (!myPruneThreadQuitFlag)
        prune(false);
      else
        break;
    }
  }

  void MapImpl::prune(bool pruneAll) const {
    bool isPruning = false;

    // Use an atomic bool for locking instead of a Mutex because pruneTask() uses a Mutex for timing.
    while (!myPruningInProgressFlag.compare_exchange_weak(isPruning, true))
      boost::this_thread::sleep_for(boost::chrono::microseconds(50));

    std::vector<std::pair<vec3, std::shared_ptr<Chunk>>> accessTimesSorted;

    accessTimesSorted.assign(myChunks.begin(), myChunks.end());

    accessTimesSorted.erase(std::remove_if(accessTimesSorted.begin(), accessTimesSorted.end(), [](const std::pair<vec3, std::shared_ptr<Chunk>> & item) {
      return !item.second || !item.second->data;
    }), accessTimesSorted.end());

    std::sort(accessTimesSorted.begin(), accessTimesSorted.end(),
              [](const std::pair<vec3, std::shared_ptr<Chunk>> & first,
                  const std::pair<vec3, std::shared_ptr<Chunk>> & second)
              {
                return first.second->lastAccess.load() < second.second->lastAccess.load();
              });

    const std::size_t itemMem = (myChunkSizeX * myChunkSizeY * myChunkSizeZ * sizeof(uint64_t)) / (1024 * 1024);
    std::size_t memUse = 0;

    memUse = std::accumulate(myChunks.begin(), myChunks.end(), memUse,
                            [&](unsigned long int sum,
                                const std::pair<vec3, std::shared_ptr<Chunk>> second)
                            {
                              return sum + (second.second->data ? second.second->size / (1024 * 1024) : 0);
                            });;

    if (memUse > myMemThresholdMB)
      myEngine.lock()->log("Map"), memUse, "MB of memory in use, will attempt to free ", memUse - myMemThresholdMB, "MB";
    else
      myEngine.lock()->log("Map"), memUse, "MB of memory in use";

    unsigned long int freed = 0;
    unsigned int posted = 0;

    {
      auto i = accessTimesSorted.begin();
      while (i != accessTimesSorted.end())
      {
        duration_type dur(myClock.now() - i->second->lastAccess.load());
        if (pruneAll || dur > myDurationThreshold ||
          (memUse > myMemThresholdMB))
        {
          if (i->second->dirty)
          {
            if (pruneAll)
              myEngine.lock()->log("Map"), "scheduling save operation for ", i->second->pos;
            else if (memUse > myMemThresholdMB)
              myEngine.lock()->log("Map"), "scheduling save operation for ", i->second->pos, " to free ",
              i->second->size / (1024 * 1024), "MB of memory";
            else if (dur > myDurationThreshold)
              myEngine.lock()->log("Map"), "scheduling save operation for ", i->second->pos, ", last accessed in ", dur;

            myEngine.lock()->service().post(boost::bind(&MapImpl::saveChunk, this, i->second));
          }
          else
            freeChunk(i->second);

          posted++;

          if (!pruneAll && myMemThresholdMB)
          {
            freed += itemMem;
            if (memUse - freed < myMemThresholdMB * 0.70)
              break;
          }
          i = accessTimesSorted.erase(i);
        }
        else ++i;
      }
    }
    if (freed)
      myEngine.lock()->log("Map"), "scheduled ", freed, "MB to be freed";
    myBank->prune(pruneAll);
    myPruningInProgressFlag.store(false);
  }

  std::shared_ptr<MapImpl::Chunk> MapImpl::getChunk(const vec3 & index) const {
    auto chunk = myChunks.find(index);
    if (chunk != myChunks.end())
    {
      chunk->second->lock.lock_shared();
      chunk->second->lastAccess = myClock.now();
      if (!chunk->second->data && boost::filesystem::exists(myMapPath / chunk->second->fileName))
        loadChunk(chunk->second);
      else if (!chunk->second->data) // a chunk was created but was never edited/saved or file is missing
      {
        chunk->second->size = sizeof(uint64_t) * myChunkSizeX * myChunkSizeY * myChunkSizeZ;
        chunk->second->data = new uint64_t[chunk->second->size];
        std::fill_n(chunk->second->data, myChunkSizeX * myChunkSizeY * myChunkSizeZ, myBackgroundValue);
        chunk->second->dirty = true;
        myEngine.lock()->log("Map"), "created missing chunk ", chunk->second->pos;
      }
      return chunk->second;
    }
    else
    {
      std::shared_ptr<Chunk> chunk(new Chunk);
      chunk->lock.lock_shared();
      chunk->pos = index;
      chunk->fileName = getChunkName(index);
      chunk->lastAccess = myClock.now();
      if (boost::filesystem::exists(myMapPath / chunk->fileName))
        loadChunk(chunk);
      else
      {
        chunk->size = sizeof(uint64_t) * myChunkSizeX * myChunkSizeY * myChunkSizeZ;
        chunk->data = new uint64_t[chunk->size];
        std::fill_n(chunk->data, myChunkSizeX * myChunkSizeY * myChunkSizeZ, myBackgroundValue);
        chunk->dirty = true;
        myEngine.lock()->log("Map"), "created ", chunk->pos;
      }
      return myChunks[index] = chunk;
    }
  }

  std::string MapImpl::getChunkName(const vec3 & v) const
  {
    return boost::str(boost::format("%d.%d.%d") % v.get<0>() % v.get<1>() % v.get<2>());
  }

  void MapImpl::loadChunk(std::shared_ptr< MapImpl::Chunk > & chunk) const
  {
    // This expects the chunk to be lock_shared() before loading
    chunk->lock.lock_upgrade();
    if (boost::filesystem::exists(myMapPath / chunk->fileName))
    {
      myEngine.lock()->log("Map"), "loading ", chunk->pos;
      boost::iostreams::file_source fs((myMapPath / chunk->fileName).native());
      boost::iostreams::filtering_istream os;
      os.push(boost::iostreams::bzip2_decompressor());
      os.push(fs);
      boost::archive::binary_iarchive ia(os);
      if (!chunk->data)
      {
        chunk->size = sizeof(uint64_t) * myChunkSizeX * myChunkSizeY * myChunkSizeZ;
        chunk->data = (uint64_t*)malloc(chunk->size);
      }
      ia.load_binary((void*)chunk->data, chunk->size);
      chunk->dirty = false;
      myEngine.lock()->log("Map"), "loaded ", chunk->pos;
    }
    chunk->lock.unlock_upgrade_and_lock_shared();
  }

  void MapImpl::saveChunk(std::shared_ptr< MapImpl::Chunk > & chunk) const
  {
    boost::unique_lock<boost::shared_mutex> guard(chunk->lock);
    myEngine.lock()->log("Map"), "saving ", chunk->pos;
    if (chunk->dirty && chunk->data)
    {
      boost::iostreams::file_sink fs((myMapPath / chunk->fileName).native());
      boost::iostreams::filtering_ostream os;
      os.push(boost::iostreams::bzip2_compressor());
      os.push(fs);
      boost::archive::binary_oarchive oa(os);
      oa.save_binary((void*)chunk->data, chunk->size);
      chunk->dirty = false;
      myEngine.lock()->log("Map"), "saved ", chunk->pos;
    }
    duration_type dur(myClock.now() - chunk->lastAccess.load());
    if (dur > myDurationThreshold)
      freeChunk(chunk);
  }

  void MapImpl::freeChunk(std::shared_ptr< MapImpl::Chunk > & chunk) const
  {
    boost::unique_lock<boost::shared_mutex> guard(chunk->lock);
    delete[] chunk->data;
    chunk->data = nullptr;
    myEngine.lock()->log("Map"), "unloaded ", chunk->pos;
  }

  Map::Map(const std::shared_ptr<class Engine> & engine, const boost::filesystem::path & mapPath, bool load, unsigned int chunkSizeX,
          unsigned int chunkSizeY, unsigned int chunkSizeZ, const MapCell & bgValue): myImpl(nullptr)
          {
            myImpl = new MapImpl(this, engine, mapPath, load,
                                  chunkSizeX, chunkSizeY, chunkSizeZ, bgValue);
          }

  Map::~Map() { delete myImpl; }

  const MapCell & Map::get(int x, int y, int z) const { return myImpl->get(x,y,z); }
  void Map::set(int x, int y, int z, const MapCell & cell) { myImpl->set(x, y, z, cell); }
  const MapCell & Map::background() const { return myImpl->background(); }
  void Map::save() const { myImpl->prune(true); }
  void Map::prune() const { myImpl->prune(false); }


}
