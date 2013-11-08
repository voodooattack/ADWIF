#ifndef ITEM_H
#define ITEM_H

#include <string>
#include "animation.hpp"
#include "fileutils.hpp"

#include <boost/serialization/access.hpp>
#include <boost/functional/hash.hpp>

namespace ADWIF
{
  class ItemCategory
  {
  public:

  };

  struct ItemContainer
  {
  public:
    virtual size_t itemCount() = 0;
    virtual const class Item & getItem(size_t index) = 0;
    virtual bool addItem(const class Item & item) = 0;
    virtual bool removeItem(const class Item & item) = 0;
  };

  class ItemClass
  {
    std::string name;
    std::string dispName;
    std::string description;
    std::string category;
    std::vector<std::string> reqSkills;
    int symbol;
    palEntry style;
  };

  class Item
  {
  public:
    Item(): myClass("other"), myName(), myDescription(), myCategory("default"),
      mySymbol('?'), myStyle(), myContainer(nullptr) { }

    Item(ItemContainer * container):
      myClass("other"), myName(), myDescription(), myCategory("default"),
      mySymbol('?'), myStyle(), myContainer(container)
    {
      if (myContainer) myContainer->addItem(*this);
    }

    virtual ~Item() { if (myContainer) myContainer->removeItem(*this); }

    virtual void use() { }

    virtual ItemContainer * container() const { return myContainer; }
    virtual void container(ItemContainer * container)
    {
      if (myContainer) myContainer->removeItem(*this);
      if (container) container->addItem(*this);
      myContainer = container;
    }

    virtual uint64_t hash() const
    {
        size_t h = boost::hash<std::string>()(myClass);
        boost::hash_combine(h, myName);
        boost::hash_combine(h, myDescription);
        boost::hash_combine(h, myCategory);
        boost::hash_combine(h, mySymbol);
        boost::hash_combine(h, myStyle.style);
        boost::hash_combine(h, myStyle.bg);
        boost::hash_combine(h, myStyle.fg);
        return h;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
      ar & myClass;
      ar & myName;
      ar & myDescription;
      ar & myCategory;
      ar & mySymbol;
      ar & myStyle.style;
      ar & myStyle.bg;
      ar & myStyle.fg;
      ar & myContainer;
    }

    bool operator== (const Item & other) { return hash() == other.hash(); }

    const std::string & class_() const { return myClass; }
    void class_(const std::string & class_) { myClass = class_; }

    const std::string & name() const { return myName; }
    void name(const std::string & name) { myName = name; }

    const std::string & description() const { return myDescription; }
    void description(const std::string & description) { myDescription = description; }

    const std::string & category() const { return myCategory; }
    void category(const std::string & category) { myCategory = category; }

    int symbol() const { return mySymbol; }
    void symbol(int symbol) { mySymbol = symbol; }

    const palEntry & style() const { return myStyle; }
    void style(const palEntry & style) { myStyle = style; }

  private:
    std::string myClass;
    std::string myName;
    std::string myDescription;
    std::string myCategory;
    int mySymbol;
    palEntry myStyle;
    ItemContainer * myContainer;

    friend class boost::serialization::access;
  };
}

#endif // ITEM_H
