#ifndef NEWGAMESTATE_HPP
#define NEWGAMESTATE_HPP
#include "gamestate.hpp"
#include "animation.hpp"

#include <vector>
#include <string>

namespace ADWIF
{
  class NewGameState: public GameState
  {
    enum State
    {
      PickGender,
      PickRace,
      PickFaction,
      PickProfession,
      PickName,
      ShowOverview,
    };
  public:
    NewGameState(std::shared_ptr<class Engine> & engine);
    virtual ~NewGameState();
    virtual void init();
    virtual void step();
    virtual void consume(int key);
    virtual void activate();
  private:
    void enterState(State state);
    void storeChoice(unsigned int choice);
  private:
    std::shared_ptr<class Engine> myEngine;
    std::shared_ptr<class Game> myGame;
    std::shared_ptr<class Player> myPlayer;
    Animation myRacesAnimation, myGenderAnimation, myFactionsAnimation, myProfessionsAnimation;
    Animation * myCurrentAnimation;
    State myState;
  };
}

#endif // NEWGAMESTATE_HPP
