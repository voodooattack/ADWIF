#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include <FreeImagePlus.h>
#include <string>

namespace ADWIF
{

  fipImage & loadImageFromPhysFS(const std::string & path, fipImage & image);
  fipImage & loadImageFromStream(std::istream & stream, fipImage & image);
}

#endif // IMAGEUTILS_H
