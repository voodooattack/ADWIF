#ifndef MAPGENERATOR_H
#define MAPGENERATOR_H
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <memory>
#include <random>

#include <boost/multi_array.hpp>

#include <json/value.h>
#include <FreeImagePlus.h>

#include "animation.hpp"

namespace ADWIF
{
  class MapGenerator
  {
  public:
    MapGenerator(std::shared_ptr<class Game> & game);
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

    unsigned int height() { return myMapImg.getHeight(); }
    unsigned int width() { return myMapImg.getWidth(); }

    void init();
    void generateAll();
    void generateAround(unsigned int x, unsigned int y, unsigned int z = 0, unsigned int radiusXY = 1, unsigned int radiusZ = 1)
    {
      
    }
    void generateOne(unsigned int x, unsigned int y);

  private:
    void generateChunk(struct Biome * biome, int x, int y);

  private:
    std::shared_ptr<class Game> myGame;
    fipImage myMapImg;
    fipImage myHeightMap;
    unsigned int myChunkSizeX, myChunkSizeY;
    std::unordered_map<uint32_t, Biome *> myColourIndex;
    boost::multi_array<bool, 2> myGenerationMap;
    std::mt19937 myRandomEngine;
  };
}

#endif // MAPGENERATOR_H
