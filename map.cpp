#include "map.hpp"
#include "map_p.hpp"
#include <unordered_set>

#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/lexical_cast.hpp>

#include <openvdb/io/Stream.h>


namespace ADWIF
{
  MapBank::MapBank(std::iostream & stream) : myStream(stream), myCache(), myAccessTimes()
  {
    if (!stream.good())
      throw std::runtime_error("bad stream state initializing map bank");
  }

  const MapCell & MapBank::get(uint64_t hash)
  {
    auto cell = myCache.find(hash);
    if (cell == myCache.end())
    {
      myCache[hash] = loadCell(hash);
    }
    myAccessTimes[hash] = myClock.now();
    if (myCache.size() > 1024 * 1024)
      prune();
    return myCache[hash];
  }

  void MapBank::prune(bool pruneAll)
  {
    std::unordered_map<uint64_t, MapCell> toStore;
    auto i = myAccessTimes.begin();
    while (i != myAccessTimes.end())
    {
      if (pruneAll || myClock.now() - i->second > std::chrono::seconds(4))
      {
        auto item = myCache.find(i->first);
        toStore.insert({item->first, item->second});
        myCache.erase(myCache.find(i->first));
        i = myAccessTimes.erase(i);
      } else ++i;
    }
    myStream.seekg(std::ios_base::beg);
    myStream.seekp(std::ios_base::beg);
    while (!toStore.empty())
    {
      uint64_t fhash;// = read<uint64_t>(myStream);
      uint32_t size; // = read<uint32_t>(myStream);
      //
      if (read<uint64_t>(myStream, fhash) && read<uint32_t>(myStream, size))
      {
        myStream.seekg(size, std::ios_base::cur);
        toStore.erase(fhash);
      }
      else
      {
        myStream.clear();
        myStream.seekp(myStream.tellg(), std::ios_base::beg);
        for (auto const & i : toStore)
        {
          write<uint64_t>(myStream, i.first);
          std::ostringstream ss(std::ios_base::binary);
          {
            boost::archive::binary_oarchive io(ss, boost::archive::no_header);
            io & i.second;
          }
          const std::string str = ss.str();
          write<std::string::value_type,uint32_t>(myStream, str);
        }
        break;
      }
    }
  }

  MapCell MapBank::loadCell(uint64_t hash)
  {
    myStream.seekg(std::ios_base::beg);
    MapCell cell;

    while (myStream.good())
    {
      uint64_t fhash;
      uint32_t size;
      if (!read<uint64_t>(myStream, fhash) || !read<uint32_t>(myStream, size))
        throw std::runtime_error("malformed map bank");

      if (fhash == hash)
      {
        boost::archive::binary_iarchive ia(myStream, boost::archive::no_header);
        ia & cell;
        return std::move(cell);
      }
      else
        myStream.seekg(size, std::ios_base::cur);
    }

    throw std::runtime_error("stream error finding cell " + boost::lexical_cast<std::string>(hash));
    //return MapCell();
  }

  uint64_t MapBank::put(const MapCell & cell)
  {
    uint64_t hash = cell.hash();
    myAccessTimes[hash] = myClock.now();
    if (myCache.find(hash) == myCache.end())
      myCache[hash] = cell;
    return hash;
  }

  MapImpl::MapImpl(Map * parent, std::shared_ptr<MapBank> & bank,
                   const std::string & mapPath, bool load, unsigned int chunkSizeX,
                   unsigned int chunkSizeY, unsigned int chunkSizeZ,
                   const MapCell & bgValue):
    myMap(parent), myGrids(), myBank(bank), myChunkSize(chunkSizeX, chunkSizeY, chunkSizeZ),
    myAccessTolerance(2000000), myBackgroundValue(0), myMapPath(mapPath), myClock(),
    myAccessCounter(0), myMemThresholdMB(800), myDurationThreshold(boost::chrono::minutes(10))
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
  }

  MapImpl::~MapImpl()
  {
  }

  const MapCell & MapImpl::get(int x, int y, int z) const
  {
    std::shared_ptr<GridType::Accessor> grid = getChunkAccessor(x, y, z)->accessor;
    if (myAccessCounter++ % myAccessTolerance == 0)
      prune(false);
    return myBank->get(grid->getValue(ovdb::Coord(x, y, z)));
  }

  void MapImpl::set(int x, int y, int z, const MapCell & cell)
  {
    uint64_t hash = myBank->put(cell);

    std::shared_ptr<GridEntry> grid = getChunkAccessor(x, y, z);

    if (hash == myBackgroundValue)
      grid->accessor->setValueOff(ovdb::Coord(x, y, z), hash);
    else
      grid->accessor->setValue(ovdb::Coord(x, y, z), hash);

    myAccessCounter++;

    if (myAccessCounter % myAccessTolerance == 0)
      grid->memUse = grid->grid->memUsage();

    if (!myGrids.empty() && (myAccessCounter % myAccessTolerance == 0))
    {
      prune(false);
    }
  }

  void MapImpl::prune(bool pruneAll) const
  {
    if (myGrids.size() <= 1 && !pruneAll) return;

    std::vector<std::shared_ptr<GridEntry>> toStore;
    std::vector<std::pair<ovdb::Vec3I, std::shared_ptr<GridEntry>>> accessTimesSorted;
    {
      accessTimesSorted.assign(myGrids.begin(), myGrids.end());
    }

    std::sort(accessTimesSorted.begin(), accessTimesSorted.end(),
              [](const std::pair<ovdb::Vec3I, std::shared_ptr<GridEntry>> & first,
                 const std::pair<ovdb::Vec3I, std::shared_ptr<GridEntry>> & second)
    {
      return first.second->lastAccess < second.second->lastAccess;
    });

    unsigned long int memUse = myMemThresholdMB ? getMemUsage() / (1024 * 1024) : 0;
/*    for(auto const & i : myGrids)
      memUse += i.second->memUse;
    memUse /= 1024 * 1024; */
    unsigned long int freed = memUse;

    {
      auto i = accessTimesSorted.begin();
      while (i != accessTimesSorted.end())
      {
        if (pruneAll || myClock.now() - i->second->lastAccess > myDurationThreshold || (memUse > myMemThresholdMB))
        {
          auto item = myGrids.find(i->first);
          toStore.push_back(item->second);
          myGrids.erase(myGrids.find(i->first));
          i = accessTimesSorted.erase(i);
          if (myMemThresholdMB && (freed -= item->second->grid->memUsage()) <= 0)
            break;
        } else ++i;
      }
    }

    for (auto & i : toStore)
    {
      std::shared_ptr<ovdb::io::File> file;
      if (i->file)
        file = i->file;
      else
      {
        std::string gridName = i->fileName;
        file.reset(new ovdb::io::File(myMapPath + dirSep + gridName));
        i->file = file;
      }
      if (file->isOpen())
        file->close();
      std::vector<GridType::Ptr> gv = { i->grid };
      file->setCompressionEnabled(true);
      file->write(gv);
      file->close();
    }

    myBank->prune(pruneAll);
  }

  std::string MapImpl::getChunkName(const ovdb::Vec3I & v) const
  {
    unsigned long int hash = 0; // myBackgroundValue; // if this is enabled it'll invalidate the map if bg data change
    boost::hash_combine(hash, v.x());
    boost::hash_combine(hash, v.y());
    boost::hash_combine(hash, v.z());
    return boost::str( boost::format("%x") % hash );
  }

  std::shared_ptr<MapImpl::GridEntry> & MapImpl::getChunkAccessor(int x, int y, int z) const
  {
    ovdb::Vec3I vec(x, y, z);
    vec /= myChunkSize;
    GridType::Ptr grid;

    if (myGrids[vec])
      grid = myGrids[vec]->grid;
    else
    {
      myGrids[vec].reset(new GridEntry);
      myGrids[vec]->fileName = getChunkName(vec);
    }

    myGrids[vec]->lastAccess = myClock.now();

    if (!grid)
    {
      const std::string & gridName = myGrids[vec]->fileName;
      if (myGrids[vec]->file)
      {
        if (myGrids[vec]->file->isOpen()) myGrids[vec]->file->open();
        grid = ovdb::gridPtrCast<GridType>(myGrids[vec]->file->readGrid(gridName));
      }
      else if (boost::filesystem::exists(myMapPath + dirSep + gridName))
      {
        std::shared_ptr<ovdb::io::File> file(new ovdb::io::File(myMapPath + dirSep + gridName));
        file->open();
        grid = ovdb::gridPtrCast<GridType>(file->readGrid(gridName));
        myGrids[vec]->file = file;
      }
      else
      {
        grid = GridType::create(myBackgroundValue);
        grid->setName(gridName);
      }
    }

    myGrids[vec]->grid = grid;
    myGrids[vec]->accessor = std::shared_ptr<GridType::Accessor>(new GridType::Accessor(grid->getAccessor()));

    return myGrids[vec];
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
