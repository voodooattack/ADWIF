#include "mapgenstate.hpp"
#include "player.hpp"
#include "engine.hpp"
#include "map.hpp"
#include "game.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "mapgenerator.hpp"

#include <physfs.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <stdio.h>

namespace ADWIF
{
  MapGenState::MapGenState(std::shared_ptr<ADWIF::Engine> & engine, std::shared_ptr<ADWIF::Game> & game):
    myEngine(engine), myGame(game), myGenerator(game), myRandomEngine(), myViewOffX(0), myViewOffY(0)
  {
    myEngine->delay(30);
    myEngine->input()->setTimeout(-1);
  }

  MapGenState::~MapGenState() { myEngine->delay(50); myEngine->input()->setTimeout(0); }

  fipImage & loadImageFromFile(const std::string & path, fipImage & image)
  {
    PHYSFS_File * file = PHYSFS_openRead(path.c_str());
    FreeImageIO io = {
      [](void *buffer, unsigned size, unsigned count, fi_handle handle) -> unsigned {
        return PHYSFS_read((PHYSFS_File*)handle, buffer, size, count);
      },
      [](void *buffer, unsigned size, unsigned count, fi_handle handle) -> unsigned {
        return PHYSFS_write((PHYSFS_File*)handle, buffer, size, count);
      },
      [](fi_handle handle, long offset, int origin) -> int {
        long org = 0;
        switch(origin)
        {
          case SEEK_END: org = PHYSFS_fileLength((PHYSFS_File*)handle); break;
          case SEEK_CUR: org = PHYSFS_tell((PHYSFS_File*)handle); break;
          case SEEK_SET:
          default: org = 0; break;
        }
        return PHYSFS_seek((PHYSFS_File*)handle, org + offset);
      },
      [](fi_handle handle) -> long {
        return PHYSFS_tell((PHYSFS_File*)handle);
      }
    };

    if (!image.loadFromHandle(&io, reinterpret_cast<fi_handle>(file), 0))
    {
      PHYSFS_close(file);
      throw std::runtime_error("error loading image from stream");
    }
    else
    {
      PHYSFS_close(file);
      return image;
    }
  }

//   fipImage & loadImageFromStream(std::istream & stream, fipImage & image)
//   {
//     FreeImageIO io = {
//       [](void *buffer, unsigned size, unsigned count, fi_handle handle) -> unsigned {
//         reinterpret_cast<std::istream*>(handle)->read(
//           (char*)buffer, count);
//         if (reinterpret_cast<std::istream*>(handle)->good()) return reinterpret_cast<std::istream*>(handle)->gcount(); else return 0;
//       },
//       [](void *buffer, unsigned size, unsigned count, fi_handle handle) -> unsigned { return 1; },
//       [](fi_handle handle, long offset, int origin) -> int {
//         std::ios_base::seekdir dir;
//         switch(origin)
//         {
//           case SEEK_END: dir = std::ios_base::end; break;
//           case SEEK_CUR: dir = std::ios_base::cur; break;
//           case SEEK_SET:
//           default: dir = std::ios_base::beg; break;
//         }
//         reinterpret_cast<std::istream*>(handle)->seekg(offset, dir);
//         return reinterpret_cast<std::istream*>(handle)->good() ? 0 : 1;
//       },
//       [](fi_handle handle) -> long {
//         return reinterpret_cast<std::istream*>(handle)->tellg();
//       }
//     };
//     if (image.loadFromHandle(&io, reinterpret_cast<fi_handle>(&stream), 0))
//       return image;
//     else
//       throw std::runtime_error("error loading image from stream");
//   }

  void MapGenState::init()
  {
/*    if (boost::filesystem::exists(saveDir + dirSep + "map"))
      myGame->loadMap();
    else */
      myGame->createMap();

    fipImage mapImg;
    fipImage hmapImg;

    loadImageFromFile("/map/map.png", mapImg);
    loadImageFromFile("/map/heightmap.png", hmapImg);

    mapImg.flipVertical();
    hmapImg.flipVertical();

    myGenerator.mapImage(mapImg);
    myGenerator.heightmapImage(hmapImg);

    myGenerator.chunkSizeX(1024);
    myGenerator.chunkSizeY(1024);

    //myViewOffX = (mapImg.getWidth() / 2) * myGenerator.chunkSizeX();
    //myViewOffY = (mapImg.getHeight() / 2) * myGenerator.chunkSizeY();

    myGenerator.init();

    myGenerator.generateOne(myViewOffX / myGenerator.chunkSizeX(),
                            myViewOffY / myGenerator.chunkSizeY());

  }

  void MapGenState::step()
  {
    myEngine->renderer()->clear();
    myEngine->renderer()->drawRegion(myViewOffX, myViewOffY, 0, myEngine->renderer()->width(), myEngine->renderer()->height(),
                                     0, 0, myGame.get(), myGame->map().get());
    myEngine->renderer()->style(White, Black, Style::Bold);
    myEngine->renderer()->drawChar(myEngine->renderer()->width() / 2, myEngine->renderer()->height() / 2, '@');
  }

  void MapGenState::consume(int key) {
//     std::stringstream ss;
//     ss << key;
//     myGame->engine()->reportError(true, ss.str());
    if (key == Key::Escape)
    {
      myGame->saveMap();
      done(true);
    }
    else if (key == Key::Up)
    {
      myViewOffY -= 10;
      myGenerator.generateOne((myViewOffX + myEngine->renderer()->width() / 2) / myGenerator.chunkSizeX(),
                              (myViewOffY + myEngine->renderer()->height() / 2) / myGenerator.chunkSizeY());
    }
    else if (key == Key::Down)
    {
      myViewOffY += 10;
      myGenerator.generateOne((myViewOffX + myEngine->renderer()->width() / 2) / myGenerator.chunkSizeX(),
                              (myViewOffY + myEngine->renderer()->height() / 2) / myGenerator.chunkSizeY());
    }
    else if (key == Key::Left)
    {
      myViewOffX -= 10;
      myGenerator.generateOne((myViewOffX + myEngine->renderer()->width() / 2) / myGenerator.chunkSizeX(),
                              (myViewOffY + myEngine->renderer()->height() / 2) / myGenerator.chunkSizeY());
    }
    else if (key == Key::Right)
    {
      myViewOffX += 10;
      myGenerator.generateOne((myViewOffX + myEngine->renderer()->width() / 2) / myGenerator.chunkSizeX(),
                              (myViewOffY + myEngine->renderer()->height() / 2) / myGenerator.chunkSizeY());
    }
    else if (key == 'c')
      myGame->createMap();
    else if (key == 'l')
      myGame->loadMap();
    else if (key == 's')
      myGame->saveMap();
    else if (key == 'g')
    {
      myEngine->input()->setTimeout(0);
      for(unsigned int y = 0; y < myGenerator.height(); y++)
        for(unsigned int x = 0; x < myGenerator.width(); x++)
        {
          std::stringstream ss;
          ss << x << "x" << y;
          std::string str = "Generating " + ss.str();
          myEngine->renderer()->startWindow(1,1,myEngine->renderer()->width() - 2, 1);
          myEngine->renderer()->style(White, Black, Style::Bold);
          myEngine->renderer()->drawText(0,0, str + std::string(myEngine->renderer()->width() - 2 - str.size(),  ' '));
          myEngine->renderer()->refresh();
          myEngine->renderer()->endWindow();
          myGenerator.generateOne(x, y);
          if (myEngine->input()->key() == Key::Escape)
          {
            myEngine->input()->setTimeout(-1);
            return;
          }
          //myEngine->sleep(0);
        }
    }
  }

  void MapGenState::activate() { }

}

