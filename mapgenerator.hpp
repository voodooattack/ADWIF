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
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <memory>
#include <random>

#include <boost/polygon/polygon.hpp>
#include <boost/multi_array.hpp>
#include <boost/polygon/gtl.hpp>

#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/string.hpp>
#include <boost/atomic.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include <json/value.h>
#include <FreeImagePlus.h>

#include "animation.hpp"

namespace ADWIF
{
  typedef boost::polygon::polygon_with_holes_data<double> polygon;
  typedef boost::polygon::polygon_traits<polygon>::point_type point;

  struct BiomeCell
  {
    std::string name;
    int x, y;
    double height;
    bool flat;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
      ar & name;
      ar & x;
      ar & y;
      ar & height;
    }

    bool operator== (const BiomeCell & other) const
    {
      return x == other.x && y == other.y && height == other.height && name == other.name;
    }
  };

  struct Region
  {
    std::string biome;
    point centroid;
    polygon poly;
    double area;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
      ar & biome;
      ar & centroid;
      ar & poly;
      ar & area;
    }
  };

  class MapGenerator: public std::enable_shared_from_this<MapGenerator>
  {
  public:
    MapGenerator(const std::shared_ptr<class Game> & game);
    ~MapGenerator();

    std::shared_ptr<class Game>  game() { return myGame; }
    void game(std::shared_ptr<class Game> & game) { myGame = game; }

    const fipImage & heightmap() const { return myHeightMap; }
    void heightmapImage(const fipImage & image) { myHeightMap = image; myHeightMap.convertTo32Bits(); }

    const fipImage & mapImage() const { return myMapImg; }
    void mapImage(const fipImage & image) { myMapImg = image; myMapImg.convertTo32Bits(); }

    unsigned int chunkSizeX() const { return myChunkSizeX; }
    void chunkSizeX(unsigned int size) { myChunkSizeX = size; }

    unsigned int chunkSizeY() const { return myChunkSizeY; }
    void chunkSizeY(unsigned int size) { myChunkSizeY = size; }

    unsigned int chunkSizeZ() const { return myChunkSizeZ; }
    void chunkSizeZ(unsigned int size) { myChunkSizeZ = size; }

    unsigned int height() const { return myHeight; }
    unsigned int width() const { return myWidth; }

    unsigned int depth() const { return myDepth; }
    void depth(unsigned int depth) { myDepth = depth; }

    void init();
    void generateAll();
    void generateAround(unsigned int x, unsigned int y, int z = 0, unsigned int radius = 1, unsigned int radiusZ = 1);
    void generateOne(unsigned int x, unsigned int y, int z, bool regenerate = false);

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
      ar & myInitialisedFlag;
      ar & myRegions;
      ar & myHeight;
      ar & myWidth;
      ar & myDepth;
    }

    void notifyLoad();
    void notifySave();

  private:
    void generateBiomeMap();

  private:

    inline static uint32_t getPixelColour (int x, int y, const fipImage & img)
    {
      RGBQUAD pptc;
      img.getPixelColor(x,y,&pptc);
      return pptc.rgbBlue | pptc.rgbGreen << 8 | pptc.rgbRed << 16;
    }

  private:
    std::shared_ptr<class Game> myGame;
    fipImage myMapImg;
    fipImage myHeightMap;
    unsigned int myChunkSizeX, myChunkSizeY, myChunkSizeZ;
    std::unordered_map<uint32_t, std::string> myColourIndex;
    std::mt19937 myRandomEngine;
    boost::multi_array<bool, 3> myGenerationMap;
    boost::recursive_mutex myGenerationLock;
    boost::multi_array<BiomeCell, 2> myBiomeMap;
    std::vector<Region> myRegions;
    unsigned int myHeight, myWidth, myDepth;
    unsigned int mySeed;
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
