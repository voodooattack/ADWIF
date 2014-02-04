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

#include "noisemodules.hpp"

namespace ADWIF
{
  double HeightMapModule::GetValue(double x, double y, double z) const
  {
    int cellX = x / myCellSizeX, cellY = y / myCellSizeY;
    int xmin = myHeightmap.index_bases()[0], xmax = myHeightmap.shape()[0];
    int ymin = myHeightmap.index_bases()[1], ymax = myHeightmap.shape()[1];

    constexpr double offsetx = 0.25, offsety = 0.25;

    auto clamp = [](int x, int min, int max)
    {
      if (x < min) return min;
      if (x > max) return max;
      return x;
    };

    unsigned int id[4][4][2];

    for (int j = 0; j <= 3; j++)
      for (int i = 0; i <= 3; i++)
      {
        id[i][j][0] = clamp(cellX + i - offsetx * 4, xmin, xmax);
        id[i][j][1] = clamp(cellY + j - offsety * 4, ymin, ymax);
      }

    double m[4][4];

    for (int j = 0; j <= 3; j++)
      for (int i = 0; i <= 3; i++)
        m[i][j] = myHeightmap[id[i][j][0]][id[i][j][1]];

    double vx = fmod(x, myCellSizeX) / (double)myCellSizeX, vy = fmod(y, myCellSizeY) / (double)myCellSizeY;
    return bicubicInterpolate(m, offsetx + vx, offsety + vy);
  }
}

