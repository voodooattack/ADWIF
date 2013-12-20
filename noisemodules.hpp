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

#ifndef NOISEMODULES_H
#define NOISEMODULES_H

#include "config.hpp"

#ifdef NOISE_DIR_IS_LIBNOISE
#include <libnoise/noise.h>
#else
#include <noise/noise.h>
#endif

#include <boost/multi_array.hpp>

namespace ADWIF
{
  class HeightMapModule: public noise::module::Module
  {
  public:
    inline static double cubicInterpolate (const double p[4], double x) {
      return p[1] + 0.5 * x*(p[2] - p[0] + x*(2.0*p[0] - 5.0*p[1] +
      4.0*p[2] - p[3] + x*(3.0*(p[1] - p[2]) + p[3] - p[0])));
    }

    inline static double bicubicInterpolate (const double p[4][4], double x, double y) {
      double arr[4];
      arr[0] = cubicInterpolate(p[0], y);
      arr[1] = cubicInterpolate(p[1], y);
      arr[2] = cubicInterpolate(p[2], y);
      arr[3] = cubicInterpolate(p[3], y);
      return cubicInterpolate(arr, x);
    }

    HeightMapModule(const boost::multi_array<double, 2> & heightMap,
                    int cellSizeX, int cellSizeY): Module(0), myHeightmap(heightMap),
                    myCellSizeX(cellSizeX), myCellSizeY(cellSizeY) { }

    virtual int GetSourceModuleCount() const { return 0; }

    virtual double GetValue(double x, double y, double z) const;

    int myCellSizeX, myCellSizeY;
    const boost::multi_array<double, 2> myHeightmap;
  };

}

#endif // NOISEMODULES_H
