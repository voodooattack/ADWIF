# - Try to find LIBTCOD
# Once done this will define
#  LIBTCOD_FOUND - System has LIBTCOD
#  LIBTCOD_INCLUDE_DIRS - The LIBTCOD include directories
#  LIBTCOD_LIBRARIES - The libraries needed to use LIBTCOD
#  LIBTCOD_DEFINITIONS - Compiler switches required for using LIBTCOD

find_path(LIBTCOD_INCLUDE_DIR libtcod.h
          HINTS ${DEP_DIR}/libtcod/ ${DEP_DIR}/libtcod/include )

find_library(LIBTCOD_LIBRARY NAMES libtcod tcod
             HINTS ${DEP_DIR}/libtcod/ )
find_library(LIBTCODXX_LIBRARY NAMES libtcodxx tcodxx
             HINTS ${DEP_DIR}/libtcod/ )

set(LIBTCOD_LIBRARIES ${LIBTCOD_LIBRARY} ${LIBTCODXX_LIBRARY} )
set(LIBTCOD_INCLUDE_DIRS ${LIBTCOD_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBXML2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LIBTCOD DEFAULT_MSG
                                  LIBTCOD_LIBRARY LIBTCOD_INCLUDE_DIR)

mark_as_advanced(LIBTCOD_INCLUDE_DIR LIBTCOD_LIBRARY)
