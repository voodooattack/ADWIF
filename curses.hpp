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
    virtual void drawText(int x, int y, const std::string & text) ;
    virtual void drawEntity(const class Entity *, int x, int y) { }
    virtual void drawRegion(int x, int y, int z, int w, int h, int scrx, int scry, const class Game * game, const class Map *);

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
    CursesInput(std::shared_ptr<class Renderer> & renderer): myRenderer(std::dynamic_pointer_cast<CursesRenderer>(renderer)) { }
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
