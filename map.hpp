#ifndef MAP_H
#define MAP_H

#include <vector>
#include <cstdint>
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <memory>

#include "fileutils.hpp"
#include "mapbank.hpp"

#include <boost/functional/hash/extensions.hpp>
#include <boost/functional/hash/hash.hpp>

namespace ADWIF
{
  class Map
  {
  public:
    Map(std::shared_ptr<class MapBank> & bank, const std::string & mapPath, bool load, unsigned int chunkSizeX,
        unsigned int chunkSizeY, unsigned int chunkSizeZ, const MapCell & bgValue = MapCell());
    ~Map();

    const MapCell & get(int x, int y, int z) const;
    void set(int x, int y, int z, const MapCell & cell);

    const MapCell & background() const;

    std::shared_ptr<class MapBank> bank() const;

    void prune() const;
    void save() const;

  private:
    class MapImpl * myImpl;
  };
}

#endif // MAP_H
