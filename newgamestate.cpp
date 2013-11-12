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

#include "newgamestate.hpp"
#include "renderer.hpp"
#include "engine.hpp"
#include "game.hpp"
#include "input.hpp"
#include "introanimation.hpp"
#include "util.hpp"
#include "animationutils.hpp"
#include "player.hpp"
#include "mapgenstate.hpp"

#include <algorithm>
#include <iterator>
#include <locale>
#include <memory>
#include <sstream>

namespace ADWIF
{
/*  static const char selKeys[] =
  {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
    'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y',
    'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '!', '@', '#', '$', '%', '^',
    '&', '*', '(', ')', '-', '_', '+', '=', '|', '[', ']', '{', '}', ',', '.', '/', '?',
    '"', ';', ':', '>', '<', '~', '\\', '\'',
  }; */

  NewGameState::NewGameState(const std::shared_ptr<ADWIF::Engine> & engine) :
    myEngine(engine), myGame(nullptr), myPlayer(nullptr), myRacesAnimation(engine->renderer()),
    myGenderAnimation(engine->renderer()), myFactionsAnimation(engine->renderer()),
    myProfessionsAnimation(engine->renderer()), myCurrentAnimation(&myGenderAnimation),
    myState(State::PickGender)
  {
    myGame.reset(new Game(myEngine));
    myPlayer.reset(new Player(myGame));
    myRacesAnimation.advance(false);
    myGenderAnimation.advance(false);
    myFactionsAnimation.advance(false);
    myProfessionsAnimation.advance(false);
  }

  NewGameState::~NewGameState()
  {
  }

  void NewGameState::init()
  {
    done(false);
    myGame->init();

    std::vector<AnimationFrame> racesFrames, genderFrames;
    std::vector<std::string> raceNames;
    std::vector<void*> raceTags;

    std::transform(myGame->races().begin(), myGame->races().end(), std::back_inserter(raceNames),
                   [](const std::pair<const std::string,Race*> & p) { return p.second->name; });
    std::transform(myGame->races().begin(), myGame->races().end(), std::back_inserter(raceTags),
                   [](const std::pair<const std::string,Race*> & p) { return p.second; });

    generateMenu(racesFrames, raceNames, introPalette, 8, 1, false, 6, raceTags, std::vector<bool>(), 0, 0);
    generateMenu(genderFrames, { "Male", "Female" }, introPalette, 8, 1);

    myRacesAnimation.loadAnimation(racesFrames);
    myRacesAnimation.normalise();

    myGenderAnimation.loadAnimation(genderFrames);
    myGenderAnimation.normalise();
  }

  void NewGameState::step()
  {
    std::string title, desc,
      bottomText1 = "Press ENTER to select",
      bottomText2 = "Press ESC to go back";

    switch (myState)
    {
      case State::PickGender:
      {
        title = "Choose your gender";
        desc.clear();
        break;
      }
      case State::PickRace:
      {
        title = "Choose your ancestry";
        auto race = myGame->races().begin();
        std::advance(race, myRacesAnimation.seek());
        if (!race->second->desc.empty())
          desc = race->second->desc;
        break;
      }
      case State::PickFaction:
      {
        title = "Choose a faction";
        auto faction = myPlayer->race()->factions.begin();
        std::advance(faction, (myCurrentAnimation->seek()));
        if (faction != myPlayer->race()->factions.end())
          desc = myGame->factions()[*faction]->desc;
        break;
      }
      case State::PickProfession:
      {
        title = "Choose a profession";
        auto profession = myPlayer->race()->allowedProfessions.begin();
        std::advance(profession, myCurrentAnimation->seek());
        if (profession != myPlayer->race()->allowedProfessions.end())
          desc = myGame->professions()[*profession]->desc;
        break;
      }
      case State::ShowOverview:
      {
        title = "Review your character";
        bottomText1 = "Press ENTER to continue";
        desc.clear();
        break;
      }
      default: break;
    }

    myEngine->renderer()->style(White, Black, Style::Bold);
    myEngine->renderer()->drawText(myEngine->renderer()->width() / 2 - title.size() / 2, 2, title);

    if (myCurrentAnimation)
      myCurrentAnimation->render(myEngine->renderer()->width() / 2 -
        myCurrentAnimation->width() / 2, 4, myEngine->delay() / 1000.0);
    if (!desc.empty())
    {
      myEngine->renderer()->startWindow(8, 5 + myCurrentAnimation->height(), myEngine->renderer()->width() - 16,
                                        myEngine->renderer()->height() - 5 - myCurrentAnimation->height());
      myEngine->renderer()->style(Yellow, Black, Style::Normal);
      myEngine->renderer()->drawText(0, 0, wrapText(desc, myEngine->renderer()->width() - 16));
      myEngine->renderer()->endWindow();
    }

    myEngine->renderer()->style(Green, Black, Style::Normal);

    int overviewOffsetX = 1, overviewOffsetY = 4;

    switch(myState)
    {
      case State::ShowOverview:
      {
        myEngine->renderer()->style(Yellow, Black, Style::Normal);
        overviewOffsetX = myEngine->renderer()->width() / 2 - title.size() / 2;
        overviewOffsetY = 4;
        myEngine->renderer()->drawText(overviewOffsetX, overviewOffsetY + 4, "      Name: " + myPlayer->name());
      }
      case State::PickName:
        myEngine->renderer()->drawText(overviewOffsetX, overviewOffsetY + 3, "Profession: " +
          myPlayer->profession()->dispName[myPlayer->gender() == Player::Male ? 0 : 1]);
      case State::PickProfession:
        if (myPlayer->faction()) myEngine->renderer()->drawText(overviewOffsetX, overviewOffsetY + 2,
            "   Faction: " + myPlayer->faction()->dispName);
        else
          myEngine->renderer()->drawText(overviewOffsetX, overviewOffsetY + 2, "   Faction: Commoner");
      case State::PickFaction:
        myEngine->renderer()->drawText(overviewOffsetX, overviewOffsetY + 1, "      Race: " + myPlayer->race()->name);
      case State::PickRace:
        myEngine->renderer()->drawText(overviewOffsetX, overviewOffsetY, "    Gender: " +
          std::string(myPlayer->gender() == Player::Gender::Female ? "Female" : "Male"));
      case State::PickGender: break;
    }

    myEngine->renderer()->style(White, Black, Style::Normal);
    myEngine->renderer()->drawText(myEngine->renderer()->width() / 2 - bottomText1.size() / 2,
                                   myEngine->renderer()->height() - 3, bottomText1);
    myEngine->renderer()->drawText(myEngine->renderer()->width() / 2 - bottomText2.size() / 2,
                                   myEngine->renderer()->height() - 2, bottomText2);
  }

  void NewGameState::consume(int key)
  {
    if (key == Key::Escape)
    {
      switch (myState)
      {
        case State::PickGender: done(true); break;
        case State::PickRace: enterState(State::PickGender); break;
        case State::PickFaction: enterState(State::PickRace); break;
        case State::PickProfession: enterState(myPlayer->race()->factions.empty() ?
          State::PickRace : State::PickFaction); break;
        case State::PickName: enterState(PickProfession); break;
        case State::ShowOverview: enterState(PickProfession); break;
      }
    }
    else if (myCurrentAnimation && (key == Key::Up || key == Key::Num8))
      myCurrentAnimation->prev();
    else if (myCurrentAnimation && (key == Key::Down || key == Key::Num2))
      myCurrentAnimation->next();
    else if (key == '\r' || key == '\n')
    {
      if (myState == State::ShowOverview)
      {
        auto state = std::dynamic_pointer_cast<GameState>(
          std::shared_ptr<MapGenState>(new MapGenState(myEngine, myGame)));
        myEngine->addState(state);
        done(true);
      }
      else if (myState != State::PickName)
        storeChoice(myCurrentAnimation->seek());
    }
    myEngine->renderer()->clear();
  }

  void NewGameState::activate() { myEngine->renderer()->clear(); }

  void NewGameState::enterState(NewGameState::State state)
  {
    myState = state;
    switch (state)
    {
      case State::PickGender: myCurrentAnimation = &myGenderAnimation; break;
      case State::PickRace: myCurrentAnimation = &myRacesAnimation; break;
      case State::PickFaction:
      {
        if (!myPlayer->race()->factions.empty())
        {
          std::vector<AnimationFrame> factionFrames;
          std::vector<std::string> factionNames;
          std::vector<void*> factionTags;

          factionNames.push_back("Commoner");
          factionTags.push_back(nullptr);

          std::transform(myPlayer->race()->factions.begin(), myPlayer->race()->factions.end(), std::back_inserter(factionNames),
                         [this](const std::string & p) { return myGame->factions()[p]->dispName; });
          std::transform(myPlayer->race()->factions.begin(), myPlayer->race()->factions.end(), std::back_inserter(factionTags),
                         [this](const std::string & p) { return myGame->factions()[p]; });

          generateMenu(factionFrames, factionNames, introPalette, 8, 1, false, 6, factionTags, std::vector<bool>(), 0, 0);
          myFactionsAnimation.loadAnimation(factionFrames);
          myFactionsAnimation.normalise();
          myCurrentAnimation = &myFactionsAnimation;
          break;
        }
        else
          myState = State::PickProfession;
      }
      case State::PickProfession:
      {
        std::vector<AnimationFrame> profFrames;
        std::vector<std::string> profNames;
        std::vector<bool> profEnabled;
        std::vector<void*> profTags;

        std::transform(myPlayer->race()->allowedProfessions.begin(), myPlayer->race()->allowedProfessions.end(),
                       std::back_inserter(profNames), [this](const std::string & p) {
                         return myGame->professions()[p]->dispName[myPlayer->gender() == Player::Gender::Male ? 0 : 1];
                      });
        std::transform(myPlayer->race()->allowedProfessions.begin(), myPlayer->race()->allowedProfessions.end(),
                       std::back_inserter(profEnabled), [this](const std::string & p) {
                         return ((myGame->professions()[p]->maleOnly && myPlayer->gender() == Player::Gender::Male) ||
                         (myGame->professions()[p]->femaleOnly && myPlayer->gender() == Player::Gender::Female) ||
                         (!myGame->professions()[p]->maleOnly && !myGame->professions()[p]->femaleOnly));
                       });
        std::transform(myPlayer->race()->allowedProfessions.begin(), myPlayer->race()->allowedProfessions.end(),
                       std::back_inserter(profTags), [this](const std::string & p) {
                         return myGame->professions()[p];
                       });
        generateMenu(profFrames, profNames, introPalette, 8, 1, false, 6, profTags, profEnabled, 6, 5);
        myProfessionsAnimation.loadAnimation(profFrames);
        myProfessionsAnimation.normalise();
        myCurrentAnimation = &myProfessionsAnimation;
        break;
      }
      case State::PickName:
      {
        myCurrentAnimation = nullptr;
        myEngine->renderer()->refresh();
        bool valid = false;
        std::string message = "Enter your name:";
        while (!valid)
        {
          const int maxNameLen = 20;
          std::string name;
          if (!myPlayer->faction() || myPlayer->faction()->surname.empty())
            name = myEngine->input()->prompt(message, maxNameLen);
          else
            name = myEngine->input()->prompt(message, maxNameLen -
                                             myPlayer->faction()->surname.length(),
                                             myPlayer->faction()->surname);
          if (name.size() >= 2)
          {
            valid = std::all_of(name.begin(), name.end(), [this](const char c) {
              return std::isalpha(c) || ((!myPlayer->faction() || myPlayer->faction()->surname.empty()) && c == ' '); });
            if (valid)
            {
              std::vector<std::string> nameComponents;
              std::istringstream iss(name);
              std::copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(),
                             std::back_inserter(nameComponents));
              name.clear();
              for(std::string & str: nameComponents)
              {
                trim(str);
                if (!str.empty())
                {
                  bool first = true;
                  for (auto & c : str)
                    c = first ? first = false, std::toupper(c) : std::tolower(c);
                  name += " " + trim(str);
                }
              }
              trim(name);
              if (!name.empty())
              {
                if (myPlayer->faction() && !myPlayer->faction()->surname.empty())
                  name += " " + myPlayer->faction()->surname;
                myPlayer->name(name);
                enterState(State::ShowOverview);
                break;
              } else
                valid = false;
            }
            else
            {
              message = "Invalid name, please try again:\nName can only include alphabetic characters";
            }
          }
          else
          {
            if (!name.empty())
              valid = false;
            else
            {
              enterState(PickProfession);
              return;
            }
          }
        }
        break;
      }
      case State::ShowOverview:
      default: myCurrentAnimation = nullptr; break;
    }
  }

  void NewGameState::storeChoice(unsigned int choice)
  {
    switch(myState)
    {
      case State::PickGender:
      {
        if (choice == 0)
          myPlayer->gender(Player::Gender::Male);
        else
          myPlayer->gender(Player::Gender::Female);
        enterState(State::PickRace);
        break;
      }
      case State::PickRace:
      {
        myPlayer->race(reinterpret_cast<Race*>(myCurrentAnimation->frame().tag));
        enterState(PickFaction);
        break;
      }
      case State::PickFaction:
      {
        if (!myPlayer->race()->factions.empty())
          myPlayer->faction(reinterpret_cast<Faction*>(myCurrentAnimation->frame().tag));
        enterState(State::PickProfession);
        break;
      }
      case State::PickProfession:
      {
        Profession * prof = reinterpret_cast<Profession*>(myCurrentAnimation->frame().tag);
        if (!(prof->maleOnly && myPlayer->gender() == Player::Gender::Female) &&
          !(prof->femaleOnly && myPlayer->gender() == Player::Gender::Male))
        {
          myPlayer->profession(prof);
          enterState(State::PickName);
        }
        break;
      }
      case State::PickName:
      default: break;
    }
  }
}



