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

#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <vector>
#include <memory>

namespace ADWIF
{
  class Engine
  {
  public:
    Engine(std::shared_ptr<class Renderer> renderer, std::shared_ptr<class Input> input);
    ~Engine();

    std::shared_ptr<class Renderer> renderer() const { return myRenderer; }
    std::shared_ptr<class Input> input() const { return myInput; }

    unsigned int delay() const;
    void delay(unsigned int delay);

    bool running() const { return myRunningFlag; }
    void running(bool r) { myRunningFlag = r; }

    int start();

    void sleep(unsigned ms);

    void addState(std::shared_ptr<class GameState> & state);
    void reportError(bool fatal, const std::string & report) ;

  private:
    bool checkScreenSize();
  private:
    std::shared_ptr<class Renderer> myRenderer;
    std::shared_ptr<class Input> myInput;
    std::vector <
      std::shared_ptr<class GameState>
    > myStates;
    unsigned int myDelay;
    bool myRunningFlag;
  };
}

#endif // ENGINE_HPP
