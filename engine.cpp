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
    myRenderer(renderer), myInput(input), myDelay(50), myRunningFlag(true), myScheduler(new boost::cgl::scheduler)
  {
    myScheduler->run();
  }

  Engine::~Engine()
  {
    while (myStates.size())
      myStates.pop_back();
    myScheduler->stop();
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
          myStates.back()->exit();
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
