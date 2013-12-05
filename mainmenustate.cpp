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
#include "mainmenustate.hpp"
#include "renderer.hpp"
#include "input.hpp"
#include "animation.hpp"
#include "introanimation.hpp"
#include "introstate.hpp"
#include "newgamestate.hpp"
#include "animationutils.hpp"
#include "game.hpp"

namespace ADWIF
{
  MainMenuState::MainMenuState(const std::shared_ptr<ADWIF::Engine> & engine): myEngine(engine)
  {
    myLogo = new Animation(myEngine->renderer());
    myMenu = new Animation(myEngine->renderer());
    myLogo->loadAnimation(introAnimation);
    std::vector<std::string> menuEntries = { "New Adventure", "Continue", "About", "Quit" };
    std::vector<AnimationFrame> menuFrames;
    generateMenu(menuFrames, menuEntries, introPalette, 8, 1, true);
    myMenu->loadAnimation(menuFrames);
    myMenu->advance(false);
    myEngine->delay(50);
  }

  MainMenuState::~MainMenuState()
  {
    delete myLogo;
    delete myMenu;
  }

  void MainMenuState::init()
  {
    done(false);
    //myEngine->delay(200);
    drawMenu();
  }

  void MainMenuState::step()
  {
    drawMenu();
  }

  void MainMenuState::consume(int key)
  {
    if (key == Key::Escape)
      done(myEngine->input()->promptYn("Really quit? (Y/N)", false));
    else if (key == Key::Up || key == Key::Num8)
      myMenu->prev();
    else if (key == Key::Down || key == Key::Num2)
      myMenu->next();
    else if (key == Key::Enter || key == '\n' || key == '\r')
    {
      switch (myMenu->seek())
      {
      case 0: // New Adventure
        {
          auto state = std::shared_ptr<GameState>(new NewGameState(myEngine));
          myEngine->addState(state);
          break;
        }
      case 1: break; // Continue (TODO)
      case 2: // About
        {
          done(true);
          auto state = std::shared_ptr<GameState>(new IntroState(myEngine));
          myEngine->addState(state);
          break;
        }
      case 3:        // Quit
        {
          done(myEngine->input()->promptYn("Really quit? (Y/N)", false));
        }
      }
    }
  }

  void MainMenuState::drawMenu()
  {
    int yoffset = 4;
    myLogo->render(myEngine->renderer()->width() / 2 - myLogo->width() / 2, yoffset, myEngine->delay() / 1000.0);
    myMenu->render(myEngine->renderer()->width() / 2 - myMenu->width() - 2, yoffset + myLogo->height() - 3, myEngine->delay() / 1000.0);
  }

  void MainMenuState::activate() { myEngine->renderer()->clear(); }
}

