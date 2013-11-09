#ifndef MAPEDITORSTATE_H
#define MAPEDITORSTATE_H

#include "gamestate.hpp"
#include "mapgenerator.hpp"

#include <fstream>

namespace ADWIF
{
  class MapEditorState: public GameState
  {
  public:
    MapEditorState(std::shared_ptr<class Engine> & engine);
    virtual ~MapEditorState();

    virtual void init();
    virtual void step();
    virtual void consume(int key);
    virtual void activate();

  private:
    std::shared_ptr<class Engine> myEngine;
    std::shared_ptr<class Game> myGame;
    std::shared_ptr<class MapGenerator> myGenerator;
  };
}

#endif // MAPEDITORSTATE_H
