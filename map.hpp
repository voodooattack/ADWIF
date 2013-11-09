#ifndef MAP_H
#define MAP_H

#include <vector>
#include <cstdint>
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <memory>

#include "item.hpp"
#include "fileutils.hpp"

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/functional/hash/extensions.hpp>
#include <boost/functional/hash/hash.hpp>

namespace ADWIF
{

  class Map;
  enum TerrainType : uint8_t
  {
    Floor,
    RampU,
    RampD,
    Hole,
    Wall,
  };

  inline std::string terrainTypeStr(TerrainType type)
  {
    switch(type)
    {
      case Floor: return "Floor";
      case RampU: return "RampU";
      case RampD: return "RampD";
      case Hole: return "Hole";
      case Wall: return "Wall";
    }
    return "Unknown";
  }

  inline TerrainType strTerrainType(const std::string & type)
  {
    std::string t;
    std::transform(type.begin(), type.end(), std::back_inserter(t), &tolower);
    if (t == "floor") return TerrainType::Floor;
    else if (t == "rampu") return TerrainType::RampU;
    else if (t == "rampd") return TerrainType::RampD;
    else if (t == "hole") return TerrainType::Hole;
    else if (t == "wall") return TerrainType::Wall;
    else return TerrainType::Hole;
  }

  enum Structure : uint8_t
  {
    None,
    SWall,
    Door,
    Window,
    Fence,
    Girder,
    Stairs,
    StairsU,
    StairsD,
    StairsUD,
  };

  struct MapCell: public ItemContainer
  {
    MapCell(): type(TerrainType::Floor), structure(None), material(), smaterial(), biome(), symIdx(0), visible(true), contents() {}
    ~MapCell() { }

    TerrainType type;
    Structure structure;
    std::string material;
    std::string smaterial;
    std::string biome;
    unsigned char symIdx;
    bool visible;
    std::vector<Item> contents;

    virtual size_t itemCount()  { return contents.size(); }
    virtual const Item & getItem(size_t index)  { return contents[index]; }

    virtual bool addItem(const class Item & item) {
      contents.push_back(item);
      return true;
    }

    virtual bool removeItem(const class Item & item) {
      contents.erase(std::remove(contents.begin(), contents.end(), item), contents.end());
      return true;
    }

    virtual uint64_t hash() const {
      size_t h = boost::hash<int>()(type);
      boost::hash_combine(h, (int)structure);
      boost::hash_combine(h, symIdx);
      boost::hash_combine(h, material);
      boost::hash_combine(h, smaterial);
      boost::hash_combine(h, biome);
      boost::hash_combine(h, visible);
      for(auto i : contents)
        boost::hash_combine(h, i.hash());
      return h;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
      ar & type;
      ar & structure;
      ar & material;
      ar & smaterial;
      ar & biome;
      ar & symIdx;
      ar & visible;
      ar & contents;
    }

    friend class boost::serialization::access;
  };

  class MapBank
  {
    using clock_type = std::chrono::steady_clock;
    using time_point = clock_type::time_point;
    using duration_type = clock_type::duration;

  public:
    MapBank(std::iostream & stream);

    const MapCell & get(uint64_t hash);
    uint64_t put(const MapCell & cell);
    void prune(bool pruneAll = false);

  private:
    MapCell loadCell(uint64_t hash);

  private:
    std::iostream & myStream;
    std::unordered_map<uint64_t, MapCell> myCache;
    std::unordered_map<uint64_t, time_point> myAccessTimes;
    clock_type myClock;
  };

  class Map
  {
  public:
    Map(std::shared_ptr<MapBank> & bank, const std::string & mapPath, bool load, unsigned int chunkSizeX,
        unsigned int chunkSizeY, unsigned int chunkSizeZ, const MapCell & bgValue = MapCell());
    ~Map();

    const MapCell & get(int x, int y, int z) const;
    void set(int x, int y, int z, const MapCell & cell);

    const MapCell & background() const;

    std::shared_ptr<MapBank> bank() const;

    void prune() const;
    void save() const;

  private:
    class MapImpl * myImpl;
  };
}

#endif // MAP_H
