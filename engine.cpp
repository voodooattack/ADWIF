#include "engine.hpp"
#include "renderer.hpp"
#include "input.hpp"
#include "gamestate.hpp"

#include <unistd.h>
#include <iostream>
#include <thread>

namespace ADWIF
{
  Engine::Engine(std::shared_ptr<ADWIF::Renderer> renderer, std::shared_ptr<ADWIF::Input> input):
    myRenderer(renderer), myInput(input), myDelay(50), myRunningFlag(true)
  {
  }

  Engine::~Engine()
  {
    while (myStates.size())
      myStates.pop_back();
    if (myInput)
      myInput->shutdown();
    if (myRenderer)
      myRenderer->shutdown();
  }

  int Engine::start()
  {
    renderer()->clear();
    bool screenCheckLast = false;
    while (myStates.size())
    {
      bool screenCheck = checkScreenSize();
      if (screenCheck)
      {
        if (myStates.back()->done())
        {
          myStates.pop_back();
          if (!myStates.empty())
            myStates.back()->activate();
          else
            break;
        }
        else
        {
          int ch = myInput->key();
          if (ch != -1)
            myStates.back()->consume(ch);
          // superfluous checks for this condition,
          // reportError() could be called at any point
          if (!myRunningFlag) break;
          if (screenCheckLast == false)
          {
            myStates.back()->resize();
            if (!myRunningFlag) break;
            myStates.back()->activate();
          }
          if (!myRunningFlag) break;
          myStates.back()->step();
          renderer()->refresh();
          sleep(delay());
        }
      }
      else
      {
        static const std::string message = "A minimum terminal size of 80x24 is required to play this game";
        renderer()->clear();
        renderer()->style(White, Black, Style::Bold);
        renderer()->drawText(1, 1, message);
        renderer()->refresh();
        sleep(1);
      }
      screenCheckLast = screenCheck;
    }
    return 0;
  }

  void Engine::addState(std::shared_ptr<GameState> & state)
  {
    if (!myRunningFlag) return;
    myStates.push_back(state);
    renderer()->clear();
    state->init();
    if (!myRunningFlag) return;
    state->activate();
  }

  bool Engine::checkScreenSize()
  {
    if (renderer()->height() < 24 || renderer()->width() < 80)
      return false;
    else
      return true;
  }

  void Engine::reportError(bool fatal, const std::string & report)
  {
    if (fatal)
    {
      myRunningFlag = false;
      input()->shutdown();
      renderer()->clear();
      renderer()->shutdown();
      myInput.reset();
      myRenderer.reset();
      std::cerr << "FATAL: ";
    }
    else
      std::cerr << "ERROR: ";
    std::cerr << report << std::endl;
  }

  void Engine::sleep(unsigned int ms)  { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

  void Engine::delay(unsigned int delay)
  {
    myDelay = delay;
    //myInput->setTimeout(delay);
  }

  unsigned int Engine::delay() const
  {
    return myDelay;
    //return myInput->getTimeout();

  }
}
