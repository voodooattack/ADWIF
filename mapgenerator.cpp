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
#include "threadingutils.hpp"
#include "noisemodules.hpp"
#include "noiseutils.hpp"

#include <string>
#include <algorithm>

#include <physfs.hpp>
#include "mapcell.hpp"

#include <boost/container/flat_set.hpp>
#include <boost/polygon/polygon.hpp>
#include <boost/polygon/voronoi.hpp>
#include <boost/multi_array.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
// #include <boost/geometry.hpp>
// #include <boost/geometry/geometries/geometries.hpp>
// #include <boost/geometry/geometries/adapted/boost_polygon.hpp>
// #include <boost/assign.hpp>

namespace boost {
  namespace polygon {
    template<class T>
    std::ostream & operator<< (std::ostream & os, const boost::polygon::point_data<T> & p)
    {
      os << p.x() << "," << p.y();
      return os;
    }
  }
}

namespace ADWIF
{
  typedef boost::polygon::segment_data<double> segment;
  typedef boost::polygon::voronoi_diagram<double> voronoi_diagram;

  using boost::container::flat_set;

  MapGenerator::MapGenerator(const std::shared_ptr<Game> & game):
    myGame(game), myMapImg(), myHeightMap(), myChunkSizeX(32), myChunkSizeY(32), myChunkSizeZ(16),
    myColourIndex(), myRandomEngine(), myGenerationMap(), myPriorityFlags(),
    myBiomeMap(), myRegions(), myHeight(0), myWidth(0), myDepth(512),
    mySeed(boost::chrono::system_clock::now().time_since_epoch().count()),
    myGenerationLock(), myGeneratorCount(0), myGeneratorAbortFlag(false),
    myLastChunkX(-1), myLastChunkY(-1), myLastChunkZ(-1), myNoiseModules(), myNoiseModuleDefs(),
    myHeightSource(), myMapPreprocessingProgress(0), myInitialisedFlag(false)
  {
    myRandomEngine.seed(mySeed);
    myGeneratorAbortFlag.store(false);
    myMapPreprocessingProgress.store(0);
  }

  MapGenerator::~MapGenerator() { }

  void MapGenerator::init()
  {
    if (!myInitialisedFlag)
    {
      myInitialisedFlag = generateBiomeMap();
    }

    PhysFS::ifstream fs("map/heightgraph.json");
    std::string json;
    json.assign(std::istreambuf_iterator<std::string::value_type>(fs),
                std::istreambuf_iterator<std::string::value_type>());
    Json::Value value;
    Json::Reader reader;

    if (reader.parse(json, value))
    {
      std::shared_ptr<HeightMapModule> heightmap(new HeightMapModule(myHeights, myChunkSizeX, myChunkSizeY));
      myHeightSource = buildNoiseGraph(value, myNoiseModules, myNoiseModuleDefs, heightmap, mySeed);
    } else
      throw std::runtime_error("error parsing 'map/heightgraph.json'");
  }

  bool MapGenerator::generateBiomeMap()
  {
    for(auto const & b : game()->biomes())
      myColourIndex[b.second->mapColour] = b.second->name;

    myHeight = myMapImg.getHeight();
    myWidth = myMapImg.getWidth();
    myMapPreprocessingProgress.store(0);
    myBiomeMap.resize(boost::extents[myWidth][myHeight]);
    myHeights.resize(boost::extents[myWidth][myHeight]);
    myGenerationMap.resize(boost::extents[myWidth][myHeight][myDepth]);
    myPriorityFlags.resize(boost::extents[myWidth][myHeight][myDepth]);

    for(unsigned int y = 0; y < myHeight; y++)
      for(unsigned int x = 0; x < myWidth; x++)
      {
        uint32_t colour = getPixelColour(x, y, myMapImg);

        if (myColourIndex.find(colour) == myColourIndex.end())
        {
          game()->engine()->log("MapGenerator", LogLevel::Fatal),
            boost::str(boost::format("unknown biome colour %x at pixel %ix%i") % colour % x % y);
          return false;
        }

        RGBQUAD h;
        myHeightMap.getPixelColor(x, y, &h);
        double height = (-0.5 + (h.rgbBlue | h.rgbGreen | h.rgbRed) / 256.0);

        myBiomeMap[x][y].name = myColourIndex[colour];
        myBiomeMap[x][y].x = x;
        myBiomeMap[x][y].y = y;
        myBiomeMap[x][y].height = height;
        myBiomeMap[x][y].flat = game()->biomes()[myColourIndex[colour]]->flat;
        myHeights[x][y] = height;

        myMapPreprocessingProgress += 1.0 / (double)(myWidth*myHeight) * 20.0;
      }

      // Clustering algorithm for terrain features

      boost::multi_array<bool,2> visited;
      visited.resize(boost::extents[myWidth][myHeight]);

      struct Cluster
      {
        std::string biome;
        flat_set<point> points;
      };

      std::vector<Cluster> clusters;

      int i = 0;

      for(unsigned int y = 0; y < myHeight; y++)
      {
        for (unsigned int x = 0; x < myWidth; x++)
        {
          myMapPreprocessingProgress += 1.0 / (double)(myWidth*myHeight) * 30.0;

          if (visited[x][y]) continue;

          uint32_t colour = getPixelColour(x, y, myMapImg);

//           fipImage img(myMapImg);

          flat_set<point> points;
          std::deque<point> q;

          q.push_back(point(x,y));

          while(!q.empty())
          {
            point n = q.back(); q.pop_back();
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

              if (edge || ((int)n.x() % 2 == 0 && (int)n.y() % 2 == 0))
              //if (edge)
              {
                points.insert(n);

//                  if (edge)
//                  {
//                    RGBQUAD outer = { 0, 0, 0, 255 };
//                    img.setPixelColor(n.x(),n.y(), &outer);
//                  } else {
//                    RGBQUAD inner = { 0, 0, 255, 255 };
//                    img.setPixelColor(n.x(),n.y(), &inner);
//                  }
              }
            }
            else if (colour != pcolour)
            {

              points.insert(n);

//               RGBQUAD outer = { 0, 0, 0, 255 };
//               img.setPixelColor(n.x(),n.y(), &outer);
            }
          }

          if (points.empty()) continue;

          Cluster c;
          c.biome = myBiomeMap[x][y].name;
          c.points = points;

          clusters.push_back(c);

//           std::string fileName = saveDir + dirSep + "map" + dirSep + "png" + dirSep + boost::lexical_cast<std::string>(i++) + ".png";
//           img.flipVertical();
//           img.save(fileName.c_str());
        }
      }

      auto concaveHull = [](const flat_set<point> & in, polygon & poly, double alpha, int mendRadius)
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

        voronoi_diagram vd;
        boost::polygon::construct_voronoi(in.begin(), in.end(), &vd);

        for(const voronoi_diagram::edge_type & e : vd.edges())
        {
          const voronoi_diagram::cell_type * c1 = e.cell();
          const voronoi_diagram::cell_type * c2 = e.twin()->cell();

          const point p1 = *(in.begin() + c1->source_index()), p2 = *(in.begin() + c2->source_index());
          const point pp1 = getCoords(p1, p2, alpha, false);
          double dist = boost::polygon::distance_squared(p1, p2);

          //std::cerr << "detecting " << p1 << " to " << p2 << ": ";

          if (boost::polygon::distance_squared(p1, pp1) < dist || dist > alpha * alpha)
          {
            //std::cerr << "early abort, dist: " << dist << std::endl;
            continue;
          }

          bool obstructed = false;
//           point ob;

          for(const voronoi_diagram::cell_type & c : vd.cells())
          {
            const point p3 = *(in.begin() + c.source_index());
            if (p3 == p1 || p3 == p2) continue;
            if (boost::polygon::distance_squared(pp1, p3) < alpha * alpha / 2.0)
            {
//               ob = p3;
              obstructed = true;
              break;
            }
          }

          if (!obstructed)
          {
            segments.insert(segment(p1, p2));
            //std::cerr << "(" << p1 << " -> " << p2 << ") ";
          } //else std::cerr << "obstructed by " << ob;
          //std::cerr << std::endl;
        }

        std::vector<polygon> polygons;

        //std::cerr << "discovered " << segments.size() << " segments." << std::endl;

        std::map<segment, bool> mendMap;

        auto mendNearest = [&](const point & p, int r)
        {
          if (r <= 0)
            return segments.end();

          auto sg = std::find_if(segments.begin(), segments.end(), [&](const segment & s)
          {
            for (int y = p.y() - r; y < p.y() + r; y++)
              for (int x = p.x() - r; x < p.x() + r; x++)
                if (((s.low().x() == x && s.low().y() == y) ||
                    (s.high().x() == x && s.high().y() == y)))
                      return true;
            return false;
          });

          if (sg == segments.end())
            return segments.end();

          for (int radius = 1; radius <= r; radius++)
          {
            int x = radius, y = 0;
            int error = 1-x;

            while(x >= y)
            {
              if
              (
                (sg->low().x() ==  x + p.x() && sg->low().y() ==  y + p.y()) ||
                (sg->low().x() ==  y + p.x() && sg->low().y() ==  x + p.y()) ||
                (sg->low().x() == -x + p.x() && sg->low().y() ==  y + p.y()) ||
                (sg->low().x() == -y + p.x() && sg->low().y() ==  x + p.y()) ||
                (sg->low().x() == -x + p.x() && sg->low().y() == -y + p.y()) ||
                (sg->low().x() == -y + p.x() && sg->low().y() == -x + p.y()) ||
                (sg->low().x() ==  x + p.x() && sg->low().y() == -y + p.y()) ||
                (sg->low().x() ==  y + p.x() && sg->low().y() == -x + p.y())
              )
              {
                segment s(p, sg->low());
                if (!mendMap[s])
                {
                  mendMap[s] = true;
                  return segments.insert(s).first - 1;
                }
              }
              else if
              (
                (sg->high().x() ==  x + p.x() && sg->high().y() ==  y + p.y()) ||
                (sg->high().x() ==  y + p.x() && sg->high().y() ==  x + p.y()) ||
                (sg->high().x() == -x + p.x() && sg->high().y() ==  y + p.y()) ||
                (sg->high().x() == -y + p.x() && sg->high().y() ==  x + p.y()) ||
                (sg->high().x() == -x + p.x() && sg->high().y() == -y + p.y()) ||
                (sg->high().x() == -y + p.x() && sg->high().y() == -x + p.y()) ||
                (sg->high().x() ==  x + p.x() && sg->high().y() == -y + p.y()) ||
                (sg->high().x() ==  y + p.x() && sg->high().y() == -x + p.y())
              )
              {
                segment s(sg->high(), p);
                if (!mendMap[s])
                {
                  mendMap[s] = true;
                  return segments.insert(s).first - 1;
                }
              }

              y++;
              if(error < 0)
                error += 2 * y + 1;
              else
              {
                x--;
                error += 2 * (y - x + 1);
              }
            }
          }

          return segments.end();
        };

        while(!segments.empty())
        {
          //std::cerr << "constructing polygon: ";
          std::list<point> pointsIndexed;
          segment seg = *segments.begin();
          segments.erase(segments.begin());
          pointsIndexed.push_back(seg.high());
          pointsIndexed.push_back(seg.low());
          //std::cerr << seg.low() << " -> " << seg.high();
          while(pointsIndexed.front() != pointsIndexed.back())
          {
            bool found = false;
            for (auto it = segments.begin(); it != segments.end(); ++it)
            {
//               if (it->low() == pointsIndexed.back())
//               {
//                 pointsIndexed.push_back(it->high());
//                 //std::cerr << " -> " << it->high();
//                 segments.erase(it);
//                 found = true;
//                 break;
//               }
              if (it->high() == pointsIndexed.back())
              {
                pointsIndexed.push_back(it->low());
                //std::cerr << " -> " << it->low();
                segments.erase(it);
                found = true;
                break;
              }
              if (it->low() == pointsIndexed.front())
              {
                pointsIndexed.push_front(it->high());
                //std::cerr << " -> " << it->high();
                segments.erase(it);
                found = true;
                break;
              }
//               if (it->high() == pointsIndexed.front())
//               {
//                 pointsIndexed.push_front(it->low());
//                 //std::cerr << " -> " << it->low();
//                 segments.erase(it);
//                 found = true;
//                 break;
//               }
              if (!found && it == segments.end() - 1 && mendRadius > 0)
              {
                //std::cerr << " {mending: ";
                auto m = mendNearest(pointsIndexed.back(), mendRadius);
                if (m == segments.end())
                  m = mendNearest(pointsIndexed.front(), mendRadius);
                if (m != segments.end())
                {
                  //std::cerr << "ok}";
                  it = m;
                }
                else
                {
                  //std::cerr << "n/a}";
                  break;
                }
              }
            }
            if (!found)
            {
              pointsIndexed.push_back(pointsIndexed.front());
              break;
            }
          }

          polygons.push_back(polygon(pointsIndexed.begin(), pointsIndexed.end()));
          //std::cerr << std::endl;
        }

        auto biggest = polygons.begin();
        double maxArea = 0;

        for (auto it = polygons.begin(); it != polygons.end(); it++)
        {
          double area = boost::polygon::area(*it);
          if (area > maxArea)
          {
            maxArea = area;
            biggest = it;
          }
        }

        if (biggest != polygons.end())
        {
          poly.set(biggest->begin(), biggest->end());
          polygons.erase(biggest);
          poly.set_holes(polygons.begin(), polygons.end());
        }

        //std::cerr << "discovered: " << polygons.size() << " holes" << std::endl;
      };

//       i = 0;

//       std::ofstream fsvg(saveDir + dirSep + "map.svg");
//       boost::geometry::svg_mapper<point> mapper(fsvg, myWidth, myHeight);

      boost::atomic_size_t clusterCompletionCount;
      boost::mutex regionMutex;

      clusterCompletionCount.store(clusters.size());

      auto calculateHullThread = [&](Cluster & c) -> void
      {
        AtomicRefCount<std::size_t> refCount(clusterCompletionCount, false);
        Region region;

        concaveHull(c.points, region.poly, 1.5, 0);

        boost::polygon::center(region.centroid, region.poly);

        region.biome = c.biome;
        region.area = boost::polygon::area(region.poly);

        {
          boost::mutex::scoped_lock guard(regionMutex);
          myRegions.push_back(boost::move(region));
        }

        myMapPreprocessingProgress += 1.0 / (double)clusters.size() * 50.0;
      };

      for (Cluster & c : clusters)
        game()->engine()->service().post(boost::bind<void>(calculateHullThread, c));

      while(clusterCompletionCount > 0) game()->engine()->service().poll_one();

      myMapPreprocessingProgress.store(100);

//       for (Region & r : myRegions)
//       {
//         fipImage imageOut = myMapImg;
//
//         imageOut.convertToGrayscale();
//         imageOut.convertTo32Bits();
// //
// //         std::ofstream fsvg(saveDir + dirSep + "map" + dirSep + "svg" + dirSep + boost::lexical_cast<std::string>(i) + ".svg");
// //         boost::geometry::svg_mapper<point> mapper(fsvg, 200, 481);//,  "width=\"200\" height=\"480\"");
// //
// //         boost::geometry::model::polygon<point> po;
//
//         for (const auto & p : r.poly)
//         {
// //           boost::geometry::append(po, p);
//           //std::cerr << p << " ";
//           RGBQUAD co = { 0, 0, 255, 255 };
//           imageOut.setPixelColor(p.x(), p.y(), &co);
//         }
// //
// //         //std::cerr << "\n";
// //
//         std::uniform_int_distribution<int> randomColour(64, 192);
// //
//         for(auto i = r.poly.begin_holes(); i != r.poly.end_holes(); ++i)
//         {
// //           boost::geometry::model::ring<point> ph;
// //
//           int r = randomColour(myRandomEngine),
//               g = randomColour(myRandomEngine),
//               b = randomColour(myRandomEngine);
// //
//           for (const auto & p : *i)
//           {
// //             boost::geometry::append(ph, p);
//             RGBQUAD co = { (BYTE)r, (BYTE)g, (BYTE)b, 255 };
//             imageOut.setPixelColor(p.x(), p.y(), &co);
//           }
// //
// //           po.inners().push_back(ph);
//         }
//
// //         boost::geometry::correct(po);
//
// //         mapper.add(po);
// //         mapper.map(po, "fill-opacity:1.0;fill:" +
// //           colourToHexString(myGame->biomes()[c.biome]->mapColour) +
// //             ";stroke:rgb(0,0,0);stroke-width:1");
// // //
//         std::string fileName = (saveDir / "png" / "out" / (boost::lexical_cast<std::string>(i) + ".png")).native();
//         imageOut.flipVertical();
//         imageOut.save(fileName.c_str());
//
//         i++;
//       }
    return true;
  }

  void MapGenerator::generateAll()
  {
    for(int y = 0; y < myHeight; y++)
      for(int x = 0; x < myWidth; x++)
        for(int z = -myDepth / 2; z < myDepth / 2; z++)
        {
          generateOne(x, y, z);
        }
  }

  void MapGenerator::generateOne(int x, int y, int z, bool regenerate, bool cooperative)
  {
    if (x < 0 || y < 0 || x >= myHeight || y >= myWidth)
      return;

    if (myGeneratorAbortFlag)
    {
      boost::recursive_mutex::scoped_lock guard(myGenerationLock);
      myGenerationMap[x][y][z+myDepth/2] = false;
      myPriorityFlags[x][y][z+myDepth/2] = false;
      return;
    }
    else
    {
      boost::recursive_mutex::scoped_lock guard(myGenerationLock);
      if (myGenerationMap[x][y][z+myDepth/2] == boost::indeterminate) return;
      if (myGenerationMap[x][y][z+myDepth/2] == true && !regenerate) return;
      myGenerationMap[x][y][z+myDepth/2] = boost::indeterminate;
    }

    game()->engine()->log("MapGenerator"), boost::str(boost::format("generating cell: %ix%ix%i") % x % y % z);

    Map * map = game()->map().get();

    AtomicRefCount<std::size_t> refCount(myGeneratorCount);

    int offx = x * myChunkSizeX, offy = y * myChunkSizeY;
    int offz = z * myChunkSizeZ - myChunkSizeZ + 1;

    Biome * biome = game()->biomes()[myBiomeMap[x][y].name];

//     std::vector<Region> regions;
//
//     for(auto & r : myRegions)
//       if (boost::polygon::contains(r.poly, point(x,y), true))
//         regions.push_back(r);
//
//     if (regions.empty())
//       throw std::runtime_error(boost::str(boost::format("could not find region for biome cell %ix%i") % x %y));
//
//     std::vector<point> neighbours;
//
//     {
//       flat_set<point> vdpoints;
//       const point coords(x,y);
//
//       vdpoints.insert(coords);

//       vdpoints.insert(point(x-1,y));
//       vdpoints.insert(point(x,y-1));
//       vdpoints.insert(point(x+1,y));
//       vdpoints.insert(point(x,y+1));
//       vdpoints.insert(point(x-1,y-1));
//       vdpoints.insert(point(x+1,y-1));
//       vdpoints.insert(point(x-1,y+1));
//       vdpoints.insert(point(x+1,y+1));

//       auto msg(game()->engine()->log("MapGenerator"));
//       msg, boost::format("map cell %ix%ix%i intersects regions: ") % x % y % z;
//       for(auto & r : regions)
//       {
//         msg, r.centroid, " (", r.biome, ") ";
//         for(const point & p : r.poly)
//           if (boost::polygon::distance_squared(p, coords) < 4)
//             vdpoints.insert(p);
//       }
//       msg.flush();
//
//       if (vdpoints.size() < 3)
//       {
//         for (auto const & p : vdpoints)
//           if (p != coords)
//             neighbours.push_back(p);
//       }
//       else
//       {
//         voronoi_diagram vd;
//
//         boost::polygon::construct_voronoi(vdpoints.begin(), vdpoints.end(), &vd);
//
//         for(const voronoi_diagram::cell_type & c : vd.cells())
//         {
//           const point & p = *(vdpoints.begin() + c.source_index());
//           if (p == coords)
//           {
//             const voronoi_diagram::edge_type * edge = c.incident_edge();
//             do
//             {
//   //             if (!edge->twin()) { edge = edge->next(); continue; } // bug?
//               const voronoi_diagram::cell_type * nc = edge->twin()->cell();
//               const point & np = *(vdpoints.begin() + nc->source_index());
//               neighbours.push_back(np);
//               edge = edge->next();
//             } while(edge != c.incident_edge());
//             break;
//           }
//         }
//       }
//     }
//
//     auto msg(game()->engine()->log("MapGenerator"));
//     msg, boost::format("found %i neighbours for cell %ix%ix%i: ") % neighbours.size() % x % y % z;
//     for(const point & p : neighbours)
//       msg, p, " (", myBiomeMap[p.x()][p.y()].name, ") ";
//     msg.flush();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    std::vector<std::string> possible;
    std::vector<double> probabilities;
//
    possible.assign(biome->materials.begin(), biome->materials.end());
    probabilities.assign(possible.size(), 1.0);
//
//     if (!biome->background) for (const point & p : neighbours)
//     {
//       if (p.x() > 0 && p.y() > 0 && p.x() < myWidth && p.y() < myHeight)
//       {
//         Biome * b = game()->biomes()[myBiomeMap[p.x()][p.y()].name];
//         for (const std::string m : b->materials)
//           if (!game()->materials()[m]->liquid)
//           {
//             double dist = boost::polygon::distance_squared(point(x,y), p);
//             probabilities.push_back(dist ? 1.0 / dist : 1.0);
//             possible.push_back(m);
//           }
//       }
//     }

//     game()->engine()->log("MapGenerator"), "possible materials: ", possible.size();

//     std::function<std::string(int,int,int,int)> getMaterial;
//
//     std::discrete_distribution<int> dd(probabilities.begin(), probabilities.end());
//     std::uniform_int_distribution<int> ud(0, biome->materials.size()-1);
//     std::bernoulli_distribution bd(0.001);
//
//     noise::module::Perlin perlinMat;
//     noise::module::ScaleBias scaleMat;
//     noise::module::Clamp clampMat;

//     if (possible.size() > 1)
//     {
//       perlinMat.SetSeed(mySeed << 2);
//       perlinMat.SetFrequency(0.06);
//       perlinMat.SetPersistence(0.4);
//       perlinMat.SetLacunarity(1.50);
//       perlinMat.SetOctaveCount(1);
//       perlinMat.SetNoiseQuality(noise::QUALITY_FAST);
//
//       scaleMat.SetSourceModule(0, *myHeightSource);
//       scaleMat.SetBias(2);
//       scaleMat.SetScale(possible.size() * 0.005);
//
//       clampMat.SetSourceModule(0, scaleMat);
//       clampMat.SetBounds(0, possible.size()-1);
//
//       getMaterial = [&](int xx, int yy, int zz, int height) -> std::string
//       {
//         double closest = -1;
//         int iclosest = -1;
//         for (int i = 0; i < neighbours.size(); i++)
//         {
//           int nx = neighbours[i].x() * myChunkSizeX + myChunkSizeX / 2.0,
//               ny = neighbours[i].y() * myChunkSizeY + myChunkSizeY / 2.0;
//           double dist = boost::polygon::distance_squared(point(xx,yy), point(nx,ny));
//           if (closest == -1 || dist < closest)
//           {
//             closest = dist;
//             iclosest = i;
//             if (dist == 0) break;
//           }
//         }
//
//         if (iclosest == -1)
//           return possible[0];
//
//         const Biome * b = game()->biomes()[myBiomeMap[neighbours[iclosest].x()][neighbours[iclosest].y()].name];
//         std::uniform_int_distribution<int> udd(0, b->materials.size()-1);
//         int nx = neighbours[iclosest].x() * myChunkSizeX + myChunkSizeX / 2.0,
//             ny = neighbours[iclosest].y() * myChunkSizeY + myChunkSizeY / 2.0;
//         int cx = xx / myChunkSizeX + myChunkSizeX / 2.0, cy = yy / myChunkSizeY + myChunkSizeY / 2.0;
//         if (boost::polygon::distance_squared(point(xx,yy), point(nx,ny)) <
//             boost::polygon::distance_squared(point(nx,ny), point(cx,cy)) / 2.0)
//           return b->materials[udd(myRandomEngine)];
//         else
//           return biome->materials[ud(myRandomEngine)];
//         int idx = floor(clampMat.GetValue(xx,yy,zz));
//         int idx = dd(myRandomEngine);
// //         std::cerr << clampType.GetValue(xx,yy,zz) << std::endl;
//         return possible[idx];
//       };
//     }
//     else
    auto getMaterial = [&](int xx, int yy, int zz, int height) -> std::string { return possible[0]; };

    int counter = 0;

    auto generateCell = [&](int xx, int yy, int zz, int height)
    {
      if (zz > height) return;

      MapCell c(map->get(xx, yy, zz));

      if (c.generated() && !regenerate)
        return;

      c.clear();

      MaterialElement * mat = new MaterialElement;

      mat->material = getMaterial(xx, yy, zz, height);
      mat->cmaterial = game()->materials()[mat->material];

      if (zz == height)
      {
        c.generated(true);

        std::uniform_int_distribution<int> ud2(0, mat->cmaterial->disp[TerrainType::Floor].size() - 1);
        mat->symIdx = ud2(myRandomEngine);
        mat->vol = 100000000;
        mat->anchored = true;
        mat->alignment = false;
        mat->state = MaterialState::Solid;

//         if (!mat->cmaterial->liquid &&
//           (getHeight(xx+1,yy) == height + 1 ||
//            getHeight(xx,yy+1) == height + 1 ||
//            getHeight(xx-1,yy) == height + 1 ||
//            getHeight(xx,yy-1) == height + 1))
//         {
// //           if (std::abs(1 - (double)height - getHeightReal(xx,yy)) < 0.5)
//           c.type(TerrainType::RampU);
//           mat->symIdx = 0;
//           mat->vol = 500000000;
//           MapCell cc;
//           MaterialElement * cmat = new MaterialElement;
//           cmat->material = getMaterial(xx, yy, zz, height+1);
//           cmat->cmaterial = game()->materials()[mat->material];
//           cmat->vol = 500000000;
//           cmat->symIdx = 0;
//           cmat->anchored = true;
//           cmat->alignment = true;
//           cmat->state = MaterialState::Solid;
//           cc.type(TerrainType::RampD);
//           cc.generated(true);
//           cc.addElement(cmat);
//           map->set(xx, yy, zz+1, cc);
//         }
      }
      else if (zz < height)
      {
        if (mat->cmaterial->disp.find(TerrainType::Wall) == mat->cmaterial->disp.end())
          return;
        std::uniform_int_distribution<int> ud2(0, mat->cmaterial->disp[TerrainType::Wall].size() - 1);
        mat->symIdx = ud2(myRandomEngine);
        mat->vol = MapCell::MaxVolume;
        mat->anchored = true;
        mat->state = MaterialState::Solid;

        c.seen(false);
        c.generated(true);

        int hw = getHeight(xx-1, yy);
        int he = getHeight(xx+1, yy);
        int hn = getHeight(xx, yy-1);
        int hs = getHeight(xx, yy+1);

        int hnw = getHeight(xx-1, yy-1);
        int hse = getHeight(xx+1, yy+1);
        int hne = getHeight(xx+1, yy-1);
        int hsw = getHeight(xx-1, yy+1);

        if (he <= zz || hw <= zz || hs <= zz || hn <= zz ||
            hnw <= zz || hsw <= zz || hse <= zz || hne <= zz)
          c.seen(true);
      }

      c.addElement(mat);

      map->set(xx, yy, zz, c);
    };

    // PASS 1 ============================================================================

    for (unsigned int yy = offy; yy < offy + myChunkSizeY; yy++)
    {
      if (myGeneratorAbortFlag)
      {
        boost::recursive_mutex::scoped_lock guard(myGenerationLock);
        myGenerationMap[x][y][z+myDepth/2] = false;
        myPriorityFlags[x][y][z+myDepth/2] = false;
        return;
      }
      if (cooperative && !myPriorityFlags[x][y][z+myDepth/2] && counter++ % 2000 == 0)
        game()->engine()->scheduler()->yield();
      for (unsigned int xx = offx; xx < offx + myChunkSizeX; xx++)
      {
        int height = getHeight(xx,yy);
        for (int zz = offz; zz <= (offz + myChunkSizeZ > height ? height : offz + myChunkSizeZ); zz++)
          generateCell(xx, yy, zz, height);
      }
    }

    // PASS 2 ============================================================================

//     for (unsigned int yy = offy; yy < offy + myChunkSizeY; yy++)
//     {
//       if (cooperative && !myPriorityFlags[x][y][z+myDepth/2] && counter++ % 100 == 0)
//         game()->engine()->scheduler()->yield();
//       for (unsigned int xx = offx; xx < offx + myChunkSizeX; xx++)
//       {
//         if (myGeneratorAbortFlag)
//         {
//           boost::recursive_mutex::scoped_lock guard(myGenerationLock);
//           myGenerationMap[x][y][z+myDepth/2] = false;
//           return;
//         }
//
//         int height = getHeight(xx,yy);
//         for (int zz = offz; zz <= (offz + myChunkSizeZ > height ? height : offz + myChunkSizeZ); zz++)
//         {
//           MapCell c(map->get(xx, yy, zz));
//           if (c.cmaterial && c.cmaterial->liquid)
//           {
//             c.visible = true;
//             map->set(xx, yy, zz, c);
//             for (int i = -1; i <= 1; i++)
//               for (int j = -1; j <= 1; j++)
// //                 for (int k = -1; k <= 1; k++)
//                 {
//                   MapCell cc(map->get(xx+i,yy+j,zz/*+k*/));
//                   if (!cc.visible)
//                   {
//                     cc.visible = true;
//                     map->set(xx+i,yy+j,zz/*+k*/, cc);
//                   }
//                 }
//           }
//           else if (c.type == TerrainType::Wall)
//           {
//             const MapCell & w = map->get(xx-1, yy, zz);
//             const MapCell & e = map->get(xx+1, yy, zz);
//             const MapCell & n = map->get(xx, yy-1, zz);
//             const MapCell & s = map->get(xx, yy+1, zz);
//
//             double hw = getHeight(xx-1, yy);
//             double he = getHeight(xx+1, yy);
//             double hn = getHeight(xx, yy-1);
//             double hs = getHeight(xx, yy+1);
//
//             if (
//               ((he < zz || (!e.background && e.type != TerrainType::Wall)) ||
//                (hw < zz || (!w.background && w.type != TerrainType::Wall)) ||
//                (hs < zz || (!s.background && s.type != TerrainType::Wall)) ||
//                (hn < zz || (!n.background && n.type != TerrainType::Wall)) ||
//                (!e.background && e.cmaterial && e.cmaterial->liquid) ||
//                (!e.background && w.cmaterial && w.cmaterial->liquid) ||
//                (!e.background && s.cmaterial && s.cmaterial->liquid) ||
//                (!e.background && n.cmaterial && n.cmaterial->liquid)))
//               c.visible = true;
//             map->set(xx, yy, zz, c);
//           }
//         }
//       }
//     }

    {
      boost::recursive_mutex::scoped_lock guard(myGenerationLock);
      myGenerationMap[x][y][z+myDepth/2] = true;
      myPriorityFlags[x][y][z+myDepth/2] = false;
    }

    game()->engine()->log("MapGenerator"), boost::str(boost::format("generated cell: %ix%ix%i") % x % y % z);
  }

  void MapGenerator::notifyLoad() {  }
  void MapGenerator::notifySave() {  }

  void MapGenerator::generateAround(int x, int y, int z, int radius, int radiusZ)
  {
    int chunkX = x / myChunkSizeX;
    int chunkY = y / myChunkSizeY;
    int chunkZ = z / myChunkSizeZ;

    if (chunkX == myLastChunkX && chunkY == myLastChunkY && chunkZ == myLastChunkZ)
      return;

    myLastChunkX = chunkX; myLastChunkY = chunkY; myLastChunkZ = chunkZ;

    myPriorityFlags[chunkX][chunkY][chunkZ+myDepth/2] = true;

//     if (isGenerated(chunkX, chunkY, chunkZ) == false)
//       abort();

    if (radius > 0) for (int r = -radius; r <= radius; r++)
    {
      for (int i = -r; i <= r; i++)
        for (int j = -r; j <= r; j++)
          for (int k = -r; k <= r; k++)
            if (isGenerated(chunkX+i, chunkY+j, chunkZ+k) == false)
              if (!(i == 0 && j == 0 && k == 0))
                game()->engine()->scheduler()->schedule(boost::bind<void>(&MapGenerator::generateOne, this,
                                                        chunkX+i, chunkY+j, chunkZ+k, false, true));
    }

    generateOne(chunkX, chunkY, chunkZ, false, false);

    do
    {
      if (isGenerated(chunkX, chunkY, chunkZ) == true)
        break;
      else
        boost::this_thread::sleep_for(boost::chrono::microseconds(50));
    } while(true);
  }

  void MapGenerator::abort()
  {
    myGeneratorAbortFlag.store(true);
    while (myGeneratorCount) game()->engine()->service().poll_one();
    myGeneratorAbortFlag.store(false);
  }

}

