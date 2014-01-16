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

#ifndef GAME_HPP
#define GAME_HPP

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <memory>
#include <fstream>

//#include <v8.h>
#include <json/json.h>

#include <unordered_map>

#include "animation.hpp"
#include "map.hpp"
#include "engine.hpp"

namespace ADWIF
{
  class Game;
  class MapGenerator;
  class Skill;
  class Profession;
  class Faction;
  class Race;

  class Skill
  {
  public:
    std::string name, desc, dispName, title;
    bool activeSkill;
    std::vector<std::string> dependencies;
    Json::Value jsonValue;

    static Skill * parse(const Json::Value & value);
  };

  class Profession
  {
  public:
    enum NameKind
    {
      SingularMale,
      SingularFemale,
      Plural,
      NameKindMax
    };

    std::string name, desc;
    std::string dispName[3];
    std::vector<std::string> skills;
    bool maleOnly, femaleOnly;
    Json::Value jsonValue;

    static Profession * parse(const Json::Value & value);
  };

  class Faction
  {
  public:
    std::string name, dispName, surname, desc, home;
    bool joinable;
    Json::Value jsonValue;

    static Faction * parse(const Json::Value & value);
  };

  class Race
  {
  public:
    std::string name, desc;
    std::vector<std::string> allowedProfessions;
    std::vector<std::string> factions;

    Json::Value jsonValue;

    static Race * parse(const Json::Value & value);
  };

  class Material
  {
  public:
    struct dispEntry
    {
      int sym;
      palEntry style;
    };
    std::string name;
    std::string desc;
    std::unordered_map<TerrainType, std::vector<dispEntry>, std::hash<uint8_t> > disp;
    palEntry style;
    bool liquid;

    Json::Value jsonValue;

    static Material * parse(const Json::Value & value);
  };

  class Biome
  {
  public:
    std::string name, desc;
    int sym;
    palEntry style;
    std::vector<std::string> materials;
    std::vector<std::string> liquids;
    int layerStart, layerEnd;
    uint32_t mapColour;
    bool background;
    bool aquatic;

    Json::Value jsonValue;

    static Biome * parse(const Json::Value & value);
  };

  class Game: public std::enable_shared_from_this<Game>
  {
  public:
    Game(const std::shared_ptr<class Engine> & engine);
    ~Game() ;

    void init();
    void shutdown(bool graceful = true);

    void reloadData();
    void clearData();
    void createNew(std::shared_ptr<class Player> & player) { this->player(player); }

    void load(const std::string & fileName);
    void save(const std::string & fileName);

    void createMap();
    void loadMap();
    void saveMap();

    std::shared_ptr<class Engine> engine() { return myEngine.lock(); }
    const std::shared_ptr<class Engine> engine() const { return myEngine.lock(); }

    std::shared_ptr<class Player> & player() { return myPlayer; }
    const std::shared_ptr<class Player> & player() const { return myPlayer; }
    void player(const std::shared_ptr<class Player> & player);

    std::shared_ptr<class Map> & map() { return myMap; }
    const std::shared_ptr<class Map> & map() const { return myMap; }
    void map(const std::shared_ptr<class Map> & map) { myMap = map; }

    std::shared_ptr<MapGenerator> & generator() { return myGenerator; }
    const std::shared_ptr<MapGenerator> & generator() const { return myGenerator; }
    void generator(const std::shared_ptr<class MapGenerator> & generator) { myGenerator = generator; }

    std::map<std::string, Race *> & races() { return myRaces; }
    std::map<std::string, Profession *> & professions() { return myProfessions; }
    std::map<std::string, Skill *> & skills() { return mySkills; }
    std::map<std::string, Faction *> & factions() { return myFactions; }
    std::map<std::string, Material *> & materials() { return myMaterials; }
    std::map<std::string, Biome *> & biomes() { return myBiomes; }

    const std::map<std::string, Race *> & races() const { return myRaces; }
    const std::map<std::string, Profession *> & professions() const { return myProfessions; }
    const std::map<std::string, Skill *> & skills() const { return mySkills; }
    const std::map<std::string, Faction *> & factions() const { return myFactions; }
    const std::map<std::string, Material *> & materials() const { return myMaterials; }
    const std::map<std::string, Biome *> & biomes() const { return myBiomes; }

  private:

    void loadSkills(const Json::Value & skills);
    void loadProfessions(const Json::Value & professions);
    void loadFactions(const Json::Value & factions);
    void loadRaces(const Json::Value & races);
    void loadMaterials(const Json::Value & materials);
    void loadBiomes(Json::Value biomes);

    void sanityCheck();

  private:
    std::weak_ptr<class Engine> myEngine;
    std::shared_ptr<class Player> myPlayer;
    std::shared_ptr<class Map> myMap;

    std::map<std::string, Race *> myRaces;
    std::map<std::string, Profession *> myProfessions;
    std::map<std::string, Skill *> mySkills;
    std::map<std::string, Faction *> myFactions;
    std::map<std::string, Material *> myMaterials;
    std::map<std::string, Biome *> myBiomes;

    std::shared_ptr<class MapGenerator> myGenerator;
  };
}

#endif // GAME_HPP

struct stat;
