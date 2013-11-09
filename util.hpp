#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <vector>
#include <cstdint>

namespace ADWIF
{
  std::string wrapText(const std::string & text, int bufferWidth);
  // trim from start
  std::string & ltrim(std::string & s);
  // trim from end
  std::string & rtrim(std::string & s);
  // trim from both ends
  std::string & trim(std::string & s);
  // split string
  std::vector<std::string> & split(const std::string &s, char delim, std::vector<std::string> &elems);
  // split string and return values
  std::vector<std::string> split(const std::string &s, char delim);
  // convert a colour code to its value (format: #000000)
  uint32_t hexStringToColour(const std::string & str);
}

#endif // UTIL_HPP
