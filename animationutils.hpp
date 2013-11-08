#ifndef ANIMATIONUTILS_H
#define ANIMATIONUTILS_H

#include "animation.hpp"

#include <string>
#include <vector>

namespace ADWIF
{
  void generateMenu(std::vector<AnimationFrame> & menu,
                    const std::vector<std::string> & entries,
                    const std::vector<palEntry> & palette,
                    int active,
                    int dormant,
                    bool alignRight = false,
                    unsigned int maxHeight = 0,
                    const std::vector<void*> tags = std::vector<void*>(),
                    const std::vector<bool> & enabled = std::vector<bool>(),
                    int disabled = 0,
                    int disabledHighlight = 0,
                    int charUp = '^',
                    int charDn = 'v',
                    int arrowP = 0
                   );

}

#endif // ANIMATIONUTILS_H
