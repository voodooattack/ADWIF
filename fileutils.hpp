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

#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <iostream>
#include <type_traits>
#include <vector>
#include <string>
#include <boost/filesystem/path.hpp>

namespace ADWIF
{
  extern boost::filesystem::path writeDir;
  extern boost::filesystem::path dataDir;
  extern boost::filesystem::path dataFile;
  extern boost::filesystem::path saveDir;

  template <class T>
  bool read(std::istream& stream, T& value,
               typename std::enable_if< std::is_pod<T>::value >::type* dummy = nullptr)
  {
    stream.read(reinterpret_cast<char*>(&value), sizeof(value));
    return stream.good();
  }

  template <class T>
  T read(std::istream& stream)
  {
    T value;
    read(stream, value);
    return value;
  }

  template <class T, class StorageSize = uint32_t>
  StorageSize read(std::istream& stream, std::vector<T>& value,
                      typename std::enable_if< std::is_pod<T>::value >::type* dummy = nullptr)
  {
    StorageSize size = read<StorageSize>(stream);
    for(StorageSize i = 0; i < size; i++)
      value.push_back(read<T>(stream));
    return size;
  }

  template <class T, class StorageSize = uint32_t>
  StorageSize read(std::istream& stream, std::vector<T>& value,
                   typename std::enable_if< std::is_compound<T>::value >::type* dummy = nullptr)
  {
    StorageSize size = read<StorageSize>(stream);
    for(StorageSize i = 0; i < size; i++)
    {
      T v;
      stream >> v;
      value.push_back(std::move(v));
    }
    return size;
  }

  template <class T, class StorageSize = uint32_t>
  StorageSize read(std::istream& stream, std::vector<T*>& value,
                   typename std::enable_if< std::is_compound<T>::value >::type* dummy = nullptr)
  {
    StorageSize size = read<StorageSize>(stream);
    for(StorageSize i = 0; i < size; i++)
    {
      T * v = new T();
      stream >> *v;
      value.push_back(v);
    }
    return size;
  }

  template <class T, class StorageSize = uint32_t>
  StorageSize read(std::istream& stream, std::basic_string<T>& value)
  {
    StorageSize size = read<StorageSize>(stream);
    value.resize(size);
    stream.read(&value[0], size * sizeof(T));
    return size;
  }

  template <class T>
  void write(std::ostream& stream, const T& value,
                typename std::enable_if< std::is_pod<T>::value >::type* dummy = nullptr)
  {
    stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
  }

  template <class T, class StorageSize = uint32_t>
  void write(std::ostream& stream, const std::vector<T>& value,
                typename std::enable_if< std::is_pod<T>::value >::type* dummy = nullptr)
  {
    StorageSize size = value.size();
    write(stream, size);
    for (auto const & i : value)
      write(stream, i);
  }

  template <class T, class StorageSize = uint32_t>
  void write(std::ostream& stream, const std::vector<T>& value,
                typename std::enable_if< std::is_compound<T>::value >::type* dummy = nullptr)
  {
    StorageSize size = value.size();
    write(stream, size);
    for (auto const & i : value)
      stream << i;
  }

  template <class T, class StorageSize = uint32_t>
  void write(std::ostream& stream, const std::vector<T*>& value,
             typename std::enable_if< std::is_compound<T>::value >::type* dummy = nullptr)
  {
    StorageSize size = value.size();
    write(stream, size);
    for (auto const i : value)
      stream << *i;
  }

  template <class T, class StorageSize = uint32_t>
  void write(std::ostream& stream, const std::basic_string<T>& value)
  {
    StorageSize size = value.size();
    write(stream, size);
    stream.write(reinterpret_cast<const char*>(value.c_str()), value.size() * sizeof(T));
//    for (auto const & i : value)
//      stream << i;
  }
}

#endif // FILEUTILS_H
