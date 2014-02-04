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

#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include "renderer.hpp"
#include <vector>
#include <memory>

namespace ADWIF
{
  struct FrameLine
  {
    std::string geometry;
    std::string colours;
  };

  struct AnimationFrame
  {
    std::vector<FrameLine> frame;
    std::vector<palEntry> palette;
    double frameTime;
    unsigned int width, height;
    void * tag;

    void calculateBounds()
    {
      height = frame.size();
      for (unsigned int i = 0; i < frame.size(); i++)
        width = frame[i].geometry.size() > width ? frame[i].geometry.size() : width;
    }

    void normalise()
    {
      calculateBounds();
      for (unsigned int i = 0; i < frame.size(); i++)
      {
        if (frame[i].geometry.size() < width) frame[i].geometry =
            frame[i].geometry + std::string(width - frame[i].geometry.size(), ' ');
        if (frame[i].colours.size() < width) frame[i].colours =
            frame[i].colours + std::string(width - frame[i].colours.size(), ' ');
      }
    }
  };

  class Animation
  {
  public:
    Animation(std::shared_ptr<class Renderer> renderer);

    void addFrame(const AnimationFrame & frame);
    void loadAnimation(const std::vector<AnimationFrame> & anim);

    void render(int x, int y, double dt);

    static void renderFrame(class Renderer * renderer, const AnimationFrame & frame, int x, int y);

    void prev() { myCurrentFrame = (myCurrentFrame ? myCurrentFrame - 1 : myFrames.size() - 1); }
    void next() { myCurrentFrame = (myCurrentFrame + 1) % myFrames.size(); }

    void calculateBounds();
    void normalise();

    unsigned int seek() const { return myCurrentFrame; }
    void seek(unsigned int frame) { myCurrentFrame = frame % myFrames.size(); }

    double frameTime() const { return myFrameTime; }
    void frameTime(double ft) { myFrameTime = ft; }

    int width() const { return myWidth; }
    int height() const { return myHeight; }

    bool advance() const { return myAdvanceFlag; }
    void advance(bool adv) { myAdvanceFlag = adv; }

    bool reverse() const { return myReverseFlag; }
    void reverse(bool rev) { myReverseFlag = rev; }

    AnimationFrame & frame() { return myFrames[seek()]; }

    unsigned int frames() { return myFrames.size(); }

  private:
    std::shared_ptr<class Renderer> myRenderer;
    double myFrameTime;
    unsigned int myCurrentFrame;
    unsigned int myWidth, myHeight;
    bool myAdvanceFlag, myReverseFlag;
    std::vector<AnimationFrame> myFrames;
  };
}

#endif // ANIMATION_HPP
