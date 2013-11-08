# - Find JSONCpp

if(JSONCPP_INCLUDE_DIR AND JSONCPP_LIBRARIES)
   set(JSONCPP_FOUND TRUE)

else(JSONCPP_INCLUDE_DIR AND JSONCPP_LIBRARIES)
  find_path(JSONCPP_INCLUDE_DIR json/json.h
      /usr/include/jsoncpp
      /usr/local/include/jsoncpp
      $ENV{SystemDrive}/jsoncpp/include
      )

  find_library(JSONCPP_LIBRARIES NAMES jsoncpp
      PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      $ENV{SystemDrive}/jsoncpp/lib
      )

  if(JSONCPP_INCLUDE_DIR AND JSONCPP_LIBRARIES)
    set(JSONCPP_FOUND TRUE)
    message(STATUS "Found JSONCpp: ${JSONCPP_INCLUDE_DIR}, ${JSONCPP_LIBRARIES}")
  else(JSONCPP_INCLUDE_DIR AND JSONCPP_LIBRARIES)
    set(JSONCPP_FOUND FALSE)
    message(STATUS "JSONCpp not found.")
  endif(JSONCPP_INCLUDE_DIR AND JSONCPP_LIBRARIES)

  mark_as_advanced(JSONCPP_INCLUDE_DIR JSONCPP_LIBRARIES)

endif(JSONCPP_INCLUDE_DIR AND JSONCPP_LIBRARIES)
