cmake_minimum_required(VERSION 2.8)

include($ENV{DEVKIT_LOCATION}/cmake/pluginEntry.cmake)

set(PROJECT_NAME "limbLocalAngle")
set(SOURCE_FILES limbLocalAngle.cpp)
set(LIBRARIES OpenMaya Foundation)

build_plugin()