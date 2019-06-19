#-
# ==========================================================================
# Copyright (c) 2018 Autodesk, Inc.
# All rights reserved.
# 
# These coded instructions, statements, and computer programs contain
# unpublished proprietary information written by Autodesk, Inc., and are
# protected by Federal copyright law. They may not be disclosed to third
# parties or copied or duplicated in any form, in whole or in part, without
# the prior written consent of Autodesk, Inc.
# ==========================================================================
#+


cmake_minimum_required(VERSION 2.8)

# include the project setting file
include($ENV{DEVKIT_LOCATION}/cmake/pluginEntry.cmake)

# specify project name
set(PROJECT_NAME AbcArnold)


# set SOURCE_FILES
set(SOURCE_FILES
   ArbGeomParams.cpp ProcArgs.cpp SampleUtil.cpp PathUtil.cpp ProcMain.cpp WriteGeo.cpp
   ArbGeomParams.h  PathUtil.h  ProcArgs.h  SampleUtil.h  WriteGeo.h
)


# find z
find_zlib()

# find Alembic
find_alembic()

# find Arnold
find_arnold()

# Build plugin
build_plugin()

