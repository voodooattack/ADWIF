#include "jsonutils.hpp"
#include <boost/regex.hpp>
#include <algorithm>
#include <utf8.h>

namespace ADWIF
{
  void validateJsonSchema(const Json::Value & schema, const std::string & key,
                          const Json::Value & value) throw(ParsingException)
  {
    auto stringToType = [](const std::string & str) -> Json::ValueType {
      if (str == "array") return Json::ValueType::arrayValue;
      else if (str == "boolean") return Json::ValueType::booleanValue;
      else if (str == "integer") return Json::ValueType::intValue;
      else if (str == "uinteger") return Json::ValueType::uintValue;
      else if (str == "number") return Json::ValueType::realValue;
      else if (str == "null") return Json::ValueType::nullValue;
      else if (str == "string") return Json::ValueType::stringValue;
      else if (str == "object") return Json::ValueType::objectValue;
      else return Json::ValueType::nullValue;
    };

    auto typeToString = [](Json::ValueType type) -> std::string {
      switch(type)
      {
        case Json::ValueType::nullValue: return "null";
        case Json::ValueType::intValue: return "integer";
        case Json::ValueType::uintValue: return "uinteger";
        case Json::ValueType::realValue: return "number";
        case Json::ValueType::stringValue: return "string";
        case Json::ValueType::booleanValue: return "boolean";
        case Json::ValueType::arrayValue: return "array";
        case Json::ValueType::objectValue: return "object";
        default: return "invalid";
      }
    };

    if (value.isNull() && schema["nullable"].asBool())
      return;

    const std::string & sctype = schema["type"].asString();

    if (stringToType(sctype) != value.type())
      throw ParsingException("value type mismatch for '" + key +
                             "', expected '" + sctype + "' and got '" +
                             typeToString(value.type()) + "'\n" + value.toStyledString());

    switch(stringToType(sctype))
    {
      case Json::ValueType::nullValue:
      case Json::ValueType::intValue:
      case Json::ValueType::uintValue:
      case Json::ValueType::realValue:
      case Json::ValueType::booleanValue: return;
      case Json::ValueType::stringValue:
      {
        if (!schema["minSize"].empty() || !schema["maxSize"].empty())
        {
          std::string v = value.asString();
          size_t len = utf8::distance(v.begin(), v.end());
          if (!schema["minSize"].empty())
            if (len < schema["minSize"].asUInt())
              throw ParsingException("string '" + key + "' must have a minimum length of " +
                Json::valueToString(schema["minSize"].asUInt()));
          if (!schema["maxSize"].empty())
            if (len > schema["maxSize"].asUInt())
              throw ParsingException("string '" + key + "' must have a maximum length of " +
                Json::valueToString(schema["maxSize"].asUInt()));
        }
        if (!schema["oneOf"].empty() && schema["oneOf"].isArray())
        {
          std::vector<std::string> oneOf;
          std::transform(schema["oneOf"].begin(), schema["oneOf"].end(), std::back_inserter(oneOf),
            [&](const Json::Value & v) {
              std::string val = v.asString();
              if (!schema["caseSensetive"].empty() && !schema["caseSensetive"].asBool())
                std::transform(val.begin(), val.end(), val.begin(), &tolower);
              return val;
            });
          std::string val = value.asString();
          if (!schema["caseSensetive"].empty() && !schema["caseSensetive"].asBool())
            std::transform(val.begin(), val.end(), val.begin(), &tolower);
          if (std::find(oneOf.begin(), oneOf.end(), val) == oneOf.end())
          {
            throw ParsingException("string '" + key + "' does not match one of \n" +
              schema["oneOf"].toStyledString());
          }
        }
        if (!schema["pattern"].empty())
        {
          try
          {
            boost::regex r(schema["pattern"].asString());
            if (!boost::regex_match(value.asString(), r))
              throw ParsingException("string '" + key + "' does not match pattern \"" +
                schema["pattern"].asString() + "\"");
          }
          catch (boost::regex_error & e)
          {
            throw ParsingException("error matching pattern \"" + schema["pattern"].asString() +
              "\" to string '" + key + "': " + e.what() + " (" + Json::valueToString(e.code()) + ")");
          }
        }
        break;
      }
      case Json::ValueType::arrayValue:
      {
        for (unsigned i = 0; i < value.size(); i++)
        {
          if (!schema["items"]["minItems"].empty() && value.size() < schema["items"]["minItems"].asUInt())
          {
            throw ParsingException("array '" + key + "', requires a minimum size of " +
              Json::valueToString(schema["items"]["minItems"].asUInt()) + "\n" + value.toStyledString());
          }
          if (!schema["items"]["maxItems"].empty() && value.size() < schema["items"]["maxItems"].asUInt())
          {
            throw ParsingException("array '" + key + "', requires a maximum size of " +
              Json::valueToString(schema["items"]["maxItems"].asUInt()) + "\n" + value.toStyledString());
          }
          validateJsonSchema(schema["items"], key + "[" + Json::valueToString(i) + "]", value[i]);
        }
        break;
      }
      case Json::ValueType::objectValue:
      {
        if (!schema["pattern"].empty())
        {
          if (!schema["required"].empty() && schema["required"].isArray())
            for (auto const & reqMember : schema["required"])
              if (value[reqMember.asString()].empty())
                throw ParsingException("required property '" + reqMember.asString() + "' in object '" + key + "' is undefined " +
                  + "\n" + value.toStyledString());
          for (auto const & member : value.getMemberNames())
          {
            try
            {
              boost::regex r(schema["pattern"].asString());
              if (!boost::regex_match(member, r))
                throw ParsingException("property name for '" + key + "/" + member + "' does not match pattern \"" +
                  schema["pattern"].asString() + "\"");
            }
            catch (boost::regex_error & e)
            {
              throw ParsingException("error matching pattern \"" + schema["pattern"].asString() +
                "\" to property name '" + key + "/" + member + "': " + e.what() + " (" + Json::valueToString(e.code()) + ")");
            }
            validateJsonSchema(schema["property"], key + "/" + member, value[member]);
          }
        }
        else
          for (auto const & member : schema["properties"].getMemberNames())
          {
            if (value[member].empty() && schema["required"].isArray())
              for (auto const & reqMember : schema["required"])
                if (reqMember.asString() == member)
                  throw ParsingException("required property '" + member + "' in object '" + key + "' is undefined " +
                    + "\n" + value.toStyledString());
            if (!value[member].empty())
              validateJsonSchema(schema["properties"][member], key + "/" + member, value[member]);
          }
        break;
      }
    }
  }

  Json::Value palEntryToJson(const palEntry & entry)
  {
    Json::Value value;
    value["fgcolour"] = ADWIF::colourStr(entry.fg);
    value["bgcolour"] = ADWIF::colourStr(entry.bg);
    std::vector<std::string> styles = ADWIF::styleStrs(entry.style);
    for (auto const & s : styles)
      value["attributes"].append(s);
    return value;
  }

  Json::Value paletteToJson(const std::vector<palEntry> & palette)
  {
    Json::Value pal;
    for (auto & i : palette)
    {
      Json::Value entry = palEntryToJson(i);
      pal.append(entry);
    }
    return pal;
  }

  palEntry jsonToPalEntry(const Json::Value & value, const palEntry & def)
  {
    palEntry entry;
    if(!value["fgcolour"].empty())
      entry.fg = ADWIF::strColour(value["fgcolour"].asString());
    else
      entry.fg = def.fg;
    if(!value["bgcolour"].empty())
      entry.bg = ADWIF::strColour(value["bgcolour"].asString());
    else
      entry.bg = def.bg;
    if(!value["attributes"].empty())
    {
      std::vector<std::string> attrs;
      std::transform(value["attributes"].begin(), value["attributes"].end(),
                     std::back_inserter(attrs), [](const Json::Value & v) {
                       return v.asString();
                    });
      entry.style = strsStyle(attrs);
    }
    else
      entry.style = def.style;

    return entry;
  }
}
