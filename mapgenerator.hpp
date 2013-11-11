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

#include <boost/multi_array.hpp>

#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/string.hpp>

#include <json/value.h>
#include <FreeImagePlus.h>

#include "animation.hpp"

namespace ADWIF
{
  class MapGenerator
  {
  public:
    MapGenerator(std::shared_ptr<class Game> & game, bool load);
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

    unsigned int height() { return myHeight; }
    unsigned int width() { return myWidth; }

    void init();
    void generateAll();
    void generateAround(unsigned int x, unsigned int y, unsigned int z = 0, unsigned int radius = 1, unsigned int radiusZ = 1)
    {

    }
    void generateOne(unsigned int x, unsigned int y);

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
      ar & myChunkSizeX;
      ar & myChunkSizeY;
      ar & myColourIndex;
      ar & myRandomEngine;
      ar & myGenerationMap;
      ar & myBiomeMap;
      ar & myInitialisedFlag;
      ar & myHeight;
      ar & myWidth;
    }

    void notifyLoad();
    void notifySave();

  private:
    void generateBiomeMap();
    void generateChunk(struct Biome * biome, int x, int y);

  private:

    struct BiomeCell
    {
      std::string name;
      double height;

      template<class Archive>
      void serialize(Archive & ar, const unsigned int version)
      {
        ar & name;
        ar & height;
      }
    };

  private:
    std::shared_ptr<class Game> myGame;
    fipImage myMapImg;
    fipImage myHeightMap;
    unsigned int myChunkSizeX, myChunkSizeY;
    std::unordered_map<uint32_t, std::string> myColourIndex;
    std::mt19937 myRandomEngine;
    boost::multi_array<bool, 2> myGenerationMap;
    boost::multi_array<BiomeCell, 2> myBiomeMap;
    unsigned int myHeight, myWidth;
    bool myInitialisedFlag;
  };
}

#endif // MAPGENERATOR_H
