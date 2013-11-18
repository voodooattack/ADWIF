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

#include <boost/container/flat_set.hpp>
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
#include <boost/assign.hpp>

template<class T>
std::ostream & operator<< (std::ostream & os, const boost::polygon::point_data<T> & p)
{
  os << p.x() << "," << p.y();
  return os;
}

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
    using boost::container::flat_set;

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

      using ptype = double;
      using polygon = boost::polygon::polygon_with_holes_data<ptype>;
      using point = boost::polygon::polygon_traits<polygon>::point_type;
      using segment = boost::polygon::segment_data<ptype>;
      using rect = boost::polygon::rectangle_data<ptype>;

      boost::multi_array<bool,2> visited;
      visited.resize(boost::extents[myWidth][myHeight]);

      struct Cluster
      {
        std::string biome;
        flat_set<point> points;
        point centroid;
      };

      std::vector<Cluster> clusters;

      int i = 0;

      boost::filesystem::remove_all("png/*");

      for(unsigned int y = 0; y < myHeight; y++)
      {
        for (unsigned int x = 0; x < myWidth; x++)
        {
          if (visited[x][y]) continue;

          uint32_t colour = getPixelColour(x, y, myMapImg);

          fipImage img(myMapImg);

          flat_set<point> points;
          std::deque<point> q;

          q.push_back(point(x,y));

          while(!q.empty())
          {
            const point n = q.back(); q.pop_back();
            uint32_t pcolour = getPixelColour(n.x(), n.y(), myMapImg);

            if (!visited[n.x()][n.y()] && colour == pcolour)
            {
              visited[n.x()][n.y()] = true;

              bool edge = false;

              if (n.x() > 0) q.push_back(point(n.x() - 1,n.y())); else edge = true;
              if (n.x() < (signed)myWidth - 1) q.push_back(point(n.x() + 1,n.y())); else edge = true;
              if (n.y() > 0) q.push_back(point(n.x(),n.y() - 1)); else edge = true;
              if (n.y() < (signed)myHeight - 1) q.push_back(point(n.x(),n.y() + 1)); else edge = true;

              if (n.x() > 0 && n.y() > 0) q.push_back(point(n.x() - 1,n.y() - 1)); else edge = true;
              if (n.x() < (signed)myWidth - 1 && n.y() < (signed)myHeight - 1) q.push_back(point(n.x() + 1,n.y() + 1)); else edge = true;
              if (n.x() < (signed)myWidth - 1 && n.y() > 0) q.push_back(point(n.x() + 1,n.y() - 1)); else edge = true;
              if (n.x() > 0 && n.y() < (signed)myHeight - 1) q.push_back(point(n.x() - 1,n.y() + 1)); else edge = true;

              //if (edge || ((int)n.x() % 2 == 0 && (int)n.y() % 2 == 0))
              //if (edge)
              {
                 points.insert(n);

                 if (edge)
                 {
                   RGBQUAD outer = { 0, 0, 0, 255 };
                   img.setPixelColor(n.x(),n.y(), &outer);
                 } else {
                   RGBQUAD inner = { 0, 0, 255, 255 };
                   img.setPixelColor(n.x(),n.y(), &inner);
                 }
              }
            }
            else if (colour != pcolour)
            {
              points.insert(n);

              RGBQUAD outer = { 0, 0, 0, 255 };
              img.setPixelColor(n.x(),n.y(), &outer);
            }
          }

          if (points.empty()) continue;

          Cluster c;
          c.biome = myBiomeMap[x][y].name;
          c.points = points;
          polygon poly;
          poly.set(points.begin(), points.end());
          boost::polygon::center(c.centroid, poly);

          clusters.push_back(c);

          std::string fileName = "png/" + boost::lexical_cast<std::string>(i++) + ".png";
          img.flipVertical();
          img.save(fileName.c_str());
        }
      }

      auto concaveHull = [](const flat_set<point> & in, polygon & poly, double alpha)
      {
        auto getCoords = [](const point & p1, const point & p2, double radius, bool dir) -> point
        {
          double midX = (p1.x() + p2.x()) / 2.0;
          double midY = (p1.y() + p2.y()) / 2.0;
          double dx = (p1.x() - p2.x()) / 2.0;
          double dy = (p1.y() - p2.y()) / 2.0;
          double dist = std::sqrt(dx * dx + dy * dy);
          double pDist = std::sqrt(radius * radius - dist * dist);
          double pdx, pdy;
          if (dir) {
            pdx = dy * pDist / dist;
            pdy = -dx * pDist / dist;
          } else {
            pdx = -dy * pDist / dist;
            pdy = dx * pDist / dist;
          }
          return point(midX + pdx, midY + pdy);
        };

        flat_set<segment> segments;

        using voronoi_diagram = boost::polygon::voronoi_diagram<ptype>;

        voronoi_diagram vd;
        boost::polygon::construct_voronoi(in.begin(), in.end(), &vd);

        for(const voronoi_diagram::edge_type & e : vd.edges())
        {
          const voronoi_diagram::cell_type * c1 = e.cell();
          const voronoi_diagram::cell_type * c2 = e.twin()->cell();

          const point p1 = *(in.begin() + c1->source_index()), p2 = *(in.begin() + c2->source_index());
          const point pp1 = getCoords(p1,p2,alpha/2,false), pp2 = getCoords(p1,p2,alpha/2,true);

          int e1 = 0, e2 = 0;
          for (const point & p3 : in)
          {
            if (p3 == p1 || p3 == p2) continue;
            if (boost::polygon::distance_squared(pp1, p3) < alpha) e1++;
            if (boost::polygon::distance_squared(pp2, p3) < alpha) e2++;
            if (e1 && e2) break;
          }
//           std::cerr << "detecting " << p1 << " to " << p2 << " ";
          bool dir = std::atan2(p1.y() - p2.y(), p1.x() - p2.x()) > 0;
          if (!e1)
          {
            if (dir)
              segments.insert(segment(p1, p2));
            else
              segments.insert(segment(p2, p1));
//             std::cerr << "(" << p1 << " -> " << p2 << ") ";
          }
          if (!e2)
          {
            if (dir)
              segments.insert(segment(p2, p1));
            else
              segments.insert(segment(p1, p2));
//             std::cerr << "(" << p1 << " <- " << p2 << ") ";
          }
          std::cerr << std::endl;
        }

        std::vector<polygon> polygons;
        std::vector<point> pointsIndexed;
        std::vector<segment> unsorted;

//     while there are still loose segments
//       take a loose segment and put it in a new polygon
//       while the tail vertex of the polygon doesn't match its head vertex
//         iterate over the remaining loose segments
//         if the head of the segment matches the tail of the polygon
//           append it to the polygon
//           break out of the iteration
//         reverse the segment
//         if the head of the segment matches the tail of the polygon
//           append it to the polygon
//           break out of the iteration
//         if control reaches here, the segments don't form a polygon -- ERROR!
//           the polygon is complete, add it to the collection

        unsorted.assign(segments.begin(), segments.end());

        while(unsorted.size() > 0)
        {
          segment & seg = unsorted.back();
          unsorted.pop_back();
          pointsIndexed.push_back(seg.high());
          pointsIndexed.push_back(seg.low());
          while(pointsIndexed.front() != pointsIndexed.back())
          {
            bool found = false;
            for (auto it = unsorted.begin(); it != unsorted.end(); ++it)
            {
              if (it->low() == pointsIndexed.back())
              {
                pointsIndexed.push_back(it->high());
                unsorted.erase(it);
                found = true;
                break;
              }
              else if (it->high() == pointsIndexed.back())
              {
                pointsIndexed.push_back(it->low());
                unsorted.erase(it);
                found = true;
                break;
              }
            }
            if (!found)
            {
              pointsIndexed.push_back(pointsIndexed.front());
              polygons.push_back(polygon(pointsIndexed.begin(), pointsIndexed.end()));
              pointsIndexed.clear();
              break;
            }
          }
        }

        auto biggest = polygons.begin();
        rect maxBB;

        for (auto it = polygons.begin(); it != polygons.end(); it++)
        {
          rect bb;
          if (boost::polygon::extents(bb, *it))
          {
            if (!boost::polygon::contains(maxBB, bb))
            {
              maxBB = bb;
              biggest = it;
            }
          }
          else
            it = polygons.erase(it);
        }

        poly.set(biggest->begin(), biggest->end());
        biggest = polygons.erase(biggest);
        poly.set_holes(polygons.begin(), polygons.end());

        std::cerr << "discovered: " << polygons.size() << " holes" << std::endl;


        //poly.set_holes(discovered.begin(), discovered.end());
      };

      i = 0;

//       std::ofstream fsvg("svg/map.svg");
//       boost::geometry::svg_mapper<point> mapper(fsvg, 200, 481);

      for (Cluster & c : clusters)
      {
        polygon poly;

        concaveHull(c.points, poly, 2);

        fipImage imageOut = myMapImg;

        std::ofstream fsvg("svg/" + boost::lexical_cast<std::string>(i) + ".svg");
        boost::geometry::svg_mapper<point> mapper(fsvg, 200, 481);//,  "width=\"200\" height=\"480\"");

        boost::geometry::model::polygon<point> po;

        for (const auto & p : poly)
        {
          boost::geometry::append(po, p);
          std::cerr << p << " ";
          RGBQUAD co = { 0, 0, 255, 255 };
          imageOut.setPixelColor(p.x(), p.y(), &co);
        }

        std::cerr << "\n";

        mapper.add(po);
        mapper.map(po, "fill-opacity:1.0;fill:rgb(153,204,0);stroke:rgb(0,0,0);stroke-width:1");

        std::uniform_int_distribution<int> randomColour(0, 255);

        for(auto i = poly.begin_holes(); i != poly.end_holes(); ++i)
        {
          boost::geometry::model::polygon<point> ph;

          int r = randomColour(myRandomEngine), g = randomColour(myRandomEngine), b = randomColour(myRandomEngine);

          for (const auto & p : *i)
          {
            boost::geometry::append(ph, p);
//             RGBQUAD co = { r, g, b, 255 };
//             imageOut.setPixelColor(p.x(), p.y(), &co);
          }

          mapper.add(ph);
          mapper.map(ph, "fill-opacity:0.8;fill:rgb(" +
            boost::lexical_cast<std::string>(r) + "," +
            boost::lexical_cast<std::string>(g) + "," +
            boost::lexical_cast<std::string>(b) + ");stroke:rgb(0,0,0);stroke-width:1");
        }

        std::string fileName = "png/out" + boost::lexical_cast<std::string>(i) + ".png";
        imageOut.flipVertical();
        imageOut.save(fileName.c_str());

        i++;

        break;
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

