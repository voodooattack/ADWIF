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

#ifndef TCODRENDERER_H
#define TCODRENDERER_H

#include <libtcod.hpp>
#include <iostream>

#include "renderer.hpp"
#include "input.hpp"


#include <memory>
#include <boost/thread.hpp>

namespace ADWIF
{
  class TCODRenderer: public Renderer
  {
    friend class TCODInput;
  public:
    virtual bool init()
    {
      TCODConsole::initRoot (80, 48, "A Dance with Ice and Fire", false, TCOD_RENDERER_GLSL);
      TCODConsole::root->setDefaultBackground(TCODColor::black);
      TCODConsole::root->setDefaultForeground(TCODColor::silver);
      myConsole = TCODConsole::root;
      return true;
    }
    virtual void shutdown() { }
    virtual int width() const { return TCODConsole::root->getWidth(); }
    virtual int height() const { return TCODConsole::root->getHeight(); }
    virtual bool resized() const { return false; }
    virtual void clear() { myConsole->clear(); }
    virtual void refresh() { myConsole->flush(); }
    virtual void style(Colour fg, Colour bg, int styleMask)
    {
      myBackgroundColour = colourToTCOD(bg, true, styleMask);
      myForegroundColour = colourToTCOD(fg, false, styleMask);
      myStyle = styleMask;
    }
    virtual void style(int x, int y, int len, Colour fg, Colour bg, int styleMask)
    {
      myConsole->setCharBackground(x, y, colourToTCOD(bg, true, styleMask));
      myConsole->setCharForeground(x, y, colourToTCOD(fg, false, styleMask));
      myStyle = styleMask;
      if (styleMask & Style::AltCharSet)
      {
        for (int xx = x; xx < x + len; xx++)
        {
          int c = myConsole->getChar(xx, y);
          extChar(c);
          myConsole->setChar(xx, y, c);
        }
      }
    }
    virtual void startWindow(int x, int y, int w, int h) {
      myConsole = new TCODConsole(w, h);
      myConsole->setDefaultBackground(TCODColor::black);
      myConsole->setDefaultForeground(TCODColor::silver);
      myConX = x;
      myConY = y;
    }
    virtual void endWindow()
    {
      if (myConsole != TCODConsole::root)
      {
        TCODConsole::root->blit(myConsole, 0, 0, myConsole->getWidth(), myConsole->getHeight(), TCODConsole::root, myConX, myConY);
        delete myConsole;
        myConsole = TCODConsole::root;
      }
    }
    virtual void drawChar(int x, int y, int c) {
      if (myStyle & Style::AltCharSet)
        extChar(c);
      myConsole->putCharEx(x, y, c, myForegroundColour, myBackgroundColour);
    }
    virtual void drawText(int x, int y, const std::string & text)
    {
      TCODColor bg = myConsole->getDefaultBackground(), fg = myConsole->getDefaultForeground();
      myConsole->setDefaultBackground(myBackgroundColour);
      myConsole->setDefaultForeground(myForegroundColour);
      if (myStyle & Style::AltCharSet)
      {
        std::string ext = text;
        for(unsigned int i = 0; i < ext.size(); i++)
          extChar(ext[i]);
        myConsole->printRect(x, y, myConsole->getWidth(), myConsole->getHeight(), ext.c_str());
      }
      else
        myConsole->printRect(x, y, myConsole->getWidth(), myConsole->getHeight(), text.c_str());
      myConsole->setDefaultBackground(bg);
      myConsole->setDefaultForeground(fg);
    }
    virtual void drawEntity(const class Entity *, int x, int y) { }
    virtual void drawRegion(int x, int y, int z, int w, int h, int scrx, int scry, const class Game * game, const class Map *);

  private:
    TCODColor myBackgroundColour;
    TCODColor myForegroundColour;
    int myStyle;
    TCODConsole * myConsole;
    int myConX, myConY;

  private:
    template<typename T>
    void extChar(T & c)
    {
      switch (c)
      {
        case 'l': c = TCOD_CHAR_NW; break;
        case 'q': c = TCOD_CHAR_HLINE; break;
        case 'x': c = TCOD_CHAR_VLINE; break;
        case 'k': c = TCOD_CHAR_NE; break;
        case 'j': c = TCOD_CHAR_SE; break;
        case 'm': c = TCOD_CHAR_SW; break;
      }
    }

    TCODColor colourToTCOD(Colour c, bool bg, int styleMask)
    {
//       std::cout << std::boolalpha << bg << " " << styleMask << std::endl;
      if (!bg)
      {
        if (styleMask & Style::Bold)
        {
          switch (c)
          {
            case Default: return TCODColor::white;
            case Black: return TCODColor::darkestGrey;
            case Red: return TCODColor::red;
            case Green: return TCODColor::green;
            case Yellow: return TCODColor::yellow;
            case Blue: return TCODColor::blue;
            case Magenta: return TCODColor::magenta;
            case Cyan: return TCODColor::cyan;
            case White: return TCODColor::white;
            default: return TCODColor::black;
          }
        }
        else if (styleMask & Style::Dim)
        {
          switch (c)
          {
            case Default: return TCODColor::darkerGrey;
            case Black: return TCODColor::black;
            case Red: return TCODColor::darkerRed;
            case Green: return TCODColor::darkerGreen;
            case Yellow: return TCODColor::darkerYellow;
            case Blue: return TCODColor::darkerBlue;
            case Magenta: return TCODColor::darkerMagenta;
            case Cyan: return TCODColor::darkerCyan;
            case White: return TCODColor::grey;
            default: return TCODColor::black;
          }
        }
        else /*if (styleMask & Style::Normal)*/
        {
          switch (c)
          {
            case Default: return TCODColor::grey;
            case Black: return TCODColor::black;
            case Red: return TCODColor::darkRed;
            case Green: return TCODColor::darkGreen;
            case Yellow: return TCODColor::darkYellow;
            case Blue: return TCODColor::darkBlue;
            case Magenta: return TCODColor::darkMagenta;
            case Cyan: return TCODColor::darkCyan;
            case White: return TCODColor::silver;
            default: return TCODColor::black;
          }
        }
      }
      else
      {
        switch (c)
        {
          case Default: return TCODColor::white;
          case Black: return TCODColor::black;
          case Red: return TCODColor::red;
          case Green: return TCODColor::green;
          case Yellow: return TCODColor::yellow;
          case Blue: return TCODColor::blue;
          case Magenta: return TCODColor::magenta;
          case Cyan: return TCODColor::cyan;
          case White: return TCODColor::silver;
          default: return TCODColor::black;
        }
      }
    }
  };

  class TCODInput: public Input
  {
  public:
    TCODInput(const std::shared_ptr<class Renderer> & renderer):
      myRenderer(std::dynamic_pointer_cast<TCODRenderer>(renderer)), myTimeout(0) { }
    virtual bool init() {
//       TCODConsole::setKeyboardRepeat(0, 0);
      return true;
    }
    virtual void shutdown() { }
    virtual int key()
    {
      TCOD_key_t key;
      if (myTimeout == -1)
        TCODSystem::waitForEvent(TCOD_EVENT_KEY_PRESS, &key, NULL, true);
      else
      {
//         if (myTimeout)
//           boost::this_thread::sleep_for(boost::chrono::milliseconds(myTimeout));
        TCODSystem::checkForEvent(TCOD_EVENT_KEY_PRESS, &key, NULL);
      }
      if (key.vk == TCODK_SPACE)
        return ' ';
      if (key.vk != TCODK_NONE && key.vk != TCODK_CHAR)
        return (key.vk<<16);
      else if (key.c)
        return key.c;
      return 0;
    }
    virtual int getTimeout() const { return myTimeout; }
    virtual void setTimeout(int timeout) { myTimeout = timeout; }
    virtual std::string prompt(const std::string & text, unsigned int maxLen, const std::string & suffix = std::string());
    virtual bool promptYn(const std::string & text, bool caseSensetive);
  private:
    std::shared_ptr<TCODRenderer> myRenderer;
    int myTimeout;
  };
}

#endif // TCODRENDERER_H
