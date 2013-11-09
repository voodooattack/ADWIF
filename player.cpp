#include "player.hpp"
#include "game.hpp"

namespace ADWIF
{
  Player::Player(std::shared_ptr<Game> & game):
    myGame(game), myRace(nullptr), myFaction(nullptr), myProfession(nullptr), myName(), myGender(Male)
  {

  }
}

