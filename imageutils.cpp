#include "imageutils.hpp"

#include <exception>
#include <stdexcept>
#include <physfs.hpp>

namespace ADWIF
{
  fipImage & loadImageFromPhysFS(const std::string & path, fipImage & image)
  {
    PHYSFS_File * file = PHYSFS_openRead(path.c_str());
    FreeImageIO io = {
      [](void *buffer, unsigned size, unsigned count, fi_handle handle) -> unsigned {
        return PHYSFS_read((PHYSFS_File*)handle, buffer, size, count);
      },
      [](void *buffer, unsigned size, unsigned count, fi_handle handle) -> unsigned {
        return PHYSFS_write((PHYSFS_File*)handle, buffer, size, count);
      },
      [](fi_handle handle, long offset, int origin) -> int {
        long org = 0;
        switch(origin)
        {
          case SEEK_END: org = PHYSFS_fileLength((PHYSFS_File*)handle); break;
          case SEEK_CUR: org = PHYSFS_tell((PHYSFS_File*)handle); break;
          case SEEK_SET:
          default: org = 0; break;
        }
        return PHYSFS_seek((PHYSFS_File*)handle, org + offset);
      },
      [](fi_handle handle) -> long {
        return PHYSFS_tell((PHYSFS_File*)handle);
      }
    };

    if (!image.loadFromHandle(&io, reinterpret_cast<fi_handle>(file), 0))
    {
      PHYSFS_close(file);
      throw std::runtime_error("error loading image from stream");
    }
    else
    {
      PHYSFS_close(file);
      return image;
    }
  }

  fipImage & loadImageFromStream(std::istream & stream, fipImage & image)
  {
    FreeImageIO io = {
      [](void *buffer, unsigned size, unsigned count, fi_handle handle) -> unsigned {
        reinterpret_cast<std::istream*>(handle)->read(
          (char*)buffer, count);
        if (reinterpret_cast<std::istream*>(handle)->good()) return reinterpret_cast<std::istream*>(handle)->gcount(); else return 0;
      },
      [](void *buffer, unsigned size, unsigned count, fi_handle handle) -> unsigned { return 1; },
      [](fi_handle handle, long offset, int origin) -> int {
        std::ios_base::seekdir dir;
        switch(origin)
        {
          case SEEK_END: dir = std::ios_base::end; break;
          case SEEK_CUR: dir = std::ios_base::cur; break;
          case SEEK_SET:
          default: dir = std::ios_base::beg; break;
        }
        reinterpret_cast<std::istream*>(handle)->seekg(offset, dir);
        return reinterpret_cast<std::istream*>(handle)->good() ? 0 : 1;
      },
      [](fi_handle handle) -> long {
        return reinterpret_cast<std::istream*>(handle)->tellg();
      }
    };
    if (image.loadFromHandle(&io, reinterpret_cast<fi_handle>(&stream), 0))
      return image;
    else
      throw std::runtime_error("error loading image from stream");
  }

}
