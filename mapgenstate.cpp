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
    myEngine(engine), myGame(game), myViewOffX(0), myViewOffY(0)
  {
    myEngine->delay(30);
    myEngine->input()->setTimeout(-1);
  }

  MapGenState::~MapGenState() {
    myEngine->delay(50); myEngine->input()->setTimeout(0);
  }

  void MapGenState::init()
  {
    if (boost::filesystem::exists(saveDir + dirSep + "map"))
      myGame->loadMap();
    else
      myGame->createMap();

    //myViewOffX = (mapImg.getWidth() / 2) * myGame->generator()->chunkSizeX();
    //myViewOffY = (mapImg.getHeight() / 2) * myGame->generator()->chunkSizeY();

    myGame->generator()->generateOne(myViewOffX / myGame->generator()->chunkSizeX(),
                            myViewOffY / myGame->generator()->chunkSizeY());
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
      myGame->generator()->generateOne((myViewOffX + myEngine->renderer()->width() / 2) / myGame->generator()->chunkSizeX(),
                              (myViewOffY + myEngine->renderer()->height() / 2) / myGame->generator()->chunkSizeY());
    }
    else if (key == Key::Down)
    {
      myViewOffY += 10;
      myGame->generator()->generateOne((myViewOffX + myEngine->renderer()->width() / 2) / myGame->generator()->chunkSizeX(),
                              (myViewOffY + myEngine->renderer()->height() / 2) / myGame->generator()->chunkSizeY());
    }
    else if (key == Key::Left)
    {
      myViewOffX -= 10;
      myGame->generator()->generateOne((myViewOffX + myEngine->renderer()->width() / 2) / myGame->generator()->chunkSizeX(),
                              (myViewOffY + myEngine->renderer()->height() / 2) / myGame->generator()->chunkSizeY());
    }
    else if (key == Key::Right)
    {
      myViewOffX += 10;
      myGame->generator()->generateOne((myViewOffX + myEngine->renderer()->width() / 2) / myGame->generator()->chunkSizeX(),
                              (myViewOffY + myEngine->renderer()->height() / 2) / myGame->generator()->chunkSizeY());
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
      for(unsigned int y = 0; y < myGame->generator()->height(); y++)
        for(unsigned int x = 0; x < myGame->generator()->width(); x++)
        {
          std::stringstream ss;
          ss << x << "x" << y;
          std::string str = "Generating " + ss.str();
          myEngine->renderer()->startWindow(1,1,myEngine->renderer()->width() - 2, 1);
          myEngine->renderer()->style(White, Black, Style::Bold);
          myEngine->renderer()->drawText(0,0, str + std::string(myEngine->renderer()->width() - 2 - str.size(),  ' '));
          myEngine->renderer()->refresh();
          myEngine->renderer()->endWindow();
          myGame->generator()->generateOne(x, y);
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

