#ifndef INTROSTATE_HPP
#define INTROSTATE_HPP

#include "gamestate.hpp"

#include <memory>

namespace ADWIF
{
  class IntroState: public GameState
  {
  public:
    IntroState(std::shared_ptr<class Engine> & engine);
    virtual ~IntroState() ;
    virtual void init();
    virtual void step();
    virtual void consume(int key);
  private:
    std::shared_ptr<class Engine> myEngine;
    class Animation * myLogo, * myCopyrightText;
  };
}

#endif // INTROSTATE_H
