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

#ifndef MAPGENERATOR_H
#define MAPGENERATOR_H

#include "config.hpp"

#include "animation.hpp"

#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <unordered_map>
#include <memory>
#include <random>
#include <utility>

#include <boost/polygon/polygon.hpp>
#include <boost/multi_array.hpp>
#include <boost/polygon/gtl.hpp>

#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <boost/atomic.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <boost/logic/tribool.hpp>

#ifdef NOISE_DIR_IS_LIBNOISE
#include <libnoise/noise.h>
#else
#include <noise/noise.h>
#endif

#include <json/value.h>
#include <FreeImagePlus.h>

namespace ADWIF
{
  class Engine;
  class Game;
  class GenerateTerrainTask;

  typedef boost::polygon::polygon_with_holes_data<double> Polygon;
  typedef boost::polygon::polygon_traits<Polygon>::point_type Point2D;

  typedef boost::geometry::model::point<int, 3, boost::geometry::cs::cartesian> Point3D;
  typedef boost::geometry::model::box<Point3D> Box3D;
  typedef std::pair<Box3D, std::shared_ptr<GenerateTerrainTask>> SIVal;
  typedef boost::geometry::index::rtree<SIVal, boost::geometry::index::quadratic<16> > SpatialIndex;

  struct BiomeCell
  {
    std::string name;
    int x, y;
    double height;
    bool aquatic;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
      ar & name;
      ar & x;
      ar & y;
      ar & height;
      ar & aquatic;
    }

    bool operator== (const BiomeCell & other) const
    {
      return x == other.x && y == other.y && height == other.height && name == other.name && aquatic == other.aquatic;
    }
  };

  struct Region
  {
    std::string biome;
    std::string name;
    std::string desc;
    Point2D centroid;
    Polygon poly;
    double area;
    bool infinite;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
      ar & biome;
      ar & name;
      ar & desc;
      ar & centroid;
      ar & poly;
      ar & area;
      ar & infinite;
    }
  };

  class MapGenerator: public std::enable_shared_from_this<MapGenerator>
  {
  public:
    MapGenerator(const std::shared_ptr<class Game> & game);
    ~MapGenerator();

    std::shared_ptr<class Game> game() { return myGame.lock(); }
    void game(std::shared_ptr<class Game> & game) { myGame = game; }

    const fipImage & heightmap() const { return myHeightMap; }
    void heightmapImage(const fipImage & image) { myHeightMap = image; myHeightMap.convertTo32Bits(); }

    const fipImage & mapImage() const { return myMapImg; }
    void mapImage(const fipImage & image) { myMapImg = image; myMapImg.convertTo32Bits(); }

    int chunkSizeX() const { return myChunkSizeX; }
    void chunkSizeX( int size) { myChunkSizeX = size; }

    int chunkSizeY() const { return myChunkSizeY; }
    void chunkSizeY( int size) { myChunkSizeY = size; }

    int chunkSizeZ() const { return myChunkSizeZ; }
    void chunkSizeZ( int size) { myChunkSizeZ = size; }

    std::mt19937 & random() { return myRandomEngine; }

    int height() const { return myHeight; }
    int width() const { return myWidth; }
    int depth() const { return myDepth; }
    void depth( int depth) { myDepth = depth; }

    const boost::multi_array<BiomeCell, 2> & biomeMap() const { return myBiomeMap; }
    boost::multi_array<BiomeCell, 2> & biomeMap() { return myBiomeMap; }

    const std::vector<Region> & regions() const { return myRegions; }
    std::vector<Region> & regions() { return myRegions; }

    int preprocessingProgress() const { return myMapPreprocessingProgress.load(); }

    void init();
    void generateAround( int x,  int y, int z = 0,  int radius = 1,  int radiusZ = 1);

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
      ar & mySeed;
      ar & myChunkSizeX;
      ar & myChunkSizeY;
      ar & myChunkSizeZ;
      ar & myColourIndex;
      ar & myRandomEngine;
      ar & myGenerationMap;
      ar & myBiomeMap;
      ar & myHeights;
      ar & myInitialisedFlag;
      ar & myRegions;
      ar & myHeight;
      ar & myWidth;
      ar & myDepth;
    }

    void notifyLoad();
    void notifySave();

    void abort();

    inline int getHeight(double x, double y, double z = 0)
    {
      return round(getHeightReal(x, y, z));
    }

    inline double getHeightReal(double x, double y, double z = 0)
    {
      return myHeightSource->GetValue(x, y, z) * (double)myChunkSizeZ * ((double)myDepth / 2.0);
    }

  private:
    bool generateBiomeMap();

  public:
    boost::logic::tribool isGenerated(int x, int y, int z)
    {
      boost::upgrade_lock<boost::shared_mutex> guard(myGenerationLock);
      return myGenerationMap[x][y][z+myDepth/2];
    }

    void setGenerated(int x, int y, int z, const boost::logic::tribool & g)
    {
      boost::upgrade_lock<boost::shared_mutex> guard(myGenerationLock);
      boost::upgrade_to_unique_lock<boost::shared_mutex> lock(guard);
      myGenerationMap[x][y][z+myDepth/2] = g;
    }

    void notifyComplete(const std::shared_ptr<GenerateTerrainTask> & task);

  private:

    inline static uint32_t getPixelColour (int x, int y, const fipImage & img)
    {
      RGBQUAD pptc;
      img.getPixelColor(x,y,&pptc);
      return pptc.rgbBlue | pptc.rgbGreen << 8 | pptc.rgbRed << 16;
    }

  private:
    std::weak_ptr<class Game> myGame;
    fipImage myMapImg;
    fipImage myHeightMap;
    int myChunkSizeX, myChunkSizeY, myChunkSizeZ;
    std::unordered_map<uint32_t, std::string> myColourIndex;
    std::mt19937 myRandomEngine;
    boost::multi_array<boost::tribool, 3> myGenerationMap;
    boost::shared_mutex myGenerationLock;
    boost::multi_array<BiomeCell, 2> myBiomeMap;
    boost::multi_array<double, 2> myHeights;
    std::vector<Region> myRegions;
    int myHeight, myWidth, myDepth;
    unsigned int mySeed;
    int myLastChunkX, myLastChunkY, myLastChunkZ;
    std::vector<std::shared_ptr<noise::module::Module>> myNoiseModules;
    std::map<std::string, std::shared_ptr<noise::module::Module>> myNoiseModuleDefs;
    std::shared_ptr<noise::module::Module> myHeightSource;
    SpatialIndex myIndex;
    boost::atomic_int myMapPreprocessingProgress;
    bool myInitialisedFlag;
  };
}

namespace std
{
  template <> struct hash<ADWIF::BiomeCell>
  {
    size_t operator()(const ADWIF::BiomeCell & bc) const
    {
      size_t h = 0;
      boost::hash_combine(h, bc.name);
      boost::hash_combine(h, bc.x);
      boost::hash_combine(h, bc.y);
      boost::hash_combine(h, bc.height);
      return h;
    }
  };
}

#endif // MAPGENERATOR_H
