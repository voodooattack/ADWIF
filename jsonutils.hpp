#ifndef JSONUTILS_H
#define JSONUTILS_H

#include <string>
#include <vector>
#include <exception>
#include <json/json.h>

#include "renderer.hpp"
#include "animation.hpp"

namespace ADWIF
{
  class ParsingException: public std::runtime_error
  {
  public:
    explicit ParsingException(const std::string & arg): runtime_error(arg) { }
    virtual ~ParsingException() { }
  };

  void validateJsonSchema(const Json::Value & schema, const std::string & key,
                      const Json::Value & value) throw(ParsingException);

  Json::Value palEntryToJson(const palEntry & entry);
  Json::Value paletteToJson(const std::vector<palEntry> & palette);

  palEntry jsonToPalEntry(const Json::Value & value, const palEntry & def = palEntry());
}

#endif // JSONUTILS_H
