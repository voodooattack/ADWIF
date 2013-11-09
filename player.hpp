#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace ADWIF
{
  class Player
  {
  public:
    // Intelligence, Wisdom, Charisma, Strength, Endurance, Dexterity, Agility
    enum Gender
    {
      Male,
      Female
    };

    Player(std::shared_ptr<class Game> & game);

    const std::string & name() const { return myName; }
    void name(const std::string & name) { myName = name; }

    Gender gender() const { return myGender; }
    void gender(Gender gender) { myGender = gender; }

    struct Race * race() const { return myRace; }
    void race(struct Race * race) { myRace = race; }

    struct Faction * faction() const { return myFaction; }
    void faction(struct Faction * faction) { myFaction = faction; }

    struct Profession * profession() const { return myProfession; }
    void profession(struct Profession * profession) { myProfession = profession; }

  private:
    std::shared_ptr<class Game> myGame;
    class Race * myRace;
    class Faction * myFaction;
    class Profession * myProfession;
    std::string myName;
    Gender myGender;
  };
}

#endif // PLAYER_HPP
