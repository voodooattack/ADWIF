cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules" ${CMAKE_MODULE_PATH})

include(cotire)
include(GetGitRevisionDescription)

get_git_head_revision(ADWIF_GIT_BRANCH ADWIF_GIT_REVISION)

project(adwif)
enable_language(CXX)

set(ADWIF_SOURCES imageutils.cpp mapbank.cpp mapgenerator.cpp mapgenstate.cpp item.cpp map.cpp fileutils.cpp
                  jsonutils.cpp renderer.cpp animationutils.cpp util.cpp scripting.cpp game.cpp player.cpp
                  newgamestate.cpp introanimation.cpp animation.cpp mainmenustate.cpp introstate.cpp engine.cpp
                  main.cpp)

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
  set(V8_CHECKOUT TRUE CACHE INTERNAL "whether or not Google V8 was checked out")
endif()

if(NOT DEFINED PHYSFS_CPP_CHECKOUT)
  execute_process(COMMAND ${GIT_EXECUTABLE} clone https://github.com/kahowell/physfs-cpp/ ${PHYSFS_CPP_ROOT})
  set(PHYSFS_CPP_CHECKOUT TRUE CACHE INTERNAL "whether or not physfscpp was checked out")
endif()

add_subdirectory(${DEP_DIR})

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "-std=gnu++11")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "-std=c++11")
elseif(MSVC)
  # On by default
endif()

#execute_process(COMMAND make -C ${V8_ROOT} -f ${V8_ROOT}/Makefile i18nsupport=off werror=no native)

set(ADWIF_RENDERER "curses" CACHE STRING
  "Select a rendering backend, valid values are 'curses' and 'tcod'")
set_property(CACHE ADWIF_RENDERER PROPERTY STRINGS
              "curses" "tcod")

set(Boost_USE_MULTITHREADED ON)

if(ADWIF_RENDERER STREQUAL "curses")
  set(CURSES_NEED_WIDE on)
  find_package(curses REQUIRED)
  set(ADWIF_RENDERER_USE_CURSES True)
  set(ADWIF_RENDERER_SOURCES "curses.cpp")
  set(ADWIF_RENDERER_INCLUDES ${CURSES_INCLUDE_DIR})
  set(ADWIF_RENDERER_LIBRARIES ${CURSES_LIBRARIES})
elseif(ADWIF_RENDERER STREQUAL "tcod")
  find_package(TCOD REQUIRED)
  set(ADWIF_RENDERER_USE_TCOD True)
  set(ADWIF_RENDERER_SOURCES "tcod.cpp")
  set(ADWIF_RENDERER_INCLUDES ${LIBTCOD_INCLUDE_DIRS})
  set(ADWIF_RENDERER_LIBRARIES ${LIBTCOD_LIBRARIES})
endif()

option(ADWIF_UNICODE "whether or not to use unicode output" ON)

if(ADWIF_UNICODE)
  find_package(UTF8CPP REQUIRED)
endif()

find_package(V8 REQUIRED)
find_package(PhysFS REQUIRED)
find_package(Threads REQUIRED)
find_package(TBB REQUIRED)
find_package(JsonCpp REQUIRED)
find_package(OpenVDB REQUIRED)
find_package(Half REQUIRED)
find_package(FreeImage REQUIRED)
find_package(Noise REQUIRED)
#find_package(Eigen3 REQUIRED)
find_package(Boost 1.54.0 COMPONENTS serialization coroutine context filesystem system iostreams regex program_options thread chrono REQUIRED)

option(ADWIF_BUILD_EDITOR "whether or not to build the game editor" OFF)

if (ADWIF_BUILD_EDITOR)
  set(EDITOR_DIR "editor")
  include_directories(${EDITOR_DIR})
  find_package(Qt COMPONENTS QtCore QtGui QtOpenGL REQUIRED)
  find_package(OpenGL REQUIRED)
  include(${QT_USE_FILE})
  set(EDITOR_INCLUDES ${QT_INCLUDES})
  set(EDITOR_LIBRARIES ${QT_LIBRARIES} ${QT_QTOPENGL_LIBRARIES} ${OPENGL_LIBRARIES})
  set(EDITOR_UI ${EDITOR_DIR}/editor.ui)
  set(EDITOR_SOURCES ${EDITOR_DIR}/editor.cpp)
  set(EDITOR_RESOURCES ${EDITOR_DIR}/resources.qrc)
  qt4_wrap_ui(EDITOR_UI_HEADERS ${EDITOR_UI})
  set(EDITOR_HEADERS ${EDITOR_DIR}/editor.hpp ${EDITOR_UI_HEADERS})
  qt4_wrap_cpp(EDITOR_MOC ${EDITOR_HEADERS})
  qt4_add_resources(EDITOR_RC ${EDITOR_RESOURCES})
  set(ADWIF_SOURCES ${ADWIF_SOURCES} ${EDITOR_SOURCES} ${EDITOR_MOC} ${EDITOR_RC} editorstate.cpp)
  include(ExternalProject)
  ExternalProject_Add(QtPropertyBrowserClone
                      GIT_REPOSITORY https://github.com/commontk/QtPropertyBrowser.git
                      SOURCE_DIR ${PROJECT_BINARY_DIR}/QtPropertyBrowserSrc
                      BINARY_DIR ${PROJECT_BINARY_DIR}/QtPropertyBrowser
                      INSTALL_COMMAND "")
  include_directories(${PROJECT_BINARY_DIR}/QtPropertyBrowserSrc/src/)
  link_directories(${PROJECT_BINARY_DIR}/QtPropertyBrowser/bin/)
endif()

# get_cmake_property(_variableNames VARIABLES)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()

configure_file ("${PROJECT_SOURCE_DIR}/config.hpp.in" "${PROJECT_BINARY_DIR}/config.hpp")

include_directories(${PROJECT_SOURCE_DIR} ${EDITOR_DIR} ${PROJECT_BINARY_DIR} ${EDITOR_INCLUDES}
                    ${ADWIF_RENDERER_INCLUDES} ${PHYSFS_INCLUDE_DIR} ${V8_INCLUDE_DIR}
                    ${Boost_INCLUDE_DIRS} ${HALF_INCLUDE_DIRS} ${PHYSFS_CPP_ROOT}/include ${JSONCPP_INCLUDE_DIR} ${OPENVDB_INCLUDE_DIRS}
                    ${TBB_INCLUDE_DIRS} ${UTF8CPP_INCLUDE_DIR} ${FREEIMAGE_INCLUDE_PATH} ${NOISE_INCLUDE_DIR} ${EIGEN3_INCLUDE_DIR}
                    ${QtPropertyBrowser_INCLUDE_DIRS})
add_executable(adwif ${ADWIF_RENDERER_SOURCES} ${ADWIF_SOURCES})
add_dependencies(adwif physfs++)

if (ADWIF_BUILD_EDITOR)
  add_dependencies(adwif QtPropertyBrowserClone)
  target_link_libraries(adwif QtPropertyBrowser)
endif()

target_link_libraries(adwif ${EDITOR_LIBRARIES} ${ADWIF_RENDERER_LIBRARIES} ${PHYSFS_LIBRARY} ${V8_LIBRARIES} ${Boost_LIBRARIES} ${HALF_LIBRARIES}
                      ${FREEIMAGE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${JSONCPP_LIBRARIES} ${OPENVDB_LIBRARIES} ${TBB_LIBRARIES} ${NOISE_LIBRARY}
                      physfs++)

#add_custom_command(TARGET adwif POST_BUILD COMMENT
#                     COMMAND ${CMAKE_COMMAND} -E copy_directory
#                       ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:adwif>/data)

add_custom_command(TARGET adwif PRE_BUILD
                     COMMAND ${ZIP_UNZIP_COMMAND} ${ZIP_ZIP_OPTION}
                       $<TARGET_FILE_DIR:adwif>/data.dat ${CMAKE_SOURCE_DIR}/data/*)

#cotire(adwif)

install(TARGETS adwif RUNTIME DESTINATION bin)
