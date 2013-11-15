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

#include <string>
#include <algorithm>
#include <unordered_set>

#include <physfs.hpp>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/polygon/polygon.hpp>
#include <boost/polygon/voronoi.hpp>
#include <boost/multi_array.hpp>
#include <boost/optional.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/adapted/boost_polygon.hpp>

namespace ADWIF
{
  MapGenerator::MapGenerator(const std::shared_ptr<Game> & game, bool load):
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
      generateBiomeMap();
      myInitialisedFlag = true;
    }
  }

  void MapGenerator::generateBiomeMap()
  {
    for(auto const & b : myGame->biomes())
      myColourIndex[b.second->mapColour] = b.second->name;
    myHeight = myMapImg.getHeight();
    myWidth = myMapImg.getWidth();
    myGenerationMap.resize(boost::extents[myWidth][myHeight]);
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
          //return;
        }
        double height = (h.rgbBlue | h.rgbGreen | h.rgbRed) / 256.0;
        myBiomeMap[x][y].name = myColourIndex[colour];
        myBiomeMap[x][y].x = x;
        myBiomeMap[x][y].y = y;
        myBiomeMap[x][y].height = height;
      }

      // Clustering algorithm for terrain features

      using polygon = boost::polygon::polygon_with_holes_data<int>;
      using point = boost::polygon::polygon_traits<polygon>::point_type;
      using segment = boost::polygon::segment_data<int>;

      boost::multi_array<bool,2> visited;
      visited.resize(boost::extents[myWidth][myHeight]);

      struct Cluster
      {
        std::string biome;
        std::vector<point> points;
        point centroid;
        polygon poly;
      };

      std::vector<Cluster> clusters;

//       int i = 0;

      for(unsigned int y = 0; y < myHeight; y++)
      {
        for (unsigned int x = 0; x < myWidth; x++)
        {
          if (visited[x][y]) continue;

          uint32_t colour = getPixelColour(x, y, myMapImg);

//           fipImage img(myMapImg);

          std::vector<point> points;
          std::deque<point> q;

          q.push_back(point(x,y));

          while(!q.empty())
          {
            const point n = q.back(); q.pop_back();
            uint32_t pcolour = getPixelColour(n.x(), n.y(), myMapImg);

            if (!visited[n.x()][n.y()] && colour == pcolour)
            {
              visited[n.x()][n.y()] = true;

              if (n.x() > 0) q.push_back(point(n.x() - 1,n.y()));
              if (n.x() < (signed)myWidth - 1) q.push_back(point(n.x() + 1,n.y()));
              if (n.y() > 0) q.push_back(point(n.x(),n.y() - 1));
              if (n.y() < (signed)myHeight - 1) q.push_back(point(n.x(),n.y() + 1));

              if (n.x() > 0 && n.y() > 0) q.push_back(point(n.x() - 1,n.y() - 1));
              if (n.x() < (signed)myWidth - 1 && n.y() < (signed)myHeight - 1) q.push_back(point(n.x() + 1,n.y() + 1));
              if (n.x() < (signed)myWidth - 1 && n.y() > 0) q.push_back(point(n.x() + 1,n.y() - 1));
              if (n.x() > 0 && n.y() < (signed)myHeight - 1) q.push_back(point(n.x() - 1,n.y() + 1));

//               RGBQUAD inner = { 0, 0, 255, 255 };
//               img.setPixelColor(n.x(),n.y(), &inner);
            }
            else
            {
              if (colour != pcolour)
              {
                points.push_back(n);
//                 RGBQUAD outer = { 0, 0, 0, 255 };
//                 img.setPixelColor(n.x(),n.y(), &outer);
              }
            }
          }

          if (points.empty()) continue;

          Cluster c;
          c.biome = myBiomeMap[x][y].name;
          c.points = points;
          //c.poly.set(points.begin(), points.end());
          //boost::polygon::center(c.centroid, c.poly);

          clusters.push_back(c);

//           std::string fileName = "png/" + boost::lexical_cast<std::string>(i++) + ".png";
//           img.flipVertical();
//           img.save(fileName.c_str());
        }
      }


     for (unsigned int c = 0; c < clusters.size(); c++)
     {
       boost::polygon::polygon_data<int> poly;


       std::cerr << clusters[c].biome << " [" << clusters[c].centroid.x() << "," << clusters[c].centroid.y() << "] { ";
       for (const auto & p : clusters[c].poly)
          std::cerr << "[" << p.x() << "," << p.y() << "] (" << myBiomeMap[p.x()][p.y()].name << "), ";
       std::cerr << "}" << std::endl;
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

  void MapGenerator::generateOne(unsigned int x, unsigned int y, bool regenerate)
  {
    if (x < 0 || y < 0 || x >= myHeight || y >= myWidth)
      return;
    if (!regenerate && myGenerationMap[x][y])
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
          continue;
        }
        else if (!bd(myRandomEngine))
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

