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

#include "util.hpp"

#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <string>
#include <sstream>

namespace ADWIF
{
  std::string wrapText(const std::string & text, int bufferWidth)
  {
    // http://arodgersblog.wordpress.com/2012/08/09/c-word-wrap-for-console-output-tutorial/
    std::string s = text;
    for (unsigned int i = 1; i <= s.length() ; i++)
    {
      char c = s[i - 1];
      int spaceCount = 0;

      // Add whitespace if newline detected.
      if (c == '\n')
      {
        int charNumOnLine = ((i) % bufferWidth);
        spaceCount = bufferWidth - charNumOnLine;
        s.insert((i - 1), (spaceCount), ' ');
        i += (spaceCount);
        continue;
      }

      if ((i % bufferWidth) == 0)
        if (c != ' ')
          for (int j = (i - 1); j > -1 ; j--)
            if (s[j] == ' ')
            {
              s.insert(j, spaceCount, ' ');
              break;
            }
            else spaceCount++;
    }

    return s;
  }


  std::string & ltrim(std::string & s)
  {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
  }

  std::string & rtrim(std::string & s)
  {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
  }

  std::string & trim(std::string & s)
  {
    return ltrim(rtrim(s));
  }

  std::vector< std::string > & split(const std::string & s, char delim, std::vector< std::string > & elems)
  {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
      elems.push_back(item);
    }
    return elems;
  }

  std::vector< std::string > split(const std::string & s, char delim)
  {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
  }

  uint32_t hexStringToColour(const std::string & str)
  {
    if (str.empty() || str.size() != 7 || str[0] != '#')
      return 0;
    std::string copy;
    std::transform(str.begin()+1, str.end(), std::back_inserter(copy), &toupper);
    //copy = "0xFF" + copy;
    uint32_t col;
    std::stringstream ss;
    ss << copy;
    ss >> std::hex >> col;
    return col;
  }
}
