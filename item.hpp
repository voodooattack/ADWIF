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
