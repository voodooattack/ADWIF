#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <iostream>
#include <type_traits>
#include <vector>
#include <string>

namespace ADWIF
{
  extern std::string dirSep;
  extern std::string writeDir;
  extern std::string dataDir;
  extern std::string dataFile;
  extern std::string saveDir;

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
