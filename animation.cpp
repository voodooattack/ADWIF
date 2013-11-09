#include "animation.hpp"
#include "renderer.hpp"

#include <cstdlib>

namespace ADWIF
{

  Animation::Animation(std::shared_ptr<ADWIF::Renderer> renderer):
    myRenderer(renderer), myFrameTime(0), myCurrentFrame(0), myWidth(0), myHeight(0),
    myAdvanceFlag(true), myReverseFlag(false)
  {

  }
  void Animation::addFrame(const AnimationFrame & frame)
  {
    if (myFrames.empty())
    {
      myFrameTime = frame.frameTime;
      myCurrentFrame = 0;
    }
    myFrames.push_back(frame);
    myWidth = myWidth > frame.width ? myWidth : frame.width;
    myHeight = myHeight > frame.height ? myHeight : frame.height;
  }

  void Animation::loadAnimation(const std::vector<AnimationFrame> & anim)
  {
    if (!anim.empty())
    {
      myFrames = anim;
      myFrameTime = myFrames[0].frameTime;
      myCurrentFrame = 0;
      calculateBounds();
    }
    else
    {
      myFrames.clear();
      myFrameTime = 0;
      myCurrentFrame = 0;
      myWidth = 0;
      myHeight = 0;
    }
  }

  void Animation::render(int x, int y, double dt)
  {
    renderFrame(myRenderer.get(), myFrames[myCurrentFrame], x, y);
    if (myAdvanceFlag)
    {
      myFrameTime -= dt;
      if (myFrameTime <= 0)
      {
        if (myReverseFlag)
          prev();
        else
          next();
        myFrameTime = myFrames[myCurrentFrame].frameTime;
      }
    }
  }

  void Animation::renderFrame(Renderer * renderer, const AnimationFrame & f, int x, int y)
  {
    unsigned int height = f.height < f.frame.size() ? f.frame.size() : f.height;
    for (unsigned int row = 0; row < height; row++)
    {
      unsigned int width = f.width < f.frame[row].geometry.size() ? f.frame[row].geometry.size() : f.width;
      width = width < f.frame[row].colours.size() ? f.frame[row].colours.size() : width;
      renderer->drawText(x, y + row, f.frame[row].geometry);
      for (unsigned int i = 0; i < width; i++)
      {
        char c[2] = { f.frame[row].colours[i], 0 };
        const palEntry & p = f.palette[std::strtol(c, 0, 16)];
        renderer->style(x + i, y + row, 1, p.fg, p.bg, p.style);
      }
    }
  }

  void Animation::calculateBounds()
  {
    for (unsigned int i = 0; i < myFrames.size(); i++)
    {
      myFrames[i].calculateBounds();
      myWidth = myWidth > myFrames[i].width ? myWidth : myFrames[i].width;
      myHeight = myHeight > myFrames[i].height ? myHeight : myFrames[i].height;
    }
  }

  void Animation::normalise()
  {
    for (unsigned int i = 0; i < myFrames.size(); i++)
      myFrames[i].normalise();
  }
}

