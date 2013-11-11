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

#include "mapbank.hpp"

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/lexical_cast.hpp>

namespace ADWIF
{
  MapBank::MapBank(std::iostream & stream) : myStream(stream), myCache(), myAccessTimes()
  {
    if (!stream.good())
      throw std::runtime_error("bad stream state initializing map bank");
  }

  const MapCell & MapBank::get(uint64_t hash)
  {
    auto cell = myCache.find(hash);
    if (cell == myCache.end())
    {
      myCache[hash] = loadCell(hash);
    }
    myAccessTimes[hash] = myClock.now();
    if (myCache.size() > 1024 * 1024)
      prune();
    return myCache[hash];
  }

  void MapBank::prune(bool pruneAll)
  {
    std::unordered_map<uint64_t, MapCell> toStore;
    auto i = myAccessTimes.begin();
    while (i != myAccessTimes.end())
    {
      if (pruneAll || myClock.now() - i->second > std::chrono::seconds(4))
      {
        auto item = myCache.find(i->first);
        toStore.insert({item->first, item->second});
        myCache.erase(myCache.find(i->first));
        i = myAccessTimes.erase(i);
      } else ++i;
    }
    myStream.seekg(std::ios_base::beg);
    myStream.seekp(std::ios_base::beg);
    while (!toStore.empty())
    {
      uint64_t fhash;// = read<uint64_t>(myStream);
      uint32_t size; // = read<uint32_t>(myStream);
      //
      if (read<uint64_t>(myStream, fhash) && read<uint32_t>(myStream, size))
      {
        myStream.seekg(size, std::ios_base::cur);
        toStore.erase(fhash);
      }
      else
      {
        myStream.clear();
        myStream.seekp(myStream.tellg(), std::ios_base::beg);
        for (auto const & i : toStore)
        {
          write<uint64_t>(myStream, i.first);
          std::ostringstream ss(std::ios_base::binary);
          {
            boost::archive::binary_oarchive io(ss, boost::archive::no_header);
            io & i.second;
          }
          const std::string str = ss.str();
          write<std::string::value_type,uint32_t>(myStream, str);
        }
        break;
      }
    }
  }

  MapCell MapBank::loadCell(uint64_t hash)
  {
    myStream.seekg(std::ios_base::beg);
    MapCell cell;

    while (myStream.good())
    {
      uint64_t fhash;
      uint32_t size;
      if (!read<uint64_t>(myStream, fhash) || !read<uint32_t>(myStream, size))
        throw std::runtime_error("malformed map bank");

      if (fhash == hash)
      {
        boost::archive::binary_iarchive ia(myStream, boost::archive::no_header);
        ia & cell;
        return std::move(cell);
      }
      else
        myStream.seekg(size, std::ios_base::cur);
    }

    throw std::runtime_error("stream error finding cell " + boost::lexical_cast<std::string>(hash));
    //return MapCell();
  }

  uint64_t MapBank::put(const MapCell & cell)
  {
    uint64_t hash = cell.hash();
    myAccessTimes[hash] = myClock.now();
    if (myCache.find(hash) == myCache.end())
      myCache[hash] = cell;
    return hash;
  }
}
