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

#ifndef INPUT_HPP
#define INPUT_HPP

#include <string>

namespace ADWIF
{
  class Input
  {
  public:
    virtual ~Input() { }
    virtual bool init() = 0;
    virtual void shutdown() = 0;
    virtual int key() = 0;
    virtual int getTimeout() const = 0;
    virtual void setTimeout(int timeout) = 0;
    virtual std::string prompt(const std::string & text, unsigned int maxLen, const std::string & suffix = std::string()) = 0;
    virtual bool promptYn(const std::string & text, bool caseSensetive) = 0;
  };

  namespace Key
  {
    extern const int Escape;
    extern const int Break;
    extern const int Backspace;
    extern const int Enter;
    extern const int Up;
    extern const int Down;
    extern const int Left;
    extern const int Right;
    extern const int Num7;
    extern const int Num8;
    extern const int Num9;
    extern const int Num4;
    extern const int Num5;
    extern const int Num6;
    extern const int Num1;
    extern const int Num2;
    extern const int Num3;
    extern const int Num0;
    extern const int Insert;
    extern const int Home;
    extern const int PgUp;
    extern const int Del;
    extern const int End;
    extern const int PgDn;
    extern const int F1;
    extern const int F2;
    extern const int F3;
    extern const int F4;
    extern const int F5;
    extern const int F6;
    extern const int F7;
    extern const int F8;
    extern const int F9;
    extern const int F10;
    extern const int F11;
    extern const int F12;
  }
}

#endif // INPUT_HPP
