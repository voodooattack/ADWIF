#ifndef MAPGENSTATE_H
#define MAPGENSTATE_H

#include "gamestate.hpp"
#include "mapgenerator.hpp"

#include <fstream>
#include <memory>
#include <random>

namespace ADWIF
{
  class MapGenState: public GameState
  {
  public:
    MapGenState(std::shared_ptr<class Engine> & engine, std::shared_ptr<class Game> & game);
    virtual ~MapGenState();

    virtual void init();
    virtual void step();
    virtual void consume(int key);
    virtual void activate();

  private:
    std::shared_ptr<class Engine> myEngine;
    std::shared_ptr<class Game> myGame;
    int myViewOffX, myViewOffY;
  };
}

#endif // MAPGENSTATE_H
