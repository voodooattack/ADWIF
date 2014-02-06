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

  class GenerateTerrainTask: public std::enable_shared_from_this<GenerateTerrainTask>
  {
  public:
    GenerateTerrainTask(std::weak_ptr<MapGenerator> parent, int x, int y, int z,
                     int width, int height, int depth, bool regenerate):
      myGenerator(parent), myX(x), myY(y), myZ(z), myWidth(width),
      myHeight(height), myDepth(depth), myRegenFlag(regenerate),
      myDoneFlag(false), myAbortFlag(false), myPriority(0)
    {

    }

    std::shared_ptr<MapGenerator> generator() const { return myGenerator.lock(); }

    void run()
    {
      generator()->game()->engine()->log("GenerateAreaTask"),
        boost::str(boost::format("generating area %ix%ix%i with size %ix%ix%i") %
          myX % myY % myZ % myWidth % myHeight % myDepth);

      int counter = 0;

      for (int z = myZ; z > myZ - myDepth; z--)
      {
        for (unsigned int y = myY; y < myY + myHeight; y++)
        {
          for (unsigned int x = myX; x < myX + myWidth; x++)
          {
            if (myAbortFlag) {
              generator()->game()->engine()->log("GenerateAreaTask"),
                boost::str(boost::format("aborting area %ix%ix%i with size %ix%ix%i") %
                  myX % myY % myZ % myWidth % myHeight % myDepth);
              done(true);
              myGenerator.lock()->notifyComplete(shared_from_this());
              return;
            }
            if (myPriority && counter++ % myPriority == 0)
              generator()->game()->engine()->scheduler()->yield();
            int height = generator()->getHeight(x, y);
            if (z <= 0 || z <= height + 1)
              generateCell(x, y, z, height);
          }
        }
      }

      generator()->game()->engine()->log("GenerateAreaTask"),
        boost::str(boost::format("generated area %ix%ix%i with size %ix%ix%i") %
          myX % myY % myZ % myWidth % myHeight % myDepth);

      done(true);

      generator()->notifyComplete(shared_from_this());
    }

    std::pair<Material*,MaterialState> getMaterial(int x, int y, int z, int height, Biome * biome)
    {
      std::vector<std::string> possible;
      std::vector<double> probabilities;
      MaterialState state;

      if (biome->aquatic && z <= 0 && z > height)
      {
        state = MaterialState::Liquid;
        possible.assign(biome->liquids.begin(), biome->liquids.end());
      }
      else
      {
        state = MaterialState::Solid;
        possible.assign(biome->materials.begin(), biome->materials.end());
      }

      probabilities.assign(possible.size(), 1.0);

      std::discrete_distribution<int> dd(probabilities.begin(), probabilities.end());

      std::string material = possible[dd(generator()->random())];

      return std::make_pair(generator()->game()->materials()[material], state);
    }

    void generateCell(int x, int y, int z, int height)
    {
      MapCell c(generator()->game()->map()->get(x, y, z));

      if (c.generated() && !myRegenFlag)
        return;

      Biome * biome = generator()->game()->biomes()
      [
        generator()->biomeMap()[x / generator()->chunkSizeX()][y / generator()->chunkSizeY()].name
      ];

      c.clear();
      c.generated(true);

      MaterialMapElement * mat = nullptr;
      std::pair<Material*,MaterialState> m = getMaterial(x, y, z, height, biome);

      if (biome->aquatic && z <= 0 && z > height)
      {
        if (m.second == MaterialState::Liquid)
        {
          mat = new MaterialMapElement;

          mat->cmaterial = m.first;
          mat->material = mat->cmaterial->name;
          mat->state = m.second;

          auto elem = mat->cmaterial->states[mat->state].begin();

          std::advance(elem, std::uniform_int_distribution<int>(0,
            mat->cmaterial->states[mat->state].size() - 1)(generator()->random()));

          mat->element = *elem;
          mat->symIdx = std::uniform_int_distribution<int> (0,
            generator()->game()->elements()[mat->element]->disp[TerrainType::Wall].size() - 1)(generator()->random());
          mat->vol = MapCell::MaxVolume;
          mat->anchored = true;
          mat->state = MaterialState::Liquid;
          
          c.seen(true);
        }
      }
      else if (z == height)
      {
        mat = new MaterialMapElement;

        mat->cmaterial = m.first;
        mat->material = mat->cmaterial->name;
        mat->state = m.second;
        auto elem = mat->cmaterial->states[mat->state].begin();
        std::advance(elem, std::uniform_int_distribution<int>(0,
          mat->cmaterial->states[mat->state].size() - 1)(generator()->random()));
        mat->element = *elem;

        double h = generator()->getHeightReal(x,y);
        double vol = (h - double(height));

        vol = ((int)round(vol * 100) / 100.0);
        mat->symIdx = std::uniform_int_distribution<int> (0,
          generator()->game()->elements()[mat->element]->disp[TerrainType::Floor].size() - 1)(generator()->random());
        mat->vol = vol * MapCell::MaxVolume;

        if (mat->vol <= 0)
          mat->vol = 1;

        mat->anchored = true;
        mat->state = MaterialState::Solid;

        c.seen(true);
      }
      else if (z < height)
      {
        mat = new MaterialMapElement;

        mat->cmaterial = m.first;
        mat->material = mat->cmaterial->name;
        mat->state = m.second;
        auto elem = mat->cmaterial->states[mat->state].begin();
        std::advance(elem, std::uniform_int_distribution<int>(0,
          mat->cmaterial->states[mat->state].size() - 1)(generator()->random()));
        mat->element = *elem;
        mat->symIdx = std::uniform_int_distribution<int> (0,
          generator()->game()->elements()[mat->element]->disp[TerrainType::Wall].size() - 1)(generator()->random());
        mat->vol = MapCell::MaxVolume;
        mat->anchored = true;
        mat->state = MaterialState::Solid;

        c.seen(false);

        if (generator()->getHeight(x-1, y)   <= z || generator()->getHeight(x+1, y)   <= z ||
            generator()->getHeight(x, y-1)   <= z || generator()->getHeight(x, y+1)   <= z ||
            generator()->getHeight(x-1, y-1) <= z || generator()->getHeight(x+1, y+1) <= z ||
            generator()->getHeight(x+1, y-1) <= z || generator()->getHeight(x-1, y+1) <= z)
          c.seen(true);
      }

      if (mat)
        c.addElement(mat);

      generator()->game()->map()->set(x, y, z, c);
    }

    bool done() const { return myDoneFlag.load(); }
    void done(bool d) { myDoneFlag = d; }

    bool aborted() const { return myAbortFlag.load(); }
    void abort() { myAbortFlag.store(true); }

    int priority() const { return myPriority.load(); }
    void priority(int priority) { myPriority.store(priority); }

    std::weak_ptr<MapGenerator> myGenerator;
    int myX, myY, myZ, myWidth, myHeight, myDepth;
    bool myRegenFlag;
    boost::atomic_bool myDoneFlag;
    boost::atomic_bool myAbortFlag;
    boost::atomic_int myPriority;
  };

  MapGenerator::MapGenerator(const std::shared_ptr<Game> & game):
    myGame(game), myMapImg(), myHeightMap(), myChunkSizeX(32), myChunkSizeY(32), myChunkSizeZ(16),
    myColourIndex(), myRandomEngine(), myGenerationMap(),
    myBiomeMap(), myRegions(), myHeight(0), myWidth(0), myDepth(512),
    mySeed(boost::chrono::system_clock::now().time_since_epoch().count()), myGenerationLock(),
    myLastChunkX(-1), myLastChunkY(-1), myLastChunkZ(-1), myNoiseModules(), myNoiseModuleDefs(),
    myHeightSource(), myMapPreprocessingProgress(0), myInitialisedFlag(false)
  {
    myRandomEngine.seed(mySeed);
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
        myBiomeMap[x][y].aquatic = game()->biomes()[myColourIndex[colour]]->aquatic;
        myHeights[x][y] = height;

        myMapPreprocessingProgress += 1.0 / (double)(myWidth*myHeight) * 20.0;
      }

      // Clustering algorithm for terrain features

      boost::multi_array<bool,2> visited;
      visited.resize(boost::extents[myWidth][myHeight]);

      struct Cluster
      {
        std::string biome;
        flat_set<Point2D> points;
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

          flat_set<Point2D> points;
          std::deque<Point2D> q;

          q.push_back(Point2D(x,y));

          while(!q.empty())
          {
            Point2D n = q.back(); q.pop_back();
            uint32_t pcolour = getPixelColour(n.x(), n.y(), myMapImg);

            if (!visited[n.x()][n.y()] && colour == pcolour)
            {
              visited[n.x()][n.y()] = true;

              bool edge = false;

              if (n.x() > 0) q.push_back(Point2D(n.x() - 1,n.y())); else edge = true;
              if (n.x() < (signed)myWidth - 1) q.push_back(Point2D(n.x() + 1,n.y())); else edge = true;
              if (n.y() > 0) q.push_back(Point2D(n.x(),n.y() - 1)); else edge = true;
              if (n.y() < (signed)myHeight - 1) q.push_back(Point2D(n.x(),n.y() + 1)); else edge = true;

              if (n.x() > 0 && n.y() > 0) q.push_back(Point2D(n.x() - 1,n.y() - 1)); else edge = true;
              if (n.x() < (signed)myWidth - 1 && n.y() < (signed)myHeight - 1) q.push_back(Point2D(n.x() + 1,n.y() + 1)); else edge = true;
              if (n.x() < (signed)myWidth - 1 && n.y() > 0) q.push_back(Point2D(n.x() + 1,n.y() - 1)); else edge = true;
              if (n.x() > 0 && n.y() < (signed)myHeight - 1) q.push_back(Point2D(n.x() - 1,n.y() + 1)); else edge = true;

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

      auto concaveHull = [](const flat_set<Point2D> & in, Polygon & poly, double alpha, int mendRadius)
      {
        auto getCoords = [](const Point2D & p1, const Point2D & p2, double radius, bool dir) -> Point2D
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
          return Point2D(midX + pdx, midY + pdy);
        };

        flat_set<segment> segments;

        voronoi_diagram vd;
        boost::polygon::construct_voronoi(in.begin(), in.end(), &vd);

        for(const voronoi_diagram::edge_type & e : vd.edges())
        {
          const voronoi_diagram::cell_type * c1 = e.cell();
          const voronoi_diagram::cell_type * c2 = e.twin()->cell();

          const Point2D p1 = *(in.begin() + c1->source_index()), p2 = *(in.begin() + c2->source_index());
          const Point2D pp1 = getCoords(p1, p2, alpha, false);
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
            const Point2D p3 = *(in.begin() + c.source_index());
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

        std::vector<Polygon> polygons;

        //std::cerr << "discovered " << segments.size() << " segments." << std::endl;

        std::map<segment, bool> mendMap;

        auto mendNearest = [&](const Point2D & p, int r)
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
          std::list<Point2D> pointsIndexed;
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

          polygons.push_back(Polygon(pointsIndexed.begin(), pointsIndexed.end()));
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

  void MapGenerator::notifyLoad() {  }
  void MapGenerator::notifySave() {  }

  struct null_output_iterator: public std::iterator<std::output_iterator_tag, null_output_iterator> {
    template<typename T> void operator=(T const&) { }
    null_output_iterator & operator++() { return *this; }
    null_output_iterator operator++(int) { return *this; }
    null_output_iterator & operator*() { return *this; }
  };

  void MapGenerator::generateAround(int x, int y, int z, int radius, int radiusZ)
  {
    if (isGenerated(x / myChunkSizeX, y / myChunkSizeY, z / myChunkSizeZ))
      return;

    int chunkX = int(x / myChunkSizeX) * myChunkSizeX;
    int chunkY = int(y / myChunkSizeY) * myChunkSizeY;
    int chunkZ = int(z / myChunkSizeZ) * myChunkSizeZ;

    std::shared_ptr<GenerateTerrainTask> task;

    {
      boost::upgrade_lock<boost::shared_mutex> guard(myGenerationLock);

      if (chunkX == myLastChunkX && chunkY == myLastChunkY && chunkZ == myLastChunkZ)
        return;

      myLastChunkX = chunkX; myLastChunkY = chunkY; myLastChunkZ = chunkZ;


      if (!myIndex.empty())
      {
        std::vector<SIVal> items;
        myIndex.query(boost::geometry::index::intersects(
          Point3D(chunkX+myChunkSizeX/2,chunkY+myChunkSizeY/2,chunkZ+myChunkSizeZ/2)), std::back_inserter(items));
        if (!items.empty())
          task = items.begin()->second;

        if (task)
        {
          items.clear();
          myIndex.query(boost::geometry::index::satisfies([](SIVal const &) { return true; }), std::back_inserter(items));

          for(auto & i : items)
            if (i.second != task)
              if (!i.second->done())
                i.second->priority(1000);

          task->priority(0);
        }
      }

      boost::upgrade_to_unique_lock<boost::shared_mutex> lock(guard);

      if (!task)
      {
        task = std::shared_ptr<GenerateTerrainTask>(
          new GenerateTerrainTask(shared_from_this(), chunkX, chunkY, chunkZ,
                              myChunkSizeX, myChunkSizeY, myChunkSizeZ, false));
        task->priority(0);

        myIndex.insert(SIVal(Box3D(Point3D(chunkX, chunkY, chunkZ),
                                  Point3D(chunkX + myChunkSizeX,
                                          chunkY + myChunkSizeY,
                                          chunkZ + myChunkSizeZ)), task));

        game()->engine()->scheduler()->schedule(std::bind(&GenerateTerrainTask::run, task));
      }

      if (radius > 0) for (int r = -radius; r <= radius; r++)
      {
        for (int i = -r; i <= r; i++)
          for (int j = -r; j <= r; j++)
            for (int k = -r; k <= r; k++)
            {
              if (!(i == 0 && j == 0 && k == 0))
              {
                if (!myIndex.query(boost::geometry::index::intersects(
                                  Point3D(chunkX+i*myChunkSizeX+myChunkSizeX/2,
                                          chunkY+j*myChunkSizeY+myChunkSizeY/2,
                                          chunkZ+k*myChunkSizeZ+myChunkSizeZ/2)), null_output_iterator()))
                {
                  std::shared_ptr<GenerateTerrainTask> task(
                    new GenerateTerrainTask(shared_from_this(), chunkX+i*myChunkSizeX, chunkY+j*myChunkSizeY, chunkZ+k*myChunkSizeZ,
                                        myChunkSizeX, myChunkSizeY, myChunkSizeZ, false));
                  task->priority(10000);
                  myIndex.insert(SIVal(Box3D(Point3D(chunkX+i*myChunkSizeX, chunkY+j*myChunkSizeY, chunkZ+k*myChunkSizeZ),
                                            Point3D(chunkX+i*myChunkSizeX + myChunkSizeX,
                                                    chunkY+j*myChunkSizeY + myChunkSizeY,
                                                    chunkZ+k*myChunkSizeZ + myChunkSizeZ)), task));
                  game()->engine()->scheduler()->schedule(std::bind(&GenerateTerrainTask::run, task));
                }
              }
            }
      }
    }

    while (task && !task->done())
      boost::this_thread::sleep_for(boost::chrono::microseconds(500));
  }

  void MapGenerator::abort()
  {
    std::vector<SIVal> items;

    boost::upgrade_lock<boost::shared_mutex> guard(myGenerationLock);
    boost::upgrade_to_unique_lock<boost::shared_mutex> lock(guard);

    myIndex.query(boost::geometry::index::satisfies([&](SIVal const & val) {
      return true; }), std::back_inserter(items));

    for(auto & i : items)
      i.second->abort();
  }

  void MapGenerator::notifyComplete(const std::shared_ptr<GenerateTerrainTask> & task)
  {
    std::vector<SIVal> items;
    boost::upgrade_lock<boost::shared_mutex> guard(myGenerationLock);

    myIndex.query(boost::geometry::index::satisfies([&](SIVal const & val) {
      return val.second == task; }), std::back_inserter(items));

    boost::upgrade_to_unique_lock<boost::shared_mutex> lock(guard);
    for(auto & i : items)
      myIndex.remove(i);

    myGenerationMap[task->myX / myChunkSizeX]
                   [task->myY / myChunkSizeY]
                   [task->myZ / myChunkSizeZ+myDepth/2] = !task->aborted() && task->done();
  }

}

