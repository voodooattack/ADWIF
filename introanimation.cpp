#include "introanimation.hpp"
#include "renderer.hpp"
#include "animation.hpp"

namespace ADWIF
{
  extern const std::vector<palEntry> introPalette =
  {
    // palette
    { Black, Default, Style::Normal }, // 0
    { Black, White,   Style::Normal }, // 1
    { Black, White,   Style::Bold   }, // 2
    { Black, Cyan,    Style::Bold   }, // 3
    { Black, Cyan,    Style::Normal | Style::AltCharSet }, // 4
    { Black, Red,     Style::Bold   }, // 5
    { Black, Red,     Style::Normal }, // 6
    { Black, Yellow,  Style::Normal }, // 7
    { Black, Yellow,  Style::Bold   }, // 8
    { Black, White, Style::AltCharSet } // 9
  };

  extern const AnimationFrame copyrightText =
  {
    {
      { "Copyright (C) 2013, Abdullah Hassan ", "111111111111111111111111111111111111" },
      { "                                    ", "                                    " },
      { "    'A Song of Ice and Fire' is     ", "111111111111111111111111111111111111" },
      { " (C) 1996-2013 George R. R. Martin, ", "111111111111111111111111111111111111" },
      { "        All rights reserved.        ", "111111111111111111111111111111111111" },
      { "                                    ", "                                    " },
      { "      Press space to continue       ", "111111111111222221111111111111111111" },
    }, introPalette, 0.3 /* frame time */, 36 /* width */, 7 /* height */
  };

  extern const std::vector<AnimationFrame> introAnimation =
  {
    {
      // Frame 0 *************************************************************************
      {
        { "                                   .", "                                   5" },
        { "                                .`` ", "                                685 " },
        { "                             ..`.`  ", "                             65866  " },
        { "                          .``.``    ", "                          555656    " },
        { "  lqqqqqqqqqqqqqqqqqqqqqq~.`~~k     ", "  99999999999999999999999565769     " },
        { "  x A Dance with Ice and Fire x     ", "  9 2 22222 2222 333 222 5555 9     " },
        { "  mqqqqqqqqqqqqqqxxxqqqqqqqqqqj     ", "  99999999999999944499999999999     " },
        { "                 'xx                ", "                 444                " },
        { "                  'x                ", "                  44                " },
        { "                   '                ", "                   4                " },
        { "                                    ", "                                    " },
      }, introPalette, 0.3 /* frame time */, 36 /* width */, 11 /* height */
    },
    {
      // Frame ***************************************************************************
      {
        { "                                 `. ", "                                 86 " },
        { "                              . '`  ", "                             68675  " },
        { "                            `.,`    ", "                            6586    " },
        { "                          `.`.`     ", "                          75 656    " },
        { "  lqqqqqqqqqqqqqqqqqqqqqq`~`,`k     ", "  99999999999999999999999565569     " },
        { "  x A Dance with Ice and Fire x     ", "  9 2 22222 2222 333 222 5555 9     " },
        { "  mqqqqqqqqqqqqqqxxxqqqqqqqqqqj     ", "  99999999999999944499999999999     " },
        { "                 'xx                ", "                 444                " },
        { "                  'x                ", "                  44                " },
        { "                   '                ", "                   3                " },
        { "                                    ", "                                    " },
      }, introPalette, 0.3 /* frame time */, 36 /* width */, 11 /* height */
    },
    {
      // Frame ***************************************************************************
      {
        { "                              `~`   ", "                              755   " },
        { "                             `.`    ", "                              58    " },
        { "                            `.,`    ", "                            6576    " },
        { "                          ~.:`,     ", "                          55 656    " },
        { "  lqqqqqqqqqqqqqqqqqqqqqq~.,`~k     ", "  99999999999999999999999575869     " },
        { "  x A Dance with Ice and Fire x     ", "  9 2 22222 2222 333 222 5555 9     " },
        { "  mqqqqqqqqqqqqqqxxxqqqqqqqqqqj     ", "  99999999999999944499999999999     " },
        { "                 'xx                ", "                 444                " },
        { "                  'x                ", "                  44                " },
        { "                   '                ", "                   3                " },
        { "                                    ", "                                    " },
      }, introPalette, 0.3 /* frame time */, 36 /* width */, 11 /* height */
    },
    {
      // Frame ***************************************************************************
      {
        { "                                ,`  ", "                                67  " },
        { "                              . .`  ", "                              8678  " },
        { "                            .:,`    ", "                            6867    " },
        { "                          `.`.`     ", "                          55 656    " },
        { "  lqqqqqqqqqqqqqqqqqqqqqq`~`,`k     ", "  99999999999999999999999568569     " },
        { "  x A Dance with Ice and Fire x     ", "  9 2 22222 2222 333 222 5555 9     " },
        { "  mqqqqqqqqqqqqqqxxxqqqqqqqqqqj     ", "  99999999999999944499999999999     " },
        { "                 'xx                ", "                 444                " },
        { "                  'x                ", "                  44                " },
        { "                   '                ", "                   4                " },
        { "                   `                ", "                   3                " },
      }, introPalette, 0.3 /* frame time */, 36 /* width */, 11 /* height */
    },
    {
      // Frame ***************************************************************************
      {
        { "                                ,`  ", "                                 5  " },
        { "                              .,.`  ", "                              5765  " },
        { "                            ,.,`    ", "                            6766    " },
        { "                          `.`.`     ", "                          55 858    " },
        { "  lqqqqqqqqqqqqqqqqqqqqqq`~`,*k     ", "  99999999999999999999999585769     " },
        { "  x A Dance with Ice and Fire x     ", "  9 2 22222 2222 333 222 5555 9     " },
        { "  mqqqqqqqqqqqqqqxxxqqqqqqqqqqj     ", "  99999999999999944499999999999     " },
        { "                 'xx                ", "                 444                " },
        { "                  'x                ", "                  44                " },
        { "                   '                ", "                   4                " },
        { "                   .                ", "                   3                " },
      }, introPalette, 0.3 /* frame time */, 36 /* width */, 11 /* height */
    },
    {
      // Frame ***************************************************************************
      {
        { "                                  .,", "                                  75" },
        { "                                ',` ", "                                855 " },
        { "                             ,.~`.  ", "                             65766  " },
        { "                          .``:*`    ", "                          585676    " },
        { "  lqqqqqqqqqqqqqqqqqqqqqq~.`~~k     ", "  99999999999999999999999565569     " },
        { "  x A Dance with Ice and Fire x     ", "  9 2 22222 2222 333 222 5555 9     " },
        { "  mqqqqqqqqqqqqqqxxxqqqqqqqqqqj     ", "  99999999999999944499999999999     " },
        { "                 'xx                ", "                 444                " },
        { "                  'x                ", "                  44                " },
        { "                   '                ", "                   4                " },
        { "                                    ", "                                    " },
      }, introPalette, 0.3 /* frame time */, 36 /* width */, 11 /* height */
    },
  };
}
