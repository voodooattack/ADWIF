#ifndef GAMESTATE_HPP
#define GAMESTATE_HPP

namespace ADWIF
{
  class GameState
  {
  public:
    GameState(): myFinishedFlag(false) { }
    virtual ~GameState() { }
    virtual void init() = 0;
    virtual void step() = 0;
    virtual void consume(int key) = 0;
    virtual bool done() { return myFinishedFlag; }
    virtual void done(bool finished) { myFinishedFlag = finished; }
    virtual void resize() { }
    virtual void activate() { }
  private:
    bool myFinishedFlag;
  };
}

#endif // GAMESTATE_HPP
