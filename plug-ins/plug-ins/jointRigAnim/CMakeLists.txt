cmake_minimum_required(VERSION 2.8)

include($ENV{DEVKIT_LOCATION}/cmake/pluginEntry.cmake)

set(PROJECT_NAME "jointRigAnim")
set(MEL_FILES jointRigAnim.MEL)
set(SOURCE_FILES
        jointRigAnim.cpp
        $ENV{MEL_FILES})
set(LIBRARIES
        OpenMaya
        OpenMayaAnim
        Foundation)

build_plugin()