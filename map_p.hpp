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

    struct GridEntry
    {
      GridType::Ptr grid;
      std::shared_ptr<GridType::Accessor> accessor;
      std::shared_ptr<ovdb::io::File> file;
      time_point lastAccess;
      std::string fileName;
      unsigned long int memUse;
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
      for(auto const & i : myGrids)
        count += i.second->grid->activeVoxelCount();
      return count;
    }

    uint64_t getMemUsage() const {
      uint64_t count = 0;
      for(auto const & i : myGrids)
        count += i.second->grid->memUsage();
      return count;
    }

    std::shared_ptr<MapBank> bank() const;

    void prune(bool pruneAll = false) const;

  private:
    std::string getChunkName(const ovdb::Vec3I & v) const;
    std::shared_ptr<GridEntry> & getChunkAccessor(int x, int y, int z) const;

    void pruneThread();

  private:


    typedef std::unordered_map<ovdb::Vec3I, std::shared_ptr<GridEntry>> GridMap;

    class Map * const myMap;
    mutable GridMap myGrids;
    std::shared_ptr<MapBank> myBank;
    ovdb::Vec3I myChunkSize;
    unsigned long int myAccessTolerance;
    uint64_t myBackgroundValue;
    std::string myMapPath;
    clock_type myClock;
    mutable unsigned long int myAccessCounter;
    unsigned long int myMemThresholdMB;
    duration_type myDurationThreshold;
    static bool myInitialisedFlag;
  };
}
#endif // MAPPRIVATE_H
