cmake_minimum_required(VERSION 2.8.8)
project(QtPropertyBrowser)
enable_language(CXX) 
find_package(Qt5Widgets REQUIRED)
include_directories(${Qt5Widgets_INCLUDES})
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(KIT_SRCS
  qtbuttonpropertybrowser.cpp    qtpropertybrowserutils.cpp
  qtbuttonpropertybrowser.h      qtpropertybrowserutils_p.h
  qteditorfactory.cpp            qtpropertymanager.cpp
  qteditorfactory.h              qtpropertymanager.h
  qtgroupboxpropertybrowser.cpp  qttreepropertybrowser.cpp
  qtgroupboxpropertybrowser.h    qttreepropertybrowser.h
  qtpropertybrowser.cpp          qtvariantproperty.cpp
  qtpropertybrowser.h            qtvariantproperty.h)
qt5_add_resources(QtPropertyBrowser_RESOURCES qtpropertybrowser.qrc)
add_library(QtPropertyBrowser SHARED ${KIT_SRCS} ${QtPropertyBrowser_RESOURCES})
qt5_use_modules(QtPropertyBrowser Core Widgets)  

