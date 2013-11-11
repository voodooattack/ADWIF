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

#include "curses.hpp"
#include "adwif.hpp"
#include "util.hpp"
#include "map.hpp"
#include "game.hpp"

#include <ncursesw/curses.h>
#include <signal.h>
#include <memory.h>
#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>

namespace ADWIF
{

  void reset(int code)
  {
    endwin();
    initscr();
    nonl();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(FALSE);
    set_escdelay(10);
    start_color();
    use_default_colors();
    assume_default_colors(COLOR_WHITE, COLOR_BLACK);
    clear();
    refresh();
  }

  bool CursesRenderer::init()
  {
    signal(SIGWINCH, reset);

    reset(0);

    if (has_colors() == FALSE)
    {
      endwin();
      std::cout << "Your terminal does not support colour output." << std::endl;
      return false;
    }
    else
    {
      if (height() < 24 || width() < 80)
      {
        endwin();
        std::cout << "A minimum terminal size of 80x24 is required to play this game." << std::endl;
        return false;
      }
      else
      {
        myColourMap.resize(COLOR_PAIRS);
        return true;
      }
    }
    myWindow = nullptr;
  }

  void CursesRenderer::shutdown()
  {
    endWindow();
    endwin();
  }

  int CursesRenderer::width() const
  {
    return COLS;
  }

  int CursesRenderer::height() const
  {
    return LINES;
  }

  WINDOW * CursesRenderer::win() { return doClip ? myWindow : stdscr; }

  int CursesRenderer::getPair(Colour fg, Colour bg)
  {
    int pair = -1;

    for (int i = 0; i < COLOR_PAIRS; i++)
    {
      short c1, c2;
      pair_content(i, &c1, &c2);
      if (c1 == fg && c2 == bg)
      {
        pair = i;
        break;
      }
    }

    if (pair == -1)
    {
      for (unsigned int i = 1; i < myColourMap.size(); i++)
        if (!myColourMap[i])
        {
          pair = i;
          init_pair(pair, fg, bg);
          myColourMap[i] = true;
          break;
        }
    }

    if (pair == -1)
      pair = 0;

    return pair;
  }

  void CursesRenderer::style(Colour fg, Colour bg, int styleMask)
  {
    int pair = getPair(fg, bg);
    styleMask |= A_COLOR;
    wattr_set(win(), styleMask, pair, NULL);
  }

  void CursesRenderer::style(int x, int y, int len, Colour fg, Colour bg, int styleMask)
  {
    int pair = getPair(fg, bg);
    styleMask |= A_COLOR;
    mvwchgat(win(), y, x, len, styleMask, pair, NULL);
  }

  void CursesRenderer::startWindow(int x, int y, int w, int h)
  {
    if (myWindow)
      endWindow();
    myWindow = newwin(h, w, y, x);
    doClip = true;
  }

  void CursesRenderer::endWindow()
  {
    if (myWindow)
    {
      wrefresh(myWindow);
      delwin(myWindow);
    }
    myWindow = nullptr;
    doClip = false;
  }

  void CursesRenderer::drawChar(int x, int y, int c)
  {
    mvwaddnwstr(win(), y, x, (const wchar_t*) &c, 1);
    //mvwaddchnstr(win(), y, x, (chtype*) &c, sizeof(c));
/*    attr_t a;
    short pair;
    wattr_get(win(), &a, &pair, 0);
    mvwchgat(win(), y, x, 1, getattrs(win()), pair, NULL);*/
  }

  void CursesRenderer::drawText(int x, int y, const std::string & text)
  {
    mvwaddnstr(win(), y, x, text.c_str(), text.size());
  }

  void CursesRenderer::drawRegion(int x, int y, int z, int w, int h, int scrx, int scry, const Game * game, const Map * map)
  {
    for(int yy = 0; yy < h; yy++)
      for(int xx = 0; xx < w; xx++)
      {
        const MapCell & c = map->get(x + xx, y + yy, z);
        const Material * mat = nullptr;
        if (c.structure == Structure::None)
        {
          auto mit = game->materials().find(c.material);
          if (mit == game->materials().end())
            throw std::runtime_error("material '" + c.material + "' undefined");
          mat = mit->second;
          auto dit = mat->disp.find(c.type);
          if (dit == mat->disp.end())
            throw std::runtime_error("terrain type '" + terrainTypeStr(c.type) + "' undefined in material '" + mat->name + "'");
          const Material::dispEntry & disp = dit->second[c.symIdx < dit->second.size() ? c.symIdx : dit->second.size() - 1];
          style(disp.style.fg, disp.style.bg, disp.style.style);
          drawChar(scrx + xx, scry + yy, disp.sym);
        }
      }
  }

  void CursesRenderer::clear()
  {
    wclear(win());
    myColourMap.assign(COLOR_PAIRS, false);
  }

  void CursesRenderer::refresh() { wrefresh(win()); }

  bool CursesRenderer::resized() const
  {
    bool r = myResizedFlag;
    if (r) myResizedFlag = false;
    return r;
  }

  int CursesInput::key()
  {
    int ch = getch();
    myRenderer->myResizedFlag = ch == KEY_RESIZE;
    return ch == KEY_MOUSE || ch == KEY_RESIZE ? -1 : ch;
  }

  std::string CursesInput::prompt(const std::string & text, unsigned int maxLen, const std::string & suffix)
  {
    std::vector<std::string> msgBuf = split(text, '\n');
    unsigned int width = maxLen + (suffix.empty() ? 0 : suffix.size() + 2);
    for(auto & s : msgBuf)
      width = (s.size() > width ? s.size() : width);
    width = (width > maxLen ? width : maxLen) + 4;
    int msgHeight = msgBuf.size();
    int height = 3 + msgHeight;
    char result[maxLen+1];
    std::memset(result, 0, maxLen + 1);
    myRenderer->startWindow(myRenderer->width() / 2 - width / 2,
                            myRenderer->height() / 2 - height / 2, width, height);
    echo();
    cbreak();
    curs_set(1);
    timeout(-1);
    wborder(myRenderer->win(), 0,0,0,0,0,0,0,0);
    int yoff = 1;
    for(auto & s : msgBuf)
      myRenderer->drawText(2, yoff++, s);
    if (!suffix.empty())
      myRenderer->drawText(maxLen + 4, msgHeight+1, suffix);
    myRenderer->refresh();
    wmove(myRenderer->win(), msgHeight+1, 2);
    wgetnstr(myRenderer->win(), result, maxLen);
    reset(0);
    timeout(0);
    myRenderer->endWindow();
    return result;
  }

  bool CursesInput::promptYn(const std::string & text, bool caseSensetive)
  {
    int width = text.size() + 4;
    int height = 3;
    bool result = false;
    myRenderer->startWindow(myRenderer->width() / 2 - width / 2,
                            myRenderer->height() / 2 - height / 2, width, height);
    wborder(myRenderer->win(), 0,0,0,0,0,0,0,0);
    myRenderer->drawText(2, 1, text);
    timeout(-1);
    myRenderer->refresh();
    while (1)
    {
      int c = getch();
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
    timeout(0);
    myRenderer->endWindow();
    return result;
  }

  int CursesInput::getTimeout() const
  {
    return myTimeout;
  }

  void CursesInput::setTimeout(int timeout)
  {
    ::timeout(timeout);
    myTimeout = timeout;
  }


  namespace Style
  {
    extern const int Normal = A_COLOR;
    extern const int Bold = A_BOLD;
    extern const int Underline = A_UNDERLINE;
    extern const int Dim = A_DIM;
    extern const int StandOut = A_STANDOUT;
    extern const int AltCharSet = A_ALTCHARSET;
  };

  namespace Key
  {
    extern const int Escape = 27;
    extern const int Break = KEY_BREAK;
    extern const int Backspace = KEY_BACKSPACE;
    extern const int Enter = KEY_ENTER;
    extern const int Up = KEY_UP;
    extern const int Down = KEY_DOWN;
    extern const int Left = KEY_LEFT;
    extern const int Right = KEY_RIGHT;
    extern const int Num7 = KEY_A1;
    extern const int Num8 = '8';
    extern const int Num9 = KEY_A3;
    extern const int Num4 = '4';
    extern const int Num5 = KEY_B2;
    extern const int Num6 = '6';
    extern const int Num1 = KEY_C1;
    extern const int Num2 = '2';
    extern const int Num3 = KEY_C3;
    extern const int Num0 = KEY_LL;
    extern const int Insert = KEY_IL;
    extern const int Home = KEY_HOME;
    extern const int PgUp = KEY_PPAGE;
    extern const int Del = KEY_DL;
    extern const int End = KEY_END;
    extern const int PgDn = KEY_NPAGE;
    extern const int F1 = KEY_F(1);
    extern const int F2 = KEY_F(2);
    extern const int F3 = KEY_F(3);
    extern const int F4 = KEY_F(4);
    extern const int F5 = KEY_F(5);
    extern const int F6 = KEY_F(6);
    extern const int F7 = KEY_F(7);
    extern const int F8 = KEY_F(8);
    extern const int F9 = KEY_F(9);
    extern const int F10 = KEY_F(10);
    extern const int F11 = KEY_F(11);
    extern const int F12 = KEY_F(12);
  }
}
