cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules" ${CMAKE_MODULE_PATH})
include(cotire)

project(adwif)
enable_language(CXX)

set(DEP_DIR ${PROJECT_SOURCE_DIR}/deps)
set(V8_ROOT ${DEP_DIR}/v8)
set(PHYSFS_CPP_ROOT ${DEP_DIR}/physfs-cpp)

find_package(Subversion)
find_package(Git)

MESSAGE ("Looking for 7-Zip")
find_program(ZIP_UNZIP_COMMAND 7z PATHS "$ENV{ProgramFiles}/7-Zip")
if(ZIP_UNZIP_COMMAND)
  set(ZIP_UNZIP_OPTION x CACHE INTERNAL "option for unzipping")
  set(ZIP_ZIP_OPTION u -tzip -bd CACHE INTERNAL "option for zipping")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(7Zip DEFAULT_MSG ZIP_UNZIP_COMMAND)
mark_as_advanced(ZIP_UNZIP_COMMAND)

if(NOT DEFINED V8_CHECKOUT)
  #execute_process(COMMAND ${SUBVERSION_SVN_EXECUTABLE} co --force http://v8.googlecode.com/svn/trunk/ ${V8_ROOT})
  #execute_process(COMMAND make -C ${V8_ROOT} -f ${V8_ROOT}/Makefile i18nsupport=off werror=no dependencies native)
  set(V8_CHECKOUT TRUE)
endif()

if(NOT DEFINED PHYSFS_CPP_CHECKOUT)
  execute_process(COMMAND ${GIT_EXECUTABLE} clone https://github.com/kahowell/physfs-cpp/ ${PHYSFS_CPP_ROOT})
  set(PHYSFS_CPP_CHECKOUT TRUE)
endif()

add_subdirectory(${DEP_DIR})

if(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
endif(UNIX)

#execute_process(COMMAND make -C ${V8_ROOT} -f ${V8_ROOT}/Makefile i18nsupport=off werror=no native)

set(CURSES_NEED_WIDE on)
find_package(V8 REQUIRED)
find_package(curses REQUIRED)
find_package(PhysFS REQUIRED)
find_package(Threads REQUIRED)
find_package(TBB REQUIRED)
find_package(JsonCpp REQUIRED)
find_package(OpenVDB REQUIRED)
find_package(Half REQUIRED)
find_package(UTF8CPP REQUIRED)
find_package(FreeImage REQUIRED)

set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.54.0 COMPONENTS serialization filesystem system iostreams regex program_options thread chrono REQUIRED)

include_directories(${PROJECT_SOURCE_DIR} ${CURSES_INCLUDE_DIR} ${PHYSFS_INCLUDE_DIR} ${V8_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${HALF_INCLUDE_DIRS}
                    ${PHYSFS_CPP_ROOT}/include ${JSONCPP_INCLUDE_DIR} ${OPENVDB_INCLUDE_DIRS} ${TBB_INCLUDE_DIRS} ${UTF8CPP_INCLUDE_DIR}
                    ${FREEIMAGE_INCLUDE_PATH})
add_executable(adwif mapeditorstate.cpp mapgenerator.cpp mapgenstate.cpp item.cpp map.cpp fileutils.cpp jsonutils.cpp renderer.cpp animationutils.cpp util.cpp scripting.cpp game.cpp player.cpp newgamestate.cpp introanimation.cpp animation.cpp mainmenustate.cpp introstate.cpp engine.cpp curses.cpp main.cpp)
add_dependencies(adwif physfs++)
target_link_libraries(adwif ${CURSES_LIBRARIES} ${PHYSFS_LIBRARY} ${V8_LIBRARIES} ${Boost_LIBRARIES} ${HALF_LIBRARIES} ${FREEIMAGE_LIBRARIES}
                      ${CMAKE_THREAD_LIBS_INIT} ${JSONCPP_LIBRARIES} ${OPENVDB_LIBRARIES} ${TBB_LIBRARIES} physfs++)

#add_custom_command(TARGET adwif POST_BUILD COMMENT
#                     COMMAND ${CMAKE_COMMAND} -E copy_directory
#                       ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:adwif>/data)

add_custom_command(TARGET adwif PRE_BUILD
                     COMMAND ${ZIP_UNZIP_COMMAND} ${ZIP_ZIP_OPTION}
                       $<TARGET_FILE_DIR:adwif>/data.dat ${CMAKE_SOURCE_DIR}/data/*)

#cotire(adwif)

install(TARGETS adwif RUNTIME DESTINATION bin)
