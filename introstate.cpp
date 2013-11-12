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
  IntroState::IntroState(const std::shared_ptr<Engine> & engine): myEngine(engine)
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
