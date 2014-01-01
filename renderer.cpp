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

#include "renderer.hpp"
#include "game.hpp"
#include "map.hpp"

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
    if (style & Style::Dark)
      strs.push_back("Dark");
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
      else if (s == "dark")
        style |= Style::Dark;
      else if (s == "standout")
        style |= Style::StandOut;
      else if (s == "altcharset")
        style |= Style::AltCharSet;
      else
        style |= Style::Normal;
    }

    return style;
  }

  void Renderer::drawRegion(int x, int y, int z, int w, int h, int scrx, int scry, const Game * game, Map * map)
  {
    auto drawCell = [&](const MapCell & c, int x, int y, int overrideStyle)
    {
      if (c.elements().size() == 0)
      {
        style(Colour::Cyan, Colour::Cyan, overrideStyle == -1 ? Style::Normal : overrideStyle);
        drawChar(x, y, ' ');
        return;
      }

      Element * e = nullptr;

      for(Element * el : c.elements())
      {
        if (!e || (!el->isMaterial() && !el->isStructure() && (e->isMaterial() || e->isStructure())))
          e = el;
        else if (!e || (el->isStructure() && e->isMaterial()))
          e = el;
        else if (!e || (el->isMaterial() && e->isMaterial() && el->volume() > e->volume()))
          e = el;
        else if (!e || (el->isMaterial() && e->isMaterial() && el->isAnchored() == true && e->isAnchored() == false))
          e = el;
        else if (!e || ((!el->isMaterial() && !el->isStructure() && !e->isMaterial() && !e->isStructure()) && el->volume() > e->volume()))
          e = el;
      }

      if (e)
      {
        if (e->isMaterial())
        {
          const MaterialElement * me = dynamic_cast<MaterialElement*>(e);
          const Material * mat = me->cmaterial;
          if (!mat)
          {
            auto mit = game->materials().find(me->material);
            if (mit == game->materials().end())
              throw std::runtime_error("material '" + me->material + "' undefined");
            mat = mit->second;
          }
          TerrainType type = TerrainType::Hole;
          if (me->anchored && me->materialState() == MaterialState::Solid)
          {
            if (me->vol == MapCell::MaxVolume)
              type = TerrainType::Wall;
            else if (me->vol >= 500000000)
            {
              if (me->alignment)
                type = TerrainType::RampD;
              else
                type = TerrainType::RampU;
            }
            else if (me->vol < 500000000)
              type = TerrainType::Floor;
          }

          if (type != TerrainType::Hole)
          {
            auto dit = mat->disp.find(type);
            if (dit == mat->disp.end())
              throw std::runtime_error("terrain type '" + terrainTypeStr(type) + "' undefined in material '" + mat->name + "'");
            const Material::dispEntry & disp = dit->second[me->symIdx < dit->second.size() ? me->symIdx : dit->second.size() - 1];
            style(disp.style.fg, disp.style.bg, overrideStyle == -1 ? disp.style.style : overrideStyle);
            drawChar(x, y, disp.sym);
          }
          else
          {
            style(Colour::Cyan, Colour::Cyan, overrideStyle == -1 ? Style::Normal : overrideStyle);
            drawChar(x, y, ' ');
          }
        }
      }


//       if (c.structure == Structure::None)
//       {
//         const Material * mat = c.cmaterial;
//         if (!mat)
//         {
//           auto mit = game->materials().find(c.material);
//           if (mit == game->materials().end())
//             throw std::runtime_error("material '" + c.material + "' undefined");
//           mat = mit->second;
//         }
//         auto dit = mat->disp.find(c.type);
//         if (dit == mat->disp.end())
//           throw std::runtime_error("terrain type '" + terrainTypeStr(c.type) + "' undefined in material '" + mat->name + "'");
//         const Material::dispEntry & disp = dit->second[c.symIdx < dit->second.size() ? c.symIdx : dit->second.size() - 1];
//         style(disp.style.fg, disp.style.bg, overrideStyle == -1 ? disp.style.style : overrideStyle);
//         drawChar(x, y, disp.sym);
//       }
    };

    for (int yy = 0; yy < h; yy++)
    {
      for (int xx = 0; xx < w; xx++)
      {
        const MapCell & c = map->get(x + xx, y + yy, z);
        if (!c.seen() && c.free() == 0)
        {
          style(Colour::Black, Colour::Black, Style::Normal);
          drawChar(scrx + xx, scry + yy, ' ');
        }
        else if (c.used() == 0)
        {
          if (!supportsMultiLayers())
          {
            style(Colour::Cyan, Colour::Cyan, Style::Dim);
            drawChar(scrx + xx, scry + yy, ' ');
          }
          else
          {
            const MapCell & cc = map->get(x + xx, y + yy, z-1);
            if (!cc.seen() && cc.free() == 0)
            {
              style(Colour::Black, Colour::Black, Style::Normal);
              drawChar(scrx + xx, scry + yy, ' ');
            }
            else if (cc.used() == 0)
            {
              style(Colour::Cyan, Colour::Cyan, Style::Dim);
              drawChar(scrx + xx, scry + yy, ' ');
            }
            else
              drawCell(cc, scrx + xx, scry + yy, Style::Dark);
          }
        }
        else
          drawCell(c, scrx + xx, scry + yy, -1);
      }
    }
  }

}
