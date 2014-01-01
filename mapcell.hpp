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

#ifndef MAPCELL_H
#define MAPCELL_H

#include <algorithm>
#include <numeric>
#include <cstdint>

#include <unordered_set>
#include <unordered_map>

#include <boost/functional/hash.hpp>
#include <boost/variant.hpp>

#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
// #include <boost/archive/polymorphic_binary_iarchive.hpp>
// #include <boost/archive/polymorphic_binary_oarchive.hpp>

namespace ADWIF
{
  class Map;
  enum TerrainType : uint8_t
  {
    Floor,
    RampU,
    RampD,
    Hole,
    Wall,
  };

  inline std::string terrainTypeStr(TerrainType type)
  {
    switch(type)
    {
      case Floor: return "Floor";
      case RampU: return "RampU";
      case RampD: return "RampD";
      case Hole: return "Hole";
      case Wall: return "Wall";
    }
    return "Unknown";
  }

  inline TerrainType strTerrainType(const std::string & type)
  {
    std::string t;
    std::transform(type.begin(), type.end(), std::back_inserter(t), &tolower);
    if (t == "floor") return TerrainType::Floor;
    else if (t == "rampu") return TerrainType::RampU;
    else if (t == "rampd") return TerrainType::RampD;
    else if (t == "hole") return TerrainType::Hole;
    else if (t == "wall") return TerrainType::Wall;
    else return TerrainType::Hole;
  }

  enum Structure : uint8_t
  {
    None,
    SWall,
    Door,
    Window,
    Fence,
    Girder,
    Stairs,
    StairsU,
    StairsD,
    StairsUD,
  };

  enum MaterialState
  {
    Solid,
    Liquid,
    Gas
  };

  class Element
  {
  public:
    virtual ~Element() { }

    virtual int volume() const = 0;
    virtual int weight() const = 0;

    virtual uint64_t hash() const = 0;

    virtual MaterialState materialState() const = 0;

    virtual bool isMaterial() const = 0;
    virtual bool isStructure() const = 0;
    virtual bool isAnchored() const = 0;

    virtual Element * clone() const = 0;

  private:
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    }
    friend class boost::serialization::access;
  };

  class MapCell
  {
  public:
//     typedef boost::variant<void*, char, short, int, double, float, std::string> Meta;

    MapCell(): myElements(), myVolume(0), myCachedHash(0) { clear(); }
    explicit MapCell(const MapCell & other)
    {
      clear();
      for (auto i : other.myElements)
        myElements.push_back(i->clone());
      myCachedHash = other.myCachedHash;
      myGeneratedFlag = other.myGeneratedFlag;
      mySeenFlag = other.mySeenFlag;
      myVolume = other.myVolume;
//       myMeta = other.myMeta;
    }
    MapCell(MapCell && other)
    {
      myElements = std::move(other.myElements);
      myCachedHash = other.myCachedHash;
      myGeneratedFlag = other.myGeneratedFlag;
      mySeenFlag = other.mySeenFlag;
      myVolume = other.myVolume;
      //       myMeta = std::move(other.myMeta);
    }
    ~MapCell() { clear(); }

    MapCell & operator= (const MapCell & other)
    {
      clear();
      for (auto i : other.myElements)
        myElements.push_back(i->clone());
      myCachedHash = other.myCachedHash;
      myGeneratedFlag = other.myGeneratedFlag;
      mySeenFlag = other.mySeenFlag;
      myVolume = other.myVolume;
//       myMeta = other.myMeta;
      return *this;
    }

    const std::vector<Element*> & elements() const { return myElements; }
    std::vector<Element*> & elements() { return myElements; }

//     const std::map<std::string, Meta> meta() const { return myMeta; }
//     std::map<std::string, Meta> meta() { return myMeta; }

    bool generated() const { return myGeneratedFlag; }
    void generated(bool g) { myGeneratedFlag = g; calcHash(); }

    bool seen() const { return mySeenFlag; }
    void seen(bool s) { mySeenFlag = s; calcHash(); }

    int temp() const { return myTemp; }
    void temp(int t) { myTemp = t; calcHash(); }

    int pressure() const { return myPressure; }
    void pressure(int p) { myPressure = p; calcHash(); }

    int used() const { return myVolume; }
    int calcUsed() const { return std::accumulate(myElements.begin(), myElements.end(), 0,
      [](int d, const Element * e) -> int { return d + e->volume(); }); }
    int free() const { return MaxVolume - myVolume; }

    void clear()
    {
      for(auto i : myElements) delete i;
      myElements.clear();
//       myMeta.clear();
      myTemp = 294.15; // Kelvins = 21c
      myPressure = 101325; // pascals
      myVolume = 0;
      myGeneratedFlag = false;
      mySeenFlag = false;
      myCachedHash = calcHash();
    }

    bool addElement(Element * e) {
      bool r = addElementFast(e);
      if (r)
        calcHash();
      return r;
    }

    bool addElementFast(Element * e) {
      if (e->volume() > free())
        return false;
      myElements.push_back(e);
      myVolume += e->volume();
      return true;
    }

    void removeElement(Element * e)
    {
      removeElementFast(e);
      calcHash();
    }

    void removeElementFast(Element * e)
    {
      myElements.erase(std::find(myElements.begin(), myElements.end(), e), myElements.end());
      myVolume -= e->volume();
    }

    uint64_t hash() const { return myCachedHash; }

    uint64_t calcHash() const {
      std::size_t h = 0;
      boost::hash_combine(h, myGeneratedFlag);
      boost::hash_combine(h, mySeenFlag);
      boost::hash_combine(h, myTemp);
      boost::hash_combine(h, myPressure);
      boost::hash_combine(h, myVolume);
      boost::hash_combine(h, myElements.size());
      for(auto i : myElements)
        boost::hash_combine(h, i->hash());
      return myCachedHash = h;
    }

    static constexpr int MaxVolume = 1000000000; // mm³

  protected:
    std::vector<Element*> myElements;
//     std::map<std::string, Meta> myMeta;
    bool myGeneratedFlag;
    bool mySeenFlag;
    int myTemp;
    int myPressure;
    int myVolume;
    mutable uint64_t myCachedHash;

  private:
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
      ar & myCachedHash;
      ar & myGeneratedFlag;
      ar & mySeenFlag;
      ar & myTemp;
      ar & myPressure;
      ar & myVolume;
      ar & myElements;
//       ar & myMeta;
    }

    friend class boost::serialization::access;
  };

//   BOOST_CLASS_VERSION(MapCell, 1)

  class MaterialElement: public Element
  {
  public:
    MaterialElement(): material(), vol(0), wgt(0), symIdx(0), state(MaterialState::Solid),
                       anchored(false), alignment(false), cmaterial(nullptr) { }
    virtual ~MaterialElement() { }

    std::string material;
    int vol;
    int wgt;
    unsigned char symIdx;
    MaterialState state;
    bool anchored;
    bool alignment;

    // cached lookups
    mutable struct Material * cmaterial;

    virtual int volume() const { return vol; }
    virtual int weight() const { return wgt; }

    virtual uint64_t hash() const {
      uint64_t h = 0;
      boost::hash_combine(h, material);
      boost::hash_combine(h, vol);
      boost::hash_combine(h, wgt);
      boost::hash_combine(h, symIdx);
      boost::hash_combine(h, (int)state);
      boost::hash_combine(h, anchored);
      return h;
    }

    virtual MaterialState materialState() const { return state; }

    virtual bool isMaterial() const { return true; }
    virtual bool isStructure() const { return false; }
    virtual bool isAnchored() const { return anchored; }

    virtual Element * clone() const { return new MaterialElement(*this); }

  private:
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
      ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Element);
      ar & material;
      ar & vol;
      ar & wgt;
      ar & symIdx;
      ar & state;
      ar & anchored;
      ar & alignment;
    }
    friend class boost::serialization::access;
  };
}

BOOST_CLASS_EXPORT_KEY(ADWIF::Element)
BOOST_CLASS_EXPORT_KEY(ADWIF::MaterialElement)

//   struct MapCell: public ItemContainer
//   {
//     MapCell(): type(TerrainType::Hole), structure(None), material("Air"), smaterial(), biome(), symIdx(0), visible(false),
//     background(false), generated(false), contents(), cmaterial(nullptr), csmaterial(nullptr), cbiome(nullptr) {}
//     ~MapCell() { }
//
//     TerrainType type;
//     Structure structure;
//     std::string material;
//     std::string smaterial;
//     std::string biome;
//     unsigned char symIdx;
//     bool visible, background, generated;
//     std::vector<Item> contents;
//
//     // cached lookups
//     mutable struct Material * cmaterial, * csmaterial;
//     mutable struct Biome * cbiome;
//
//     virtual size_t itemCount()  { return contents.size(); }
//     virtual const Item & getItem(size_t index)  { return contents[index]; }
//
//     virtual bool addItem(const class Item & item) {
//       contents.push_back(item);
//       return true;
//     }
//
//     virtual bool removeItem(const class Item & item) {
//       contents.erase(std::remove(contents.begin(), contents.end(), item), contents.end());
//       return true;
//     }
//
//     virtual uint64_t hash() const {
//       size_t h = 0;
//       boost::hash_combine(h, (int)type);
//       boost::hash_combine(h, (int)structure);
//       boost::hash_combine(h, material);
//       boost::hash_combine(h, smaterial);
//       boost::hash_combine(h, biome);
//       boost::hash_combine(h, symIdx);
//       boost::hash_combine(h, visible);
//       boost::hash_combine(h, background);
//       boost::hash_combine(h, generated);
//       for(auto i : contents)
//         boost::hash_combine(h, i.hash());
//       return h;
//     }
//
//     template<class Archive>
//     void serialize(Archive & ar, const unsigned int version)
//     {
//       ar & type;
//       ar & structure;
//       ar & material;
//       ar & smaterial;
//       ar & biome;
//       ar & symIdx;
//       ar & visible;
//       ar & contents;
//     }
//
//     friend class boost::serialization::access;
//   };


#endif // MAPCELL_H
