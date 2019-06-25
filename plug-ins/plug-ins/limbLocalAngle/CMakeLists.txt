cmake_minimum_required(VERSION 2.8)

include($ENV{DEVKIT_LOCATION}/cmake/pluginEntry.cmake)

set(PROJECT_NAME "armLocalAngle")
set(SOURCE_FILES armLocalAngle.cpp)
set(LIBRARIES OpenMaya Foundation)

build_plugin()