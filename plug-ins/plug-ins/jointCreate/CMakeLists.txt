cmake_minimum_required(VERSION 2.8)

include($ENV{DEVKIT_LOCATION}/cmake/pluginEntry.cmake)

set(PROJECT_NAME "jointCreate")
set(SOURCE_FILES jointCreate.cpp)
set(LIBRARIES
        OpenMaya
        OpenMayaAnim
        Foundation)

build_plugin()