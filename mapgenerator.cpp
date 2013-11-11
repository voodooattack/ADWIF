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

#include "mapgenerator.hpp"
#include "engine.hpp"
#include "game.hpp"
#include "map.hpp"
#include "jsonutils.hpp"
#include "util.hpp"

#include <physfs.hpp>
#include <string>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/filesystem.hpp>

namespace ADWIF
{
  MapGenerator::MapGenerator(std::shared_ptr<Game> & game, bool load):
    myGame(game), myMapImg(), myHeightMap(), myChunkSizeX(32), myChunkSizeY(32),
    myColourIndex(), myRandomEngine(), myGenerationMap(), myBiomeMap(),
    myInitialisedFlag(false)
  {
    myRandomEngine.seed(time(NULL));
  }

  MapGenerator::~MapGenerator() { }

  void MapGenerator::init()
  {
    if (!myInitialisedFlag)
    {
      for(auto const & b : myGame->biomes())
        myColourIndex[b.second->mapColour] = b.second->name;
      myHeight = myMapImg.getHeight();
      myWidth = myMapImg.getWidth();
      myGenerationMap.resize(boost::extents[myWidth][myHeight]);
      generateBiomeMap();
      myInitialisedFlag = true;
    }
  }

  void MapGenerator::generateBiomeMap()
  {
    RGBQUAD v, h;
    myBiomeMap.resize(boost::extents[myWidth][myHeight]);
    for(unsigned int y = 0; y < myHeight; y++)
      for(unsigned int x = 0; x < myWidth; x++)
      {
        myMapImg.getPixelColor(x, y, &v);
        myHeightMap.getPixelColor(x, y, &h);
        uint32_t colour = v.rgbBlue | v.rgbGreen << 8 | v.rgbRed << 16;
        if (myColourIndex.find(colour) == myColourIndex.end())
        {
          std::stringstream ss;
          ss << "unknown biome colour: " << std::hex << colour
             << " at pixel " << std::dec << x << "x" << y << ", area not generated";
          myGame->engine()->reportError(false, ss.str());
          myGenerationMap[x][y] = false;
          return;
        }
        double height = (h.rgbBlue | h.rgbGreen | h.rgbRed) / 256.0;
        Biome * biome = myGame->biomes()[myColourIndex[colour]];
        myBiomeMap[x][y].name = biome->name;
        myBiomeMap[x][y].height = height;
      }
  }

  void MapGenerator::generateAll()
  {
    for(unsigned int y = 0; y < myMapImg.getHeight(); y++)
      for(unsigned int x = 0; x < myMapImg.getWidth(); x++)
      {
        generateOne(x, y);
      }
  }

  void MapGenerator::generateOne(unsigned int x, unsigned int y)
  {
    if (x < 0 || y < 0 || x >= myHeight || y >= myWidth)
      return;
    if (myGenerationMap[x][y])
      return;

    unsigned int offx = x * myChunkSizeX, offy = y * myChunkSizeY;

    Biome * biome = myGame->biomes()[myBiomeMap[x][y].name];

    std::uniform_int_distribution<int> ud(0, biome->materials.size()-1);
    std::bernoulli_distribution bd(0.001);

    for (unsigned int yy = offy; yy < offy + myChunkSizeY; yy++)
    {
      for (unsigned int xx = offx; xx < offx + myChunkSizeX; xx++)
      {
        MapCell c = myGame->map()->get(xx, yy, 0);

        std::string mat = biome->materials[ud(myRandomEngine)];
        std::uniform_int_distribution<int> ud2(0, myGame->materials()[mat]->disp[TerrainType::Floor].size() - 1);

        c.material = mat;
        c.symIdx = ud2(myRandomEngine);
        c.biome = biome->name;

        if (c.type == TerrainType::RampU)
        {

        }
        if (!bd(myRandomEngine))
          c.type = TerrainType::Floor;
        else
        {
          c.type = TerrainType::Wall;
          for (int y1 = -1; y1 < 2; y1++)
            for (int x1 = -1; x1 < 2; x1++)
              //if ((x1 | y1) != 0)
              {
                MapCell cc = c;
                cc.type = TerrainType::RampU;
                cc.symIdx = 0;
                myGame->map()->set(xx+x1, yy+y1, 0, cc);
              }
        }
        myGame->map()->set(xx, yy, 0, c);
      }
    }

    myGenerationMap[x][y] = true;
  }
  void MapGenerator::notifyLoad() {  }
  void MapGenerator::notifySave() {  }

}

