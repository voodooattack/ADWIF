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
  MapImpl::MapImpl(Map * parent, std::shared_ptr<MapBank> & bank,
                   const std::string & mapPath, bool load, unsigned int chunkSizeX,
                   unsigned int chunkSizeY, unsigned int chunkSizeZ,
                   const MapCell & bgValue):
    myMap(parent), myChunks(), myBank(bank), myChunkSize(chunkSizeX, chunkSizeY, chunkSizeZ),
    myAccessTolerance(200000), myBackgroundValue(0), myMapPath(mapPath), myClock(),
    myAccessCounter(0), myMemThresholdMB(800), myDurationThreshold(boost::chrono::minutes(1)),
    myService(), myThreads(), myServiceLock()
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
    myServiceLock.reset(new boost::asio::io_service::work(myService));
    int nthreads = boost::thread::hardware_concurrency() - 1;
    /* if (nthreads < 1) */ nthreads = 2;
    while(nthreads--)
      myThreads.create_thread(boost::bind(&boost::asio::io_service::run, &myService));
    myService.post(boost::bind(&MapImpl::pruneTask, this));
  }

  MapImpl::~MapImpl()
  {
    myServiceLock.reset();
    myService.stop();
    myThreads.join_all();
  }

  const MapCell & MapImpl::get(int x, int y, int z) const
  {
    std::shared_ptr<GridType::Accessor> grid = getChunk(x, y, z)->accessor;
    if (myAccessCounter++ % myAccessTolerance == 0)
      prune(false);
    return myBank->get(grid->getValue(ovdb::Coord(x, y, z)));
  }

  void MapImpl::set(int x, int y, int z, const MapCell & cell)
  {
    uint64_t hash = myBank->put(cell);

    std::shared_ptr<Chunk> grid = getChunk(x, y, z);

    if (hash == myBackgroundValue)
      grid->accessor->setValueOff(ovdb::Coord(x, y, z), hash);
    else
      grid->accessor->setValue(ovdb::Coord(x, y, z), hash);

   myAccessCounter++;

    if (!myChunks.empty() && (myAccessCounter % myAccessTolerance == 0))
    {
      prune(false);
    }
  }

  void MapImpl::pruneTask()
  {
    //prune(false);
    //boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
    //myService.post(boost::bind(&MapImpl::pruneTask, this));
  }

  void MapImpl::prune(bool pruneAll) const
  {
    if (myChunks.size() <= 1 && !pruneAll) return;

    boost::recursive_mutex::scoped_lock guard(myLock);

    std::vector<std::pair<ovdb::Vec3I, std::shared_ptr<Chunk>>> accessTimesSorted;
    accessTimesSorted.assign(myChunks.begin(), myChunks.end());

    accessTimesSorted.erase(std::remove_if(accessTimesSorted.begin(), accessTimesSorted.end(),
                            [](const std::pair<ovdb::Vec3I, std::shared_ptr<Chunk>> pair)
                            {
                              return !pair.second;
                            }));

    std::sort(accessTimesSorted.begin(), accessTimesSorted.end(),
              [](const std::pair<ovdb::Vec3I, std::shared_ptr<Chunk>> & first,
                 const std::pair<ovdb::Vec3I, std::shared_ptr<Chunk>> & second)
    {
      return first.second->lastAccess < second.second->lastAccess;
    });

    unsigned long int memUse = 0;

    if (myMemThresholdMB)
    {
      for(auto const & i : accessTimesSorted)
      {
        if (i.second)
        {
          boost::mutex::scoped_lock guard(i.second->lock);
          if (i.second->grid && i.second)
            memUse += i.second->grid->memUsage();
        }
      }
    }

    memUse /= (1024 * 1024);

//    unsigned long int freed = memUse;

    {
      auto i = accessTimesSorted.begin();
      while (i != accessTimesSorted.end())
      {
        if (pruneAll || myClock.now() - i->second->lastAccess > myDurationThreshold || (memUse > myMemThresholdMB))
        {
          auto item = myChunks.find(i->first);
          myChunks.erase(item);
          boost::mutex::scoped_lock guard(item->second->lock);
          if (item->second->grid)
          {
            myService.post(boost::bind(&MapImpl::saveChunk, this, item->second));
            i = accessTimesSorted.erase(i);
/*            if (myMemThresholdMB)
            {
              freed -= item->second->grid->memUsage();
              if (freed <= myMemThresholdMB)
                break;
            } */
          } else ++i;
        } else ++i;
      }
    }

    myBank->prune(pruneAll);

    if(memUse > myMemThresholdMB)
      myService.poll();
  }

  std::string MapImpl::getChunkName(const ovdb::Vec3I & v) const
  {
    unsigned long int hash = 0; // myBackgroundValue; // if this is enabled it'll invalidate the map if bg data change
    boost::hash_combine(hash, v.x());
    boost::hash_combine(hash, v.y());
    boost::hash_combine(hash, v.z());
    return boost::str( boost::format("%x") % hash );
  }

  std::shared_ptr<MapImpl::Chunk> & MapImpl::getChunk(int x, int y, int z) const
  {
    ovdb::Vec3I vec(x, y, z);
    vec /= myChunkSize;
    GridType::Ptr grid;

    if (myChunks.find(vec) != myChunks.end())
    {
      grid = myChunks[vec]->grid;
    }
    else
    {
      myChunks[vec].reset(new Chunk);
      myChunks[vec]->fileName = getChunkName(vec);
    }

    myChunks[vec]->lastAccess = myClock.now();

    if (!grid)
    {
      loadChunk(myChunks[vec]);
    }

    return myChunks[vec];
  }

  void MapImpl::loadChunk(std::shared_ptr<Chunk> & chunk) const
  {
    boost::mutex::scoped_lock guard(chunk->lock);
    if (boost::filesystem::exists(myMapPath + dirSep + chunk->fileName) &&
        boost::filesystem::file_size(myMapPath + dirSep + chunk->fileName) > 0)
    {
      iostreams::file_source fs(myMapPath + dirSep + chunk->fileName);
      iostreams::filtering_istream is;
      is.push(iostreams::bzip2_decompressor());
      is.push(fs);
      ovdb::io::Stream ss(is);
      ss.setCompressionEnabled(false);
      ovdb::GridPtrVecPtr vc = ss.getGrids();
      chunk->grid = ovdb::gridPtrCast<GridType>(vc->operator[](0));
    }
    else
    {
      chunk->grid = GridType::create(myBackgroundValue);
      chunk->grid->setName(chunk->fileName);
    }
    chunk->accessor.reset(new GridType::Accessor(chunk->grid->getAccessor()));
  }

  void MapImpl::saveChunk(std::shared_ptr<Chunk> & chunk) const
  {
    boost::mutex::scoped_lock guard(chunk->lock);
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
    chunk->grid.reset();
    chunk->accessor.reset();
  }

  const MapCell & MapImpl::background() const { return myBank->get(myBackgroundValue); }

  std::shared_ptr<MapBank> MapImpl::bank() const { return myBank; }

  Map::Map(std::shared_ptr<MapBank> & bank, const std::string & mapPath, bool load, unsigned int chunkSizeX,
           unsigned int chunkSizeY, unsigned int chunkSizeZ, const MapCell & bgValue): myImpl(nullptr)
  {
    myImpl = new MapImpl(this, bank, mapPath, load, chunkSizeX, chunkSizeY, chunkSizeZ, bgValue);
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
