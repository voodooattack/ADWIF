#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "introstate.hpp"
#include "introanimation.hpp"
#include "renderer.hpp"
#include "mainmenustate.hpp"
#include "engine.hpp"
#include "animation.hpp"
#include "input.hpp"

namespace ADWIF
{
  IntroState::IntroState(std::shared_ptr<Engine> & engine): myEngine(engine)
  {
    myLogo = new Animation(myEngine->renderer());
    myCopyrightText = new Animation(myEngine->renderer());
    myLogo->loadAnimation(introAnimation);
    myCopyrightText->addFrame(copyrightText);
    myEngine->delay(50);
  }

  IntroState::~IntroState() { delete myLogo; delete myCopyrightText; }

  void IntroState::init()
  {
    done(false);
    //myEngine->delay(200);
  }

  void IntroState::step()
  {
    const int yoffset = 1;
    myCopyrightText->render(myEngine->renderer()->width() / 2 - myCopyrightText->width() / 2,
                            yoffset + myLogo->height(), myEngine->delay() / 1000.0);
    myLogo->render(myEngine->renderer()->width() / 2 - myLogo->width() / 2, yoffset, myEngine->delay() / 1000.0);
  }

  void IntroState::consume(int key)
  {
    if (key == '\n' || key == '\r' || key == ' ')
    {
      auto state = std::shared_ptr<GameState>(new MainMenuState(myEngine));
      myEngine->addState(state);
      done(true);
    }
  }
}
