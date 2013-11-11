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

#include "animationutils.hpp"

#include <cstdlib>
#include <cstring>
#include <cstdio>

namespace ADWIF
{
  void generateMenu(std::vector<AnimationFrame> & menu,
                    const std::vector< std::string > & entries,
                    const std::vector<palEntry> & palette,
                    int active,
                    int dormant,
                    bool alignRight,
                    unsigned int maxHeight,
                    const std::vector<void*> tags,
                    const std::vector<bool> & enabled,
                    int disabled,
                    int disabledHighlight,
                    int charUp,
                    int charDn,
                    int arrowP
                   )
  {
    unsigned int width = 0, height = entries.size();
    AnimationFrame frameTemplate;

    char sactive[3], sdormant[3], sdisabled[3], shdisabled[3], sarow[3];
    std::snprintf(sactive, sizeof(sactive) - 1, "%u", (unsigned int)active);
    std::snprintf(sdormant, sizeof(sdormant) - 1, "%u", (unsigned int)dormant);
    std::snprintf(sdisabled, sizeof(sdisabled) - 1, "%u", (unsigned int)disabled);
    std::snprintf(shdisabled, sizeof(shdisabled) - 1, "%u", (unsigned int)disabledHighlight);
    std::snprintf(sarow, sizeof(sarow) - 1, "%u", (unsigned int)arrowP);

    for (unsigned int i = 0; i < entries.size(); i++)
      width = entries[i].size() > width ? entries[i].size() : width;

    if (maxHeight == 0)
      maxHeight = height;

    if (height > maxHeight)
      width += 2;

    for (unsigned int i = 0; i < height; i++)
    {
      AnimationFrame frame;
      unsigned int low = height > maxHeight ? i : 0;
      unsigned int high = height > maxHeight ? i + maxHeight : height;
      if (high > height)
        high = height;
      if (height > maxHeight && high - low < maxHeight) low -= maxHeight - (high - low);
      for (unsigned int j = low; j < high; j++)
      {
        std::string name = entries[j];
        if (name.size() < width)
        {
          if (!alignRight)
            name.append(width - name.size(), ' ');
          else
            name.insert(name.begin(), width - name.size(), ' ');
        }

        if (height > maxHeight)
        {
          if (!alignRight)
            frame.frame.push_back( { name, std::string(width - 2, i != j ?
              ((!enabled.empty() && !enabled[j]) ? sdisabled[0] : sdormant[0]) :
              ((!enabled.empty() && !enabled[j]) ? shdisabled[0] : sactive[0]))
              + std::string(sdormant) + std::string(sarow) });
          else
            frame.frame.push_back( { name, std::string(sarow) + std::string(sdormant) +
              std::string(width - 2, i != j ?
              ((!enabled.empty() && !enabled[j]) ? sdisabled[0] : sdormant[0]) :
              ((!enabled.empty() && !enabled[j]) ? shdisabled[0] : sactive[0])) } );
        }
        else
        {
          frame.frame.push_back( { name, std::string(width, i == j ? sactive[0] : sdormant[0]) });
        }

        if (height > maxHeight && low != high)
        {
          int pos = alignRight ? 0 : frame.frame[frame.frame.size() - 1].geometry.size() - 1;
          if (j == low)
            frame.frame[frame.frame.size() - 1].geometry[pos] = charUp;
          else if (j == high - 1)
            frame.frame[frame.frame.size() - 1].geometry[pos] = charDn;
          /* else
           *         frame.frame[frame.frame.size()-1].geometry[frame.frame[frame.frame.size()-1].geometry.size()-1] = '#'; */
        }
      }

      frame.height = maxHeight;
      frame.width = width;
      frame.palette = palette;
      if (!tags.empty())
        frame.tag = tags[i];
      else
        frame.tag = nullptr;
      menu.push_back(frame);
    }
  }
}
