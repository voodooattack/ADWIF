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

namespace ADWIF
{
  class Game;
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
    bool grow;
    std::vector<std::string> needs;
    palEntry style;
    bool background;

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
    int layerStart, layerEnd;
    uint32_t mapColour;
    bool background;

    Json::Value jsonValue;

    static Biome * parse(const Json::Value & value);
  };

  class Game
  {
  public:
    Game(std::shared_ptr<class Engine> & engine);
    ~Game() ;

    void init();

    void reloadData();
    void clearData();
    void createNew(std::shared_ptr<class Player> & player) { this->player(player); }

    void load(const std::string & fileName) { }
    void save(const std::string & fileName) { }
    void load(const std::istream & stream) { }
    void save(const std::ostream & stream) { }

    void createMap();
    void loadMap();
    void saveMap();

    std::shared_ptr<class Engine> engine() { return myEngine; }

    std::shared_ptr<class Player> player() const { return myPlayer; }
    void player(std::shared_ptr<class Player> & player);

    std::shared_ptr<class Map> map() const { return myMap; }
    void map(std::shared_ptr<class Map> & map) { myMap = map; }

    std::shared_ptr<class MapBank> & mapbank() { return myBank; }
    void mapbank(std::shared_ptr<MapBank> & mapbank) { myBank = mapbank; }

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
    std::shared_ptr<class Engine> myEngine;
    std::shared_ptr<class Player> myPlayer;
    std::shared_ptr<class Map> myMap;
    std::shared_ptr<class MapBank> myBank;

    std::fstream myCacheStream;

    std::map<std::string, Race *> myRaces;
    std::map<std::string, Profession *> myProfessions;
    std::map<std::string, Skill *> mySkills;
    std::map<std::string, Faction *> myFactions;
    std::map<std::string, Material *> myMaterials;
    std::map<std::string, Biome *> myBiomes;
  };
}

#endif // GAME_HPP

struct stat;
