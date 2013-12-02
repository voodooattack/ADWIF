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

#include "map.hpp"
#include "map_p.hpp"

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/lexical_cast.hpp>

#include <openvdb/io/Stream.h>

namespace iostreams = boost::iostreams;

namespace ADWIF
{
  MapImpl::MapImpl(ADWIF::Map * parent, boost::asio::io_service & service, const std::shared_ptr< ADWIF::MapBank > & bank, const std::string & mapPath, bool load, unsigned int chunkSizeX, unsigned int chunkSizeY, unsigned int chunkSizeZ, const ADWIF::MapCell & bgValue):
    myMap(parent), myChunks(), myBank(bank), myChunkSize(chunkSizeX, chunkSizeY, chunkSizeZ),
    myAccessTolerance(200000), myBackgroundValue(0), myMapPath(mapPath), myClock(),
    myAccessCounter(0), myMemThresholdMB(2048), myDurationThreshold(boost::chrono::minutes(1)),
    myPruningInterval(boost::chrono::seconds(10)), myService(service),
    myLock(), myPruningInProgressFlag()/*, myPruneTimer(myService)*/
  {
    if (!myInitialisedFlag)
    {
      ovdb::initialize();
      GridType::registerGrid();
      myInitialisedFlag = true;
    }
    myBackgroundValue = myBank->put(bgValue);
    if (!load)
    {
      boost::filesystem::remove_all(myMapPath);
      boost::filesystem::create_directory(myMapPath);
    }
    myPruningInProgressFlag.store(false);
    myPruneThreadQuitFlag.store(false);
    myPruneThread = boost::thread(boost::bind(&MapImpl::pruneTask, this));
  }

  MapImpl::~MapImpl()
  {
    myPruneThreadQuitFlag.store(true);
    myPruneThread.join();
  }

  const MapCell & MapImpl::get(int x, int y, int z) const
  {
    std::shared_ptr<Chunk> chunk = getChunk(x, y, z);
    boost::upgrade_lock<boost::shared_mutex> guard(chunk->lock);
    if(!chunk->accessor)
    {
      loadChunk(chunk, guard);
    }
//     boost::shared_lock<boost::shared_mutex> guard(chunk->lock);
//     if (myAccessCounter++ % myAccessTolerance == 0)
//       prune(false);
//     while(chunk->writerCount)
//       boost::this_thread::sleep_for(boost::chrono::microseconds(50));
//     chunk->readerCount++;
    const MapCell & value = myBank->get(chunk->accessor->getValue(
      ovdb::Coord(x % myChunkSize.x(), y % myChunkSize.y(), z % myChunkSize.z())));
//     chunk->readerCount--;
    return value;
  }

  void MapImpl::set(int x, int y, int z, const MapCell & cell)
  {
    if (!myPruneThread.joinable())
      myPruneThread.start_thread();

    uint64_t hash = myBank->put(cell);

    std::shared_ptr<Chunk> chunk = getChunk(x, y, z);
    boost::upgrade_lock<boost::shared_mutex> guard(chunk->lock);
    if(!chunk->accessor)
      loadChunk(chunk, guard);
    boost::upgrade_to_unique_lock<boost::shared_mutex> lock(guard);
//     while(chunk->readerCount)
//       boost::this_thread::sleep_for(boost::chrono::microseconds(50));
//     chunk->writerCount++;
    if (hash == myBackgroundValue)
      chunk->accessor->setValueOff(
        ovdb::Coord(x % myChunkSize.x(), y % myChunkSize.y(), z % myChunkSize.z()), hash);
    else
      chunk->accessor->setValue(
        ovdb::Coord(x % myChunkSize.x(), y % myChunkSize.y(), z % myChunkSize.z()), hash);
//     chunk->writerCount--;

    chunk->dirty = true;

//     if (!myChunks.empty() && (myAccessCounter++ % myAccessTolerance == 0))
//       prune(false);
  }

  void MapImpl::pruneTask()
  {
    while (!myPruneThreadQuitFlag)
    {
      boost::this_thread::sleep_for(myPruningInterval);
      prune(false);
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
                            [](const std::pair<Vec3Type, std::shared_ptr<Chunk>> pair)
                            {
                              //if (!pair.second || !pair.second->grid)
                                // std::cerr << "discarding (empty): " << pair.second->pos << std::endl;
                              return !pair.second || !pair.second->grid;
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

    std::map<Vec3Type, std::size_t> memoryMap;

    if (!pruneAll && myMemThresholdMB)
    {
      std::transform(accessTimesSorted.begin(), accessTimesSorted.end(),
                     std::inserter(memoryMap, memoryMap.begin()),
                     [](const std::pair<Vec3Type, std::shared_ptr<Chunk>> pair) {
                       boost::upgrade_lock<boost::shared_mutex> guard(pair.second->lock);
                       boost::upgrade_to_unique_lock<boost::shared_mutex> lock(guard);
                       if (guard.owns_lock())
                         return std::make_pair(pair.first,
                                               pair.second->grid ? pair.second->grid->memUsage() : 0);
                       else
                         return std::make_pair(pair.first, (std::size_t)0);
                     });

      memUse = std::accumulate(memoryMap.begin(), memoryMap.end(), memUse,
                               [](unsigned long int sum,
                                  const std::pair<Vec3Type, std::size_t> second)
                              {
                                 return sum + second.second;
                              });
    }

    memUse /= (1024 * 1024);

    unsigned long int freed = 0;
    unsigned int posted = 0;

    {
      auto i = accessTimesSorted.begin();
      while (i != accessTimesSorted.end())
      {
        // std::cerr << "pruning: " << i->second->pos << std::endl;
        if (pruneAll || myClock.now() - i->second->lastAccess.load() > myDurationThreshold ||
            (memUse > myMemThresholdMB))
        {
          // std::cerr << "scheduling save operation for: " << i->second->pos << std::endl;
          if (i->second->dirty)
            myService.dispatch(boost::bind(&MapImpl::saveChunk, this, i->second));
          else
          {
            i->second->grid.reset();
            i->second->accessor.reset();
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

    myPruningInProgressFlag.store(false);
  }

  std::string MapImpl::getChunkName(const Vec3Type & v) const
  {
    return boost::str(boost::format("%i.%i.%i") % v.x() % v.y() % v.z());
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
// //       newChunk->readerCount.store(0);
// //       newChunk->writerCount.store(0);
      return myChunks[vec] = newChunk;
    }
  }

  void MapImpl::loadChunk(std::shared_ptr<Chunk> & chunk, boost::upgrade_lock<boost::shared_mutex> & guard) const
  {
    // std::cerr << "loading: " << chunk->pos << std::endl;
//     boost::upgrade_lock<boost::shared_mutex> guard(chunk->lock);
    boost::upgrade_to_unique_lock<boost::shared_mutex> lock(guard);
    if (boost::filesystem::exists(myMapPath + dirSep + chunk->fileName) &&
        boost::filesystem::file_size(myMapPath + dirSep + chunk->fileName))
    {
      iostreams::file_source fs(myMapPath + dirSep + chunk->fileName);
      iostreams::filtering_istream is;
      is.push(iostreams::bzip2_decompressor());
      is.push(fs);
      ovdb::io::Stream ss(is);
      ss.setCompressionEnabled(false);
      ovdb::GridPtrVecPtr vc = ss.getGrids();
      chunk->grid = ovdb::gridPtrCast<GridType>(vc->operator[](0));
      // std::cerr << "loaded: " << chunk->pos << std::endl;
    }
    else
    {
      chunk->grid = GridType::create(myBackgroundValue);
      chunk->grid->setName(chunk->fileName);
      // std::cerr << "created: " << chunk->pos << std::endl;
    }
    chunk->accessor.reset(new GridType::Accessor(chunk->grid->getAccessor()));
    chunk->lastAccess = myClock.now();
    chunk->dirty = false;
  }

  void MapImpl::saveChunk(std::shared_ptr<Chunk> & chunk) const
  {
    if (!chunk->dirty)
      return;
    // std::cerr << "saving: " << chunk->pos << std::endl;
    boost::upgrade_lock<boost::shared_mutex> guard(chunk->lock);
    boost::upgrade_to_unique_lock<boost::shared_mutex> lock(guard);
    if (!chunk->grid)
      return;
    iostreams::file_sink fs(myMapPath + dirSep + chunk->fileName);
    iostreams::filtering_ostream os;
    os.push(iostreams::bzip2_compressor());
    os.push(fs);
    ovdb::io::Stream ss;
    ss.setCompressionEnabled(false);
    ovdb::GridPtrVec vc = { chunk->grid };
    ss.write(os, vc);
    chunk->accessor.reset();
    chunk->grid.reset();
    chunk->dirty = false;
    // std::cerr << "saved: " << chunk->pos << std::endl;
  }

  const MapCell & MapImpl::background() const { return myBank->get(myBackgroundValue); }

  std::shared_ptr<MapBank> MapImpl::bank() const { return myBank; }

  Map::Map(boost::asio::io_service & service, const std::shared_ptr<MapBank> & bank,
           const std::string & mapPath, bool load, unsigned int chunkSizeX,
           unsigned int chunkSizeY, unsigned int chunkSizeZ, const MapCell & bgValue): myImpl(nullptr)
  {
    myImpl = new MapImpl(this, service, bank, mapPath, load,
                         chunkSizeX, chunkSizeY, chunkSizeZ, bgValue);
  }

  Map::~Map() { delete myImpl; }

  const MapCell & Map::get(int x, int y, int z) const { return myImpl->get(x,y,z); }
  void Map::set(int x, int y, int z, const MapCell & cell) { myImpl->set(x, y, z, cell);}
  const MapCell & Map::background() const { return myImpl->background(); }
  std::shared_ptr< MapBank > Map::bank() const { return myImpl->bank(); }
  void Map::prune() const { myImpl->prune(false); }
  void Map::save() const { myImpl->prune(true); }

  bool MapImpl::myInitialisedFlag = false;
}
