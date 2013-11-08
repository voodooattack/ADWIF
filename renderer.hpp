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
