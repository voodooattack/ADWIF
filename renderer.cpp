#include "renderer.hpp"

#include <string>
#include <vector>
#include <algorithm>
#include <locale>

namespace ADWIF
{
  std::string colourStr(Colour col)
  {
    switch(col)
    {
      case Colour::Black: return "Black";
      case Colour::Blue: return "Blue";
      case Colour::Cyan: return "Cyan";
      case Colour::Green: return "Green";
      case Colour::Magenta: return "Magenta";
      case Colour::Red: return "Red";
      case Colour::White: return "White";
      case Colour::Yellow: return "Yellow";
      case Colour::Default: return "Default";
    }
    return "Default";
  }

  Colour strColour(const std::string & colour)
  {
    std::string c;
    std::transform(colour.begin(), colour.end(), std::back_inserter(c), &tolower);
    if (c == "black")
      return Colour::Black;
    else if (c == "blue")
      return Colour::Blue;
    else if (c == "cyan")
      return Colour::Cyan;
    else if (c == "green")
      return Colour::Green;
    else if (c == "magenta")
      return Colour::Magenta;
    else if (c == "red")
      return Colour::Red;
    else if (c == "white")
      return Colour::White;
    else if (c == "yellow")
      return Colour::Yellow;
    else
      return Colour::Default;
  }

  std::vector<std::string> styleStrs(int style)
  {
    std::vector<std::string> strs;
    if (style & Style::Bold)
      strs.push_back("Bold");
    if (style & Style::Underline)
      strs.push_back("Underline");
    if (style & Style::Dim)
      strs.push_back("Dim");
    if (style & Style::StandOut)
      strs.push_back("StandOut");
    if (style & Style::AltCharSet)
      strs.push_back("AltCharSet");
    if (!style || style & Style::Normal)
      strs.push_back("Normal");
    return strs;
  }

  int strsStyle(const std::vector<std::string> & strs)
  {
    int style = 0;
    for (auto s : strs)
    {
      std::transform(s.begin(), s.end(), s.begin(), &tolower);

      if (s == "bold")
        style |= Style::Bold;
      else if (s == "underline")
        style |= Style::Underline;
      else if (s == "dim")
        style |= Style::Dim;
      else if (s == "standout")
        style |= Style::StandOut;
      else if (s == "altcharset")
        style |= Style::AltCharSet;
      else
        style |= Style::Normal;
    }

    return style;
  }

}
