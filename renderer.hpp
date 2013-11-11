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

#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <string>
#include <vector>

namespace ADWIF
{
  enum Colour : int32_t
  {
    Default = -1,
    Black,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White,
  };

  std::string colourStr(Colour col);
  Colour strColour(const std::string & colour);

  std::vector<std::string> styleStrs(int style);
  int strsStyle(const std::vector<std::string> & strs);

  namespace Style
  {
    extern const int Normal;
    extern const int Bold;
    extern const int Underline;
    extern const int Dim;
    extern const int StandOut;
    extern const int AltCharSet;
  };

  class Renderer
  {
  public:
    virtual ~Renderer() { }
    virtual bool init() = 0;
    virtual void shutdown() = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual bool resized() const = 0;
    virtual void clear() = 0;
    virtual void refresh() = 0;
    virtual void style(Colour fg, Colour bg, int styleMask) = 0;
    virtual void style(int x, int y, int len, Colour fg, Colour bg, int styleMask) = 0;
    virtual void startWindow(int x, int y, int w, int h) = 0;
    virtual void endWindow() = 0;
    virtual void drawChar(int x, int y, int c) = 0;
    virtual void drawText(int x, int y, const std::string & text) = 0;
    virtual void drawEntity(const class Entity *, int x, int y) = 0;
    virtual void drawRegion(int x, int y, int z, int w, int h, int scrx, int scry, const class Game * game, const class Map * map) = 0;
  };
}

#endif
