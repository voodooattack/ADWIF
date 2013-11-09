#ifndef MAINMENUSTATE_HPP
#define MAINMENUSTATE_HPP
#include "gamestate.hpp"
#include <memory>

namespace ADWIF
{
  class MainMenuState: public GameState
  {
  public:
    MainMenuState(std::shared_ptr<class Engine> & engine) ;
    ~MainMenuState();

    virtual void init();
    virtual void step();
    virtual void consume(int key);
    virtual void activate();

  private:
    void drawMenu();
  private:
    std::shared_ptr<class Engine> myEngine;
    class Animation * myLogo, * myMenu;
  };
}

#endif // MAINMENUSTATE_HPP
