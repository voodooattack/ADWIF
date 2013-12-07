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

#include "tcod.hpp"
#include "game.hpp"
#include "map.hpp"

#include "util.hpp"

#include <boost/algorithm/string.hpp>

namespace ADWIF
{
  void TCODRenderer::drawMessage(const std::string & message)
  {
    std::string text = wrapText(message, this->width() / 2);
    std::vector<std::string> msgBuf;
    boost::split(msgBuf, text, boost::is_any_of("\n"));
    unsigned int width = 0;
    for (auto & s : msgBuf)
      width = (s.size() > width ? s.size() : width);
    width += 2;
    int msgHeight = msgBuf.size();
    int height = 2 + msgHeight;

    startWindow(this->width() / 2 - width / 2,
                this->height() / 2 - height / 2, width, height);
    style(Colour::White, Colour::Black, Style::Normal);
    myConsole->printFrame(0, 0, width, height, true);
    myConsole->print(1, 1, text.c_str());
    endWindow();
  }

  std::string TCODInput::prompt(const std::string & text, unsigned int maxLen, const std::string & suffix)
  {
    std::vector<std::string> msgBuf = split(text, '\n');
    unsigned int width = maxLen + (suffix.empty() ? 0 : suffix.size() + 2);
    for (auto & s : msgBuf)
      width = (s.size() > width ? s.size() : width);
    width = (width > maxLen ? width : maxLen) + 4;
    int msgHeight = msgBuf.size();
    int height = 3 + msgHeight;
    std::string result;
    setTimeout(-1);
    while(1)
    {
      myRenderer->startWindow(myRenderer->width() / 2 - width / 2,
                              myRenderer->height() / 2 - height / 2, width, height);
      myRenderer->style(Colour::White, Colour::Black, Style::Normal);
      myRenderer->myConsole->printFrame(0, 0, myRenderer->myConsole->getWidth(), myRenderer->myConsole->getHeight(), true);
      int yoff = 1;
      for (auto & s : msgBuf)
        myRenderer->drawText(2, yoff++, s);
      myRenderer->drawText(2, msgHeight + 1, result);
      if (!suffix.empty())
        myRenderer->drawText(result.size() + 4, msgHeight + 1, suffix);
      myRenderer->style(Colour::White, Colour::Black, Style::Bold);
      myRenderer->drawChar(result.size() + 2, msgHeight + 1, '_');
      myRenderer->endWindow();
      myRenderer->refresh();

      int key = this->key();
      if (key == Key::Enter)
        break;
      else if (key == Key::Escape)
      {
        result = "";
        break;
      }
      else if (key == Key::Backspace && !result.empty())
        result.resize(result.size()-1);
      else if (key >= ' ' && key <= '~' && result.size() < maxLen)
        result += (char)key;
    }
    setTimeout(0);
    return result;
  }

  bool TCODInput::promptYn(const std::string & text, bool caseSensetive)
  {

    int width = text.size() + 4;
    int height = 3;
    bool result = false;
    while (1)
    {
      myRenderer->startWindow(myRenderer->width() / 2 - width / 2,
                              myRenderer->height() / 2 - height / 2, width, height);
      myRenderer->style(Colour::White, Colour::Black, Style::Normal);
      myRenderer->myConsole->printFrame(0, 0, myRenderer->myConsole->getWidth(), myRenderer->myConsole->getHeight(), true);
      myRenderer->drawText(2, 1, text);
      setTimeout(-1);
      myRenderer->endWindow();
      myRenderer->refresh();
      int c = key();
      if (c == 'Y' || (!caseSensetive && c == 'y'))
      {
        result = true;
        break;
      }
      else if (c == 'N' || c == Key::Escape || (!caseSensetive && c == 'n'))
      {
        result = false;
        break;
      }
    }
    setTimeout(0);
    return result;
  }

  namespace Style
  {
    extern const int Normal = 0;
    extern const int Bold = 1;
    extern const int Underline = 2;
    extern const int Dim = 4;
    extern const int Dark = 8;
    extern const int StandOut = 16;
    extern const int AltCharSet = 32;
  };

  namespace Key
  {
    extern const int Escape = (TCODK_ESCAPE<<16);
    extern const int Break = (TCODK_PAUSE<<16);
    extern const int Backspace = (TCODK_BACKSPACE<<16);
    extern const int Enter = (TCODK_ENTER<<16);
    extern const int Up = (TCODK_UP<<16);
    extern const int Down = (TCODK_DOWN<<16);
    extern const int Left = (TCODK_LEFT<<16);
    extern const int Right = (TCODK_RIGHT<<16);
    extern const int Num7 = (TCODK_KP7<<16);
    extern const int Num8 = (TCODK_KP8<<16);
    extern const int Num9 = (TCODK_KP9<<16);
    extern const int Num4 = (TCODK_KP4<<16);
    extern const int Num5 = (TCODK_KP5<<16);
    extern const int Num6 = (TCODK_KP6<<16);
    extern const int Num1 = (TCODK_KP1<<16);
    extern const int Num2 = (TCODK_KP2<<16);
    extern const int Num3 = (TCODK_KP3<<16);
    extern const int Num0 = (TCODK_KP0<<16);
    extern const int Insert = (TCODK_INSERT<<16);
    extern const int Home = (TCODK_HOME<<16);
    extern const int PgUp = (TCODK_PAGEUP<<16);
    extern const int Del = (TCODK_DELETE<<16);
    extern const int End = (TCODK_END<<16);
    extern const int PgDn = (TCODK_PAGEDOWN<<16);
    extern const int F1 = (TCODK_F1<<16);
    extern const int F2 = (TCODK_F2<<16);
    extern const int F3 = (TCODK_F3<<16);
    extern const int F4 = (TCODK_F4<<16);
    extern const int F5 = (TCODK_F5<<16);
    extern const int F6 = (TCODK_F6<<16);
    extern const int F7 = (TCODK_F7<<16);
    extern const int F8 = (TCODK_F8<<16);
    extern const int F9 = (TCODK_F9<<16);
    extern const int F10 = (TCODK_F10<<16);
    extern const int F11 = (TCODK_F11<<16);
    extern const int F12 = (TCODK_F12<<16);
  }
}
