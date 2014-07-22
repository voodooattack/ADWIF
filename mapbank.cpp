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


#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "mapbank.hpp"
#include "fileutils.hpp"

namespace ADWIF
{
  MapBank::MapBank(std::iostream & stream) : myStream(stream), myCache(), myAccessTimes()
  {
    if (!stream.good())
      throw std::runtime_error("bad stream state initializing map bank");
  }

  const MapCell & MapBank::get(uint64_t hash)
  {
//     boost::upgrade_lock<boost::shared_mutex> guard(myMutex);
    auto cell = myCache.find(hash);
    if (cell == myCache.end())
    {
//       boost::upgrade_to_unique_lock<boost::shared_mutex> lock(guard);
      myCache[hash] = loadCell(hash);
    }
    if (myAccessTimes[hash] != myClock.now())
    {
//       boost::upgrade_to_unique_lock<boost::shared_mutex> lock(guard);
      myAccessTimes[hash] = myClock.now();
    }
    if (myCache.size() > 1024 * 1024)
      prune();
    return myCache[hash];
  }

  void MapBank::prune(bool pruneAll)
  {
//     boost::unique_lock<boost::shared_mutex> guard(myMutex);
    std::unordered_map<uint64_t, MapCell> toStore;
    auto i = myAccessTimes.begin();
    while (i != myAccessTimes.end())
    {
      if (pruneAll || myClock.now() - i->second > std::chrono::minutes(2))
      {
        auto item = myCache.find(i->first);
        toStore.insert({item->first, item->second});
        myCache.unsafe_erase(myCache.find(i->first));
        i = myAccessTimes.unsafe_erase(i);
      } else ++i;
    }
    myStream.clear();
    myStream.seekg(0, std::ios_base::beg);
    while (!toStore.empty())
    {
      uint64_t fhash;
      uint32_t size;
      if (read<uint64_t>(myStream, fhash) && read<uint32_t>(myStream, size))
      {
        myStream.seekg(size, std::ios_base::cur);
        toStore.erase(fhash);
      }
      else
        break;
    }
    for (auto const & i : toStore)
    {
      myStream.clear();
      write<uint64_t>(myStream, i.first);
      std::streampos sizePos = myStream.tellp();
      write<uint32_t>(myStream, -1);
      std::streampos start = myStream.tellp();
      boost::archive::binary_oarchive io(myStream, boost::archive::no_header);
      io & i.second;
      std::streampos endpos = myStream.tellp();
      std::streamsize size = endpos - start;
      myStream.seekp(sizePos, std::ios_base::beg);
      write<uint32_t>(myStream, (uint32_t)size);
      myStream.seekp(endpos, std::ios_base::beg);
    }
  }

  MapCell MapBank::loadCell(uint64_t hash)
  {
    boost::unique_lock<boost::shared_mutex> guard(myMutex);
    myStream.clear();
    myStream.seekg(0, std::ios_base::beg);

    while (myStream.good())
    {
      uint64_t fhash;
      uint32_t size;

      if (!read<uint64_t>(myStream, fhash) || !read<uint32_t>(myStream, size))
      {
        myStream.clear();
        myStream.seekg(0, std::ios_base::beg);
        break;
      }

      if (fhash == hash)
      {
        MapCell cell;
        boost::archive::binary_iarchive ia(myStream, boost::archive::no_header);
        ia & cell;
        if (cell.calcHash() != fhash)
          throw std::runtime_error("invalid cell hash " + boost::lexical_cast<std::string>(hash));
        myStream.seekg(0, std::ios_base::beg);
        return std::move(cell);
      }
      else
        myStream.seekg(size, std::ios_base::cur);
    }

    throw std::runtime_error("stream error finding cell " + boost::lexical_cast<std::string>(hash));
    return MapCell();
  }

  uint64_t MapBank::put(const MapCell & cell)
  {
    uint64_t hash = cell.hash();
//     boost::upgrade_lock<boost::shared_mutex> guard(myMutex);
    if (myCache.find(hash) == myCache.end())
    {
//       boost::upgrade_to_unique_lock<boost::shared_mutex> lock(guard);
      myAccessTimes[hash] = myClock.now();
      myCache[hash] = cell;
    }
    return hash;
  }
}
