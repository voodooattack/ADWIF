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

#include "map.hpp"

namespace ovdb = openvdb::v1_1;

namespace std
{
  template <> struct hash<ovdb::Vec3I>
  {
    size_t operator()(const ovdb::Vec3I & vec) const
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
      GridType::Ptr grid;
      std::shared_ptr<GridType::Accessor> accessor;
      time_point lastAccess;
      std::string fileName;
      boost::mutex lock;
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
          boost::mutex::scoped_lock guard(i.second->lock);
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
          boost::mutex::scoped_lock guard(i.second->lock);
          if (i.second->grid && i.second)
            count += i.second->grid->memUsage();
        }
      }
      return count;
    }

    std::shared_ptr<MapBank> bank() const;

    void prune(bool pruneAll = false) const;

  private:
    std::string getChunkName(const ovdb::Vec3I & v) const;
    std::shared_ptr<Chunk> & getChunk(int x, int y, int z) const;

    void loadChunk(std::shared_ptr<Chunk> & chunk) const;
    void saveChunk(std::shared_ptr<Chunk> & chunk) const;

    void pruneTask();

  private:


    typedef std::unordered_map<ovdb::Vec3I, std::shared_ptr<Chunk>> GridMap;

    class Map * const myMap;
    mutable GridMap myChunks;
    std::shared_ptr<MapBank> myBank;
    ovdb::Vec3I myChunkSize;
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
    unsigned long int myMemUsage;

    static bool myInitialisedFlag;
  };
}
#endif // MAPPRIVATE_H
