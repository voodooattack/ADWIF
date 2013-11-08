#include "mapeditorstate.hpp"
#include "game.hpp"
#include "fileutils.hpp"
#include "map.hpp"
#include "engine.hpp"

#include <fstream>

namespace ADWIF
{
  MapEditorState::MapEditorState(std::shared_ptr<ADWIF::Engine> & engine): myEngine(engine)
  {
    myGame.reset(new Game(engine));
    myGenerator.reset(new MapGenerator(myGame));
    myEngine->delay(200);
  }

  MapEditorState::~MapEditorState()
  {
  }

  void MapEditorState::init()
  {
    myGame->reloadData();
    myGame->createMap();
    myGenerator->init();
  }

  void MapEditorState::step()
  {

  }

  void MapEditorState::consume(int key)
  {

  }

  void MapEditorState::activate()
  {

  }
}

