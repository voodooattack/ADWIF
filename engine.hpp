#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <vector>
#include <memory>

namespace ADWIF
{
  class Engine
  {
  public:
    Engine(std::shared_ptr<class Renderer> renderer, std::shared_ptr<class Input> input);
    ~Engine();

    std::shared_ptr<class Renderer> renderer() const { return myRenderer; }
    std::shared_ptr<class Input> input() const { return myInput; }

    unsigned int delay() const;
    void delay(unsigned int delay);

    bool running() const { return myRunningFlag; }
    void running(bool r) { myRunningFlag = r; }

    int start();

    void sleep(unsigned ms);

    void addState(std::shared_ptr<class GameState> & state);
    void reportError(bool fatal, const std::string & report) ;

  private:
    bool checkScreenSize();
  private:
    std::shared_ptr<class Renderer> myRenderer;
    std::shared_ptr<class Input> myInput;
    std::vector <
      std::shared_ptr<class GameState>
    > myStates;
    unsigned int myDelay;
    bool myRunningFlag;
  };
}

#endif // ENGINE_HPP
