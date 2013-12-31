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

#ifndef CURSES_HPP
#define CURSES_HPP

#include "renderer.hpp"
#include "input.hpp"

#include <string>
#include <vector>
#include <memory>

#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif

#include <ncursesw/curses.h>

namespace ADWIF
{
  class CursesRenderer: public Renderer
  {
    friend class CursesInput;
  public:
    virtual const std::string name() const { return "curses"; }
    virtual bool init();
    virtual void shutdown() ;
    virtual int width() const;
    virtual int height() const;
    virtual bool resized() const;
    virtual void clear();
    virtual void refresh();
    virtual void style(Colour fg, Colour bg, int styleMask);
    virtual void style(int x, int y, int len, Colour fg, Colour bg, int styleMask);
    virtual void startWindow(int x, int y, int w, int h);
    virtual void endWindow();
    virtual void drawChar(int x, int y, int c);
    virtual void drawText(int x, int y, const std::string & text);
    virtual void drawMessage(const std::string & message);
    virtual void drawEntity(const class Entity *, int x, int y) { }

    virtual bool supportsMultiLayers() const { return false; }

  private:
    WINDOW * win();
    int getPair(Colour fg, Colour bg);
  private:
    std::vector<bool> myColourMap;
    mutable bool myResizedFlag;
    WINDOW * myWindow;
    bool doClip;
  };

  class CursesInput: public Input
  {
  public:
    CursesInput(const std::shared_ptr<class Renderer> & renderer): myRenderer(std::dynamic_pointer_cast<CursesRenderer>(renderer)) { }
    virtual bool init() { return true; }
    virtual void shutdown() { }
    virtual int key();
    virtual int getTimeout() const;
    virtual void setTimeout(int timeout);
    virtual std::string prompt(const std::string & text, unsigned int maxLen, const std::string & suffix = std::string());
    virtual bool promptYn(const std::string & text, bool caseSensetive);
  private:
    std::shared_ptr<CursesRenderer> myRenderer;
    int myTimeout;
  };
}
#endif // CURSES_HPP
