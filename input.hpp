#ifndef INPUT_HPP
#define INPUT_HPP

#include <string>

namespace ADWIF
{
  class Input
  {
  public:
    virtual ~Input() { }
    virtual bool init() = 0;
    virtual void shutdown() = 0;
    virtual int key() = 0;
    virtual int getTimeout() const = 0;
    virtual void setTimeout(int timeout) = 0;
    virtual std::string prompt(const std::string & text, unsigned int maxLen, const std::string & suffix = std::string()) = 0;
    virtual bool promptYn(const std::string & text, bool caseSensetive) = 0;
  };

  namespace Key
  {
    extern const int Escape;
    extern const int Break;
    extern const int Backspace;
    extern const int Enter;
    extern const int Up;
    extern const int Down;
    extern const int Left;
    extern const int Right;
    extern const int Num7;
    extern const int Num8;
    extern const int Num9;
    extern const int Num4;
    extern const int Num5;
    extern const int Num6;
    extern const int Num1;
    extern const int Num2;
    extern const int Num3;
    extern const int Num0;
    extern const int Insert;
    extern const int Home;
    extern const int PgUp;
    extern const int Del;
    extern const int End;
    extern const int PgDn;
    extern const int F1;
    extern const int F2;
    extern const int F3;
    extern const int F4;
    extern const int F5;
    extern const int F6;
    extern const int F7;
    extern const int F8;
    extern const int F9;
    extern const int F10;
    extern const int F11;
    extern const int F12;
  }
}

#endif // INPUT_HPP
