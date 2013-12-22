cmake_minimum_required(VERSION 2.8.8)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules" ${CMAKE_MODULE_PATH})

include(cotire)
include(GetGitRevisionDescription)

get_git_head_revision(ADWIF_GIT_BRANCH ADWIF_GIT_REVISION)

project(adwif)
enable_language(C CXX)

set(ADWIF_SOURCES noisemodules.cpp noiseutils.cpp imageutils.cpp mapgenerator.cpp mapgenstate.cpp item.cpp
                  fileutils.cpp jsonutils.cpp renderer.cpp animationutils.cpp util.cpp scripting.cpp game.cpp
                  player.cpp newgamestate.cpp introanimation.cpp animation.cpp mainmenustate.cpp introstate.cpp
                  engine.cpp main.cpp)

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
  set(PHYSFS_CPP_CHECKOUT TRUE CACHE INTERNAL "whether or not physfs-cpp was checked out")
endif()

add_subdirectory(${DEP_DIR})

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "-std=gnu++11")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libstdc++")
elseif(MSVC)
  # On by default
endif()

#execute_process(COMMAND make -C ${V8_ROOT} -f ${V8_ROOT}/Makefile i18nsupport=off werror=no native)

set(ADWIF_MAP_ENGINE "OpenVDB" CACHE STRING
  "Select a mapping backend, valid values are 'OpenVDB' and 'Field3D'")
set_property(CACHE ADWIF_MAP_ENGINE PROPERTY STRINGS
  "OpenVDB" "Field3D")

if(ADWIF_MAP_ENGINE STREQUAL "OpenVDB")
  find_package(OpenVDB REQUIRED)
  find_package(Half)
  find_package(TBB)
  set(ADWIF_SOURCES ${ADWIF_SOURCES} map_openvdb.cpp mapbank.cpp)
  set(ADWIF_MAP_INCLUDES ${OPENVDB_INCLUDE_DIRS} ${TBB_INCLUDE_DIRS})
  set(ADWIF_MAP_LIBRARIES ${OPENVDB_LIBRARIES} ${TBB_LIBRARIES} ${HALF_LIBRARIES})
elseif(ADWIF_MAP_ENGINE STREQUAL "Field3D")
  find_package(Field3D REQUIRED)
  find_package(ILMBase REQUIRED)
  find_package(HDF5 REQUIRED)
  set(ADWIF_SOURCES ${ADWIF_SOURCES} map_field3d.cpp mapbank.cpp)
  set(ADWIF_MAP_INCLUDES ${FIELD3D_INCLUDE_DIRS} ${ILMBASE_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})
  set(ADWIF_MAP_LIBRARIES ${FIELD3D_LIBRARIES} ${HDF5_LIBRARIES})
endif()

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
find_package(JsonCpp REQUIRED)
find_package(FreeImage REQUIRED)
find_package(Noise REQUIRED)
#find_package(Eigen3 REQUIRED)
find_package(Boost 1.54.0 COMPONENTS serialization coroutine context filesystem system iostreams regex program_options thread chrono REQUIRED)
find_package(Threads REQUIRED)

option(ADWIF_BUILD_EDITOR "whether or not to build the game editor" OFF)

if (ADWIF_BUILD_EDITOR)
  set(EDITOR_DIR "editor")
  include_directories(${EDITOR_DIR})
  set(CMAKE_AUTOMOC ON)
  find_package(Qt5Widgets REQUIRED)
  find_package(Qt5OpenGL REQUIRED)
  find_package(OpenGL REQUIRED)
  set(EDITOR_UI editor/editor.ui editor/heightmapeditor.ui)
  set(EDITOR_SOURCES editor/editor.cpp editor/heightmapeditor.cpp editor/noisegraphbuilder.cpp
                     editor/extendedvariantmanager.cpp editor/extendedvarianteditorfactory.cpp
    editor/propertybrowserbox.cpp editor/curveeditor.cpp editor/scrollablegraphicsview.cpp)
  set(EDITOR_HEADERS editor/editor.hpp editor/heightmapeditor.hpp editor/noisegraphbuilder.hpp
                     editor/extendedvariantmanager.hpp editor/extendedvarianteditorfactory.hpp
                     editor/propertybrowserbox.hpp editor/curveeditor.hpp editor/scrollablegraphicsview.hpp)
  set(EDITOR_RESOURCES editor/resources.qrc)
  qt5_wrap_ui(EDITOR_UI_SRC ${EDITOR_UI})
  qt5_add_resources(EDITOR_RC ${EDITOR_RESOURCES})
  set(ADWIF_SOURCES ${EDITOR_UI_SRC} ${ADWIF_SOURCES} ${EDITOR_SOURCES} ${EDITOR_RC} editorstate.cpp)
  include_directories(${DEP_DIR}/QtPropertyBrowser)
endif()

# get_cmake_property(_variableNames VARIABLES)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()

configure_file ("${PROJECT_SOURCE_DIR}/config.hpp.in" "${PROJECT_BINARY_DIR}/config.hpp")

include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR} ${EDITOR_INCLUDES} ${ADWIF_MAP_INCLUDES}
                    ${ADWIF_RENDERER_INCLUDES} ${PHYSFS_INCLUDE_DIR} ${V8_INCLUDE_DIR}
                    ${Boost_INCLUDE_DIRS} ${HALF_INCLUDE_DIRS} ${PHYSFS_CPP_ROOT}/include ${JSONCPP_INCLUDE_DIR}
                    ${UTF8CPP_INCLUDE_DIR} ${FREEIMAGE_INCLUDE_PATH} ${NOISE_INCLUDE_DIR} ${EIGEN3_INCLUDE_DIR})
add_executable(adwif ${ADWIF_RENDERER_SOURCES} ${ADWIF_SOURCES})
add_dependencies(adwif physfs++)

if (ADWIF_BUILD_EDITOR)
  add_dependencies(adwif QtPropertyBrowser)
  target_link_libraries(adwif QtPropertyBrowser)
  qt5_use_modules(adwif Core Widgets OpenGL)
endif()

target_link_libraries(adwif ${EDITOR_LIBRARIES} ${ADWIF_RENDERER_LIBRARIES} ${ADWIF_MAP_LIBRARIES} ${PHYSFS_LIBRARY}
                      ${V8_LIBRARIES} ${Boost_LIBRARIES} ${FREEIMAGE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${JSONCPP_LIBRARIES}
                      ${NOISE_LIBRARY} physfs++)

#add_custom_command(TARGET adwif POST_BUILD COMMENT
#                     COMMAND ${CMAKE_COMMAND} -E copy_directory
#                       ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:adwif>/data)

add_custom_command(TARGET adwif PRE_BUILD
                     COMMAND ${ZIP_UNZIP_COMMAND} ${ZIP_ZIP_OPTION}
                       $<TARGET_FILE_DIR:adwif>/data.dat ${CMAKE_SOURCE_DIR}/data/*)

#cotire(adwif)

install(TARGETS adwif RUNTIME DESTINATION bin)
