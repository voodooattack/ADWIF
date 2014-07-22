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

#include "engine.hpp"
#include "map_field3d.hpp"
#include "map.hpp"
#include "mapbank.hpp"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include <fstream>
#include <memory>
#include <climits>

namespace ADWIF
{
  bool MapImpl::myInitialisedFlag;

  MapImpl::MapImpl(Map * parent, const std::shared_ptr<Engine> & engine, const boost::filesystem::path & mapPath,
                   bool load, unsigned int chunkSizeX, unsigned int chunkSizeY, unsigned int chunkSizeZ,
                   const MapCell & bgValue):
    myMap(parent), myEngine(engine), myMapPath(mapPath), myIndexStream(), myBank(), myChunkSize(chunkSizeX, chunkSizeY, chunkSizeZ),
    myBackgroundValue(0), myChunks(), myLock(), myClock(), myMemThresholdMB(2048), myDurationThreshold(boost::chrono::minutes(1)),
    myPruningInterval(boost::chrono::seconds(10)), myPruningInProgressFlag(false), myPruneThread(), myPruneThreadCond(),
    myPruneThreadMutex(), myPruneThreadQuitFlag(false)
  {
    if (!myInitialisedFlag)
    {
      F3D::initIO();
      F3D::SparseFileManager::singleton().setLimitMemUse(true);
      F3D::SparseFileManager::singleton().setMaxMemUse(1024);
      myInitialisedFlag = true;
    }

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

  MapImpl::~MapImpl() { }

  const MapCell & MapImpl::get(int x, int y, int z) const
  {
    std::shared_ptr<Chunk> chunk = getChunk(x, y, z);
    boost::upgrade_lock<boost::shared_mutex> guard(chunk->lock);
    if(!chunk->field)
      loadChunk(chunk, guard);
    const MapCell & value = myBank->get(
      chunk->field->fastValue((x%myChunkSize.x+myChunkSize.x)%myChunkSize.x,
                              (y%myChunkSize.y+myChunkSize.y)%myChunkSize.y,
                              (z%myChunkSize.z+myChunkSize.z)%myChunkSize.z));
    return value;
  }

  void MapImpl::set(int x, int y, int z, const MapCell & cell)
  {
    if (!myPruneThread.joinable())
      myPruneThread.start_thread();
    uint64_t hash = myBank->put(cell);
    std::shared_ptr<Chunk> chunk = getChunk(x, y, z);
    boost::upgrade_lock<boost::shared_mutex> guard(chunk->lock);
    if(!chunk->field)
      loadChunk(chunk, guard);
    boost::upgrade_to_unique_lock<boost::shared_mutex> lock(guard);
    chunk->field->fastLValue((x%myChunkSize.x+myChunkSize.x)%myChunkSize.x,
                             (y%myChunkSize.y+myChunkSize.y)%myChunkSize.y,
                             (z%myChunkSize.z+myChunkSize.z)%myChunkSize.z) = hash;
    chunk->dirty = true;
//     if (!myChunks.empty() && (myAccessCounter++ % myAccessTolerance == 0))
//       prune(false);
  }

  const MapCell & MapImpl::background() const
  {
    return myBank->get(myBackgroundValue);
  }

  std::string MapImpl::getChunkName(const Vec3Type & v) const
  {
    return boost::str(boost::format("%i.%i.%i") % v.x % v.y % v.z);
  }

  std::shared_ptr<MapImpl::Chunk> & MapImpl::getChunk(int x, int y, int z) const
  {
    Vec3Type vec(x, y, z);
    vec /= myChunkSize;

    boost::recursive_mutex::scoped_lock guard(myLock);

    if (myChunks.find(vec) != myChunks.end())
    {
      myChunks[vec]->lastAccess = myClock.now();
      return myChunks[vec];
    }
    else
    {
      std::shared_ptr<Chunk> newChunk;
      newChunk.reset(new Chunk);
      newChunk->pos = vec;
      newChunk->fileName = getChunkName(vec);
      newChunk->dirty = false;
      newChunk->lastAccess = myClock.now();
      return myChunks[vec] = newChunk;
    }
  }

  void MapImpl::loadChunk(std::shared_ptr<Chunk> & chunk, boost::upgrade_lock<boost::shared_mutex> & guard) const
  {
    boost::upgrade_to_unique_lock<boost::shared_mutex> lock(guard);
    boost::filesystem::path path = myMapPath / chunk->fileName;
    if (boost::filesystem::exists(path) &&
      boost::filesystem::file_size(path))
    {
      myEngine.lock()->log("Map"), "loading ", chunk->pos;
      F3D::Field3DInputFile in;
      in.open(path.native());
      FieldType::Vec vec = in.readScalarLayersAs<F3D::DenseField, uint64_t>();
      chunk->field = vec.back();
      myEngine.lock()->log("Map"), "loaded ", chunk->pos;
    }
    else
    {
      chunk->field = new FieldType;
      chunk->field->setSize(myChunkSize);
      chunk->field->clear(myBackgroundValue);
      chunk->field->name = chunk->fileName;
      chunk->field->attribute = "terrain";
      myEngine.lock()->log("Map"), "created ", chunk->pos;
    }
    chunk->lastAccess = myClock.now();
    chunk->dirty = false;
  }


  void MapImpl::saveChunk(std::shared_ptr<Chunk> & chunk) const
  {
    boost::upgrade_lock<boost::shared_mutex> guard(chunk->lock);
    boost::upgrade_to_unique_lock<boost::shared_mutex> lock(guard);
    if (!chunk->field)
      return;
    if (chunk->dirty)
    {
      myEngine.lock()->log("Map"), "saving ", chunk->pos;
      boost::filesystem::path path = myMapPath / chunk->fileName;
      F3D::Field3DOutputFile of;
      of.create(path.native());
      of.writeScalarLayer<uint64_t>(chunk->field);
      of.close();
    } else
      myEngine.lock()->log("Map"), "unloading ", chunk->pos;
    chunk->field.reset();
    chunk->dirty = false;
    myEngine.lock()->log("Map"), "saved ", chunk->pos;
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

  void MapImpl::prune(bool pruneAll) const
  {
    bool isPruning = false;

    while (!myPruningInProgressFlag.compare_exchange_weak(isPruning, true))
      boost::this_thread::sleep_for(boost::chrono::microseconds(50));

    std::vector<std::pair<Vec3Type, std::shared_ptr<Chunk>>> accessTimesSorted;
    {
      boost::recursive_mutex::scoped_lock guard(myLock);
      accessTimesSorted.assign(myChunks.begin(), myChunks.end());
    }

    accessTimesSorted.erase(std::remove_if(accessTimesSorted.begin(), accessTimesSorted.end(),
                                           [&](const std::pair<Vec3Type, std::shared_ptr<Chunk>> pair)
                                           {
                                             //if (!pair.second || !pair.second->grid)
                                             //  myEngine.lock()->log(), "discarding (empty): ", pair.second->pos;
                                             return !pair.second || !pair.second->field;
                                           }), accessTimesSorted.end());

    {
      boost::recursive_mutex::scoped_lock guard(myLock);
      std::sort(accessTimesSorted.begin(), accessTimesSorted.end(),
                [](const std::pair<Vec3Type, std::shared_ptr<Chunk>> & first,
                   const std::pair<Vec3Type, std::shared_ptr<Chunk>> & second)
                {
                  return first.second->lastAccess.load() < second.second->lastAccess.load();
                });
    }

    std::size_t memUse = 0;

    std::unordered_map<Vec3Type, std::size_t> memoryMap;

    if (!pruneAll && myMemThresholdMB)
    {
      std::transform(accessTimesSorted.begin(), accessTimesSorted.end(),
                     std::inserter(memoryMap, memoryMap.begin()),
                     [](const std::pair<Vec3Type, std::shared_ptr<Chunk>> pair) {
                       boost::upgrade_lock<boost::shared_mutex> guard(pair.second->lock);
                       boost::upgrade_to_unique_lock<boost::shared_mutex> lock(guard);
                       return std::make_pair(pair.first,
                                             pair.second->field ? pair.second->field->memSize() : 0);
                     });

      memUse = std::accumulate(memoryMap.begin(), memoryMap.end(), memUse,
                               [](unsigned long int sum,
                                  const std::pair<Vec3Type, std::size_t> second)
                               {
                                 return sum + second.second;
                               });
    }

    memUse /= (1024 * 1024);

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
        boost::upgrade_lock<boost::shared_mutex> guard(i->second->lock);
        boost::upgrade_to_unique_lock<boost::shared_mutex> lock(guard);
        duration_type dur(myClock.now() - i->second->lastAccess.load());
        if (pruneAll || dur > myDurationThreshold ||
          (memUse > myMemThresholdMB))
        {
          if (i->second->dirty)
          {
            if (pruneAll)
              myEngine.lock()->log("Map"), "scheduling save operation for ", i->second->pos;
            else if (memUse > myMemThresholdMB)
              myEngine.lock()->log("Map"), "scheduling save operation for ", i->second->pos, " to free",
              memoryMap.find(i->first)->second / (1024 * 1024), "MB of memory";
            else if (dur > myDurationThreshold)
              myEngine.lock()->log("Map"), "scheduling save operation for ", i->second->pos, ", last accessed in ", dur;

            myEngine.lock()->service().post(boost::bind(&MapImpl::saveChunk, this, i->second));
          }
          else
          {
            myEngine.lock()->log("Map"), "unloading ", i->second->pos;
            i->second->field.reset();
          }
          posted++;
          if (!pruneAll && myMemThresholdMB)
          {
            freed += memoryMap.find(i->first)->second / (1024 * 1024);
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

  Map::Map(const std::shared_ptr<class Engine> & engine, const boost::filesystem::path & mapPath, bool load, unsigned int chunkSizeX,
           unsigned int chunkSizeY, unsigned int chunkSizeZ, const MapCell & bgValue): myImpl(nullptr)
  {
    myImpl = new MapImpl(this, engine, mapPath, load,
                        chunkSizeX, chunkSizeY, chunkSizeZ, bgValue);
  }

  Map::~Map() { delete myImpl; }

  const MapCell & Map::get(int x, int y, int z) const { return myImpl->get(x,y,z); }
  void Map::set(int x, int y, int z, const MapCell & cell) { myImpl->set(x, y, z, cell);}
  const MapCell & Map::background() const { return myImpl->background(); }
  void Map::save() const { myImpl->prune(true); }
  void Map::prune() const { myImpl->prune(false); }
}
