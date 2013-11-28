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

#ifndef SERIALISATIONUTILS_H
#define SERIALISATIONUTILS_H

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/collections_save_imp.hpp>
#include <boost/serialization/collections_load_imp.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/multi_array.hpp>
#include <boost/polygon/point_data.hpp>
#include <boost/polygon/polygon_with_holes_data.hpp>
#include <boost/geometry/io/wkt/wkt.hpp>
#include <boost/geometry/io/io.hpp>
#include <boost/geometry/geometries/adapted/boost_polygon.hpp>
#include <unordered_map>


namespace boost
{
  namespace serialization
  {
    template< class Archive, class T >
    inline void save(Archive & ar, const multi_array<T, 2> & t,
                     const unsigned int /* version */)
    {
      unsigned int rows = t.shape()[0];
      unsigned int cols = t.shape()[1];

      ar & rows & cols;

      for (unsigned int i = 0; i < rows; ++i)
        for (unsigned int j = 0; j < cols; ++j)
          ar & t[i][j];
    }

    template< class Archive, class T >
    inline void load(Archive & ar, multi_array<T, 2> & t,
                     const unsigned int /* version */)
    {
      unsigned int rows, cols;
      ar & rows & cols;
      t.resize(boost::extents[rows][cols]);
      for (unsigned int i = 0; i < rows; ++i)
        for (unsigned int j = 0; j < cols; ++j)
          ar & t[i][j];
    }

    template<class Archive, class T>
    inline void serialize(Archive & ar, multi_array<T, 2> & t,
                          const unsigned int file_version)
    {
      boost::serialization::split_free(ar, t, file_version);
    }

    template< class Archive, class T >
    inline void save(Archive & ar, const multi_array<T, 3> & t,
                     const unsigned int /* version */)
    {
      unsigned int rows = t.shape()[0];
      unsigned int cols = t.shape()[1];
      unsigned int depth = t.shape()[2];

      ar & rows & cols & depth;

      for (unsigned int i = 0; i < rows; ++i)
        for (unsigned int j = 0; j < cols; ++j)
          for (unsigned int k = 0; k < depth; ++k)
            ar & t[i][j][k];
    }

    template< class Archive, class T >
    inline void load(Archive & ar, multi_array<T, 3> & t,
                     const unsigned int /* version */)
    {
      unsigned int rows, cols, depth;
      ar & rows & cols & depth;
      t.resize(boost::extents[rows][cols][depth]);
      for (unsigned int i = 0; i < rows; ++i)
        for (unsigned int j = 0; j < cols; ++j)
          for (unsigned int k = 0; k < depth; ++k)
            ar & t[i][j][k];
    }

    template<class Archive, class T>
    inline void serialize(Archive & ar, multi_array<T, 3> & t,
                          const unsigned int file_version)
    {
      boost::serialization::split_free(ar, t, file_version);
    }


    template < class Archive,
             typename UIntType,
             size_t w, size_t n, size_t m, size_t r,
             UIntType a, size_t u, UIntType d, size_t s,
             UIntType b, size_t t,
             UIntType c, size_t l, UIntType f >
    inline void save(Archive & ar, const std::mersenne_twister_engine<UIntType, w, n, m, r, a, u, d, s, b, t, c, l, f> & tt,
                     const unsigned int file_version)
    {
      std::ostringstream os;
      os << tt;
      std::string ss = os.str();
      ar & ss;
    }

    template < class Archive,
             typename UIntType,
             size_t w, size_t n, size_t m, size_t r,
             UIntType a, size_t u, UIntType d, size_t s,
             UIntType b, size_t t,
             UIntType c, size_t l, UIntType f >
    inline void load(Archive & ar, std::mersenne_twister_engine<UIntType, w, n, m, r, a, u, d, s, b, t, c, l, f> & tt,
                     const unsigned int file_version)
    {
      std::string ss;
      ar & ss;
      std::istringstream is(ss);
      is >> tt;
    }

    template < class Archive,
             typename UIntType,
             size_t w, size_t n, size_t m, size_t r,
             UIntType a, size_t u, UIntType d, size_t s,
             UIntType b, size_t t,
             UIntType c, size_t l, UIntType f >
    inline void serialize(Archive & ar, std::mersenne_twister_engine<UIntType, w, n, m, r, a, u, d, s, b, t, c, l, f> & tt,
                          const unsigned int file_version)
    {
      boost::serialization::split_free(ar, tt, file_version);
    }

    template<class Archive, class Type, class Key, class Hash, class Compare, class Allocator >
    inline void save(
      Archive & ar,
      const std::unordered_map<Key, Type, Hash, Compare, Allocator> & t,
      const unsigned int /* file_version */
    )
    {
      boost::serialization::stl::save_collection <
      Archive,
      std::unordered_map<Key, Type, Hash, Compare, Allocator>
      > (ar, t);
    }

    template<class Archive, class Type, class Key, class Hash, class Compare, class Allocator >
    inline void load(
      Archive & ar,
      std::unordered_map<Key, Type, Hash, Compare, Allocator> & t,
      const unsigned int /* file_version */
    )
    {
      boost::serialization::stl::load_collection <
      Archive,
      std::unordered_map<Key, Type, Hash, Compare, Allocator>,
      boost::serialization::stl::archive_input_map <
      Archive, std::unordered_map<Key, Type, Hash, Compare, Allocator> > ,
      boost::serialization::stl::no_reserve_imp < std::unordered_map <
      Key, Type, Hash, Compare, Allocator
      >
      >
      > (ar, t);
    }

    // split non-intrusive serialization function member into separate
    // non intrusive save/load member functions
    template<class Archive, class Type, class Key, class Hash, class Compare, class Allocator >
    inline void serialize(
      Archive & ar,
      std::unordered_map<Key, Type, Hash, Compare, Allocator> & t,
      const unsigned int file_version
    )
    {
      boost::serialization::split_free(ar, t, file_version);
    }

    template<class Archive, typename T, typename Traits, typename Alloc>
    inline void save(Archive & ar, const std::basic_stringstream<T, Traits, Alloc> & t, const unsigned int /* file_version */)
    {
      std::basic_string<T, Traits, Alloc> str = t.str();
      ar & str;
    }

    template<class Archive, typename T, typename Traits, typename Alloc>
    inline void load(Archive & ar, std::basic_stringstream<T, Traits, Alloc> & t, const unsigned int /* file_version */)
    {
      std::basic_string<T, Traits, Alloc> str;
      ar & str;
      //t = std::basic_stringstream<T, Traits, Alloc>(str);
      t << str;
      t.seekg(0, std::ios_base::beg);
      t.seekp(0, std::ios_base::beg);
      t.clear();
    }

    template<class Archive, typename T, typename Traits, typename Alloc>
    inline void serialize(Archive & ar, std::basic_stringstream<T, Traits, Alloc> & t, const unsigned int file_version)
    {
      boost::serialization::split_free(ar, t, file_version);
    }


//   // multimap
//   template<class Archive, class Type, class Key, class Hash, class Compare, class Allocator >
//   inline void save(
//   Archive & ar,
//   const std::unordered_multimap<Key, Type, Hash, Compare, Allocator> &t,
//   const unsigned int /* file_version */
//   ){
//     boost::serialization::stl::save_collection<
//     Archive,
//     std::unordered_multimap<Key, Type, Hash, Compare, Allocator>
//     >(ar, t);
//   }

//   template<class Archive, class Type, class Key, class Hash, class Compare, class Allocator >
//   inline void load(
//   Archive & ar,
//   std::unordered_multimap<Key, Type, Hash, Compare, Allocator> &t,
//   const unsigned int /* file_version */
//   ){
//     boost::serialization::stl::load_collection<
//     Archive,
//     std::unordered_multimap<Key, Type, Hash, Compare, Allocator>,
//     boost::serialization::stl::archive_input_multimap<
//     Archive, std::unordered_multimap<Key, Type, Hash, Compare, Allocator>
//     >,
//     boost::serialization::stl::no_reserve_imp<
//     std::unordered_multimap<Key, Type, Hash, Compare, Allocator>
//     >
//     >(ar, t);
//   }

//   // split non-intrusive serialization function member into separate
//   // non intrusive save/load member functions
//   template<class Archive, class Type, class Key, class Hash, class Compare, class Allocator >
//   inline void serialize(
//   Archive & ar,
//   std::unordered_multimap<Key, Type, Hash, Compare, Allocator> &t,
//   const unsigned int file_version
//   ){
//     boost::serialization::split_free(ar, t, file_version);
//   }

    template<class Archive, typename T>
    inline void save(Archive & ar, const boost::polygon::point_data<T> & t, const unsigned int /* file_version */)
    {
      T x = t.x(), y = t.y();
      ar << x << y;
    }

    template<class Archive, typename T>
    inline void load(Archive & ar, boost::polygon::point_data<T> & t, const unsigned int /* file_version */)
    {
      T x, y;
      ar >> x >> y;
      t.x(x); t.y(y);
    }

    template<class Archive, typename T>
    inline void serialize(Archive & ar, boost::polygon::point_data<T> & t, const unsigned int file_version)
    {
      boost::serialization::split_free(ar, t, file_version);
    }

    template<class Archive, typename T>
    inline void save(Archive & ar, const boost::polygon::polygon_with_holes_data<T> & t, const unsigned int /* file_version */)
    {
      std::stringstream ss;
      ss << boost::geometry::wkt_manipulator<typename boost::polygon::polygon_with_holes_data<T>>(t);
      std::string str = ss.str();
      ar << str;
    }

    template<class Archive, typename T>
    inline void load(Archive & ar, boost::polygon::polygon_with_holes_data<T> & t, const unsigned int /* file_version */)
    {
      std::string str;
      ar >> str;
      boost::geometry::read_wkt(str, t);
    }

    template<class Archive, typename T>
    inline void serialize(Archive & ar, boost::polygon::polygon_with_holes_data<T> & t, const unsigned int file_version)
    {
      boost::serialization::split_free(ar, t, file_version);
    }

  }
}
#endif // SERIALISATIONUTILS_H
