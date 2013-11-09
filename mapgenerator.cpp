#include "mapgenerator.hpp"
#include "engine.hpp"
#include "game.hpp"
#include "map.hpp"
#include "jsonutils.hpp"
#include "util.hpp"

#include <physfs.hpp>
#include <string>


namespace ADWIF
{
  MapGenerator::MapGenerator(std::shared_ptr<Game> & game):
    myGame(game), myMapImg(), myHeightMap(), myChunkSizeX(32), myChunkSizeY(32),
    myColourIndex(), myRandomEngine()
  {
    myRandomEngine.seed(time(NULL));
  }

  MapGenerator::~MapGenerator() { }

  void MapGenerator::init()
  {
    for(auto const & b : myGame->biomes())
      myColourIndex[b.second->mapColour] = b.second;
    myGenerationMap.resize(boost::extents[myMapImg.getWidth()][myMapImg.getHeight()]);
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
    if (x < 0 || y < 0 || x >= myMapImg.getWidth() || y >= myMapImg.getHeight())
      return;
    if (myGenerationMap[x][y])
      return;

    RGBQUAD v, h;
    myMapImg.getPixelColor(x, y, &v);
    myHeightMap.getPixelColor(x, y, &h);
    uint32_t colour = v.rgbBlue | v.rgbGreen << 8 | v.rgbRed << 16;
    if (myColourIndex.find(colour) == myColourIndex.end())
    {
      std::stringstream ss;
      ss << "unknown biome colour: " << std::hex << colour << " at pixel " << std::dec << x << "x" << y;
      //throw std::runtime_error(ss.str());
      myGame->engine()->reportError(false, ss.str());
      myGenerationMap[x][y] = false;
      return;
    }

    unsigned int offx = x * myChunkSizeX, offy = y * myChunkSizeY;

    double height = (h.rgbBlue | h.rgbGreen | h.rgbRed) / 256.0;

    Biome * biome = myColourIndex[colour];

    if (biome->background)
      return;

    std::uniform_int_distribution<int> ud(0, biome->materials.size()-1);

    for (unsigned int yy = offy; yy < offy + myChunkSizeY; yy++)
    {
      for (unsigned int xx = offx; xx < offx + myChunkSizeX; xx++)
      {
        std::string mat = biome->materials[ud(myRandomEngine)];
        std::uniform_int_distribution<int> ud2(0, myGame->materials()[mat]->disp[TerrainType::Floor].size() - 1);
        MapCell c;
        c.type = TerrainType::Floor;
        c.material = mat;
        c.symIdx = ud2(myRandomEngine);
        c.biome = biome->name;
        myGame->map()->set(xx, yy, 0, c);
      }
    }

    myGenerationMap[x][y] = true;
  }
}

