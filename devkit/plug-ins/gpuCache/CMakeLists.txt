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
include($ENV{DEVKIT_LOCATION}/cmake/gpuCache.cmake)

# specify project name
set(PROJECT_NAME gpuCache)

# set MEL files
set(MEL_FILES 
	  AEgpuCacheRelated.mel 
      AEgpuCacheTemplate.mel
      gpuCacheInitStrings.mel
      gpuCacheCreateUI.mel
      gpuCacheListModelEditorPanels.mel
      performGpuCacheImport.mel
      performGpuCacheExport.mel
      doGpuCacheImport.mel
      doGpuCacheImportArgList.mel
      doGpuCacheExportArgList.mel
    )

# set SOURCE_FILES
set(SOURCE_FILES
	gpuCacheGLFT.cpp
	gpuCacheConfig.cpp
	gpuCacheVramQuery.cpp
	gpuCacheVBOProxy.cpp 
	gpuCacheSample.cpp 
	gpuCacheGeometry.cpp 
	gpuCacheFrustum.cpp
	gpuCacheDrawTraversal.cpp
	gpuCacheMaterial.cpp
	gpuCacheMaterialNodes.cpp
	gpuCacheMaterialBakers.cpp
	gpuCacheUnitBoundingBox.cpp

	gpuCacheSelect.cpp
	gpuCacheRasterSelect.cpp
	gpuCacheGLPickingSelect.cpp

	gpuCacheShapeNode.cpp 
	gpuCacheDrawOverride.cpp 
	gpuCacheSubSceneOverride.cpp 

	gpuCacheSpatialSubdivision.cpp
	gpuCacheSpatialGrid.cpp
	gpuCacheSpatialGridWalker.cpp
	gpuCacheIsectUtil.cpp

	gpuCacheUtil.cpp
	gpuCacheCmd.cpp 

	CacheAlembicUtil.cpp
	CacheWriter.cpp 
	CacheWriterAlembic.cpp 
	CacheReader.cpp 
	CacheReaderAlembic.cpp

	gpuCachePluginMain.cpp

	gpuCacheStrings.h
	gpuCacheGLFT.h
	gpuCacheConfig.h
	gpuCacheVramQuery.h
	gpuCacheVBOProxy.h 
	gpuCacheSample.h 
	gpuCacheGeometry.h 
	gpuCacheFrustum.h
	gpuCacheDrawTraversal.h
	gpuCacheMaterial.h
	gpuCacheMaterialNodes.h
	gpuCacheMaterialBakers.h
	gpuCacheUnitBoundingBox.h
	gpuCacheSelect.h
	gpuCacheRasterSelect.h
	gpuCacheGLPickingSelect.h

	gpuCacheShapeNode.h 
	gpuCacheDrawOverride.h 
	gpuCacheSubSceneOverride.h 

	gpuCacheSpatialSubdivision.h
	gpuCacheSpatialGrid.h
	gpuCacheSpatialGridWalker.h
	gpuCacheIsectUtil.h

	gpuCacheUtil.h
	gpuCacheCmd.h
	gpuCacheTimeInterval.h

	CacheAlembicUtil.h 
	CacheWriter.h 
	CacheWriterAlembic.h 
	CacheReader.h 
	CacheReaderAlembic.h
	
	${MEL_FILES}
)

# set linking libraries
set(LIBRARIES
     OpenMaya OpenMayaUI OpenMayaAnim OpenMayaRender Foundation
)

# set linking package libraries
if (WIN32)
	set(PACKAGE_LIBS ${PACKAGE_LIBS} wbemuuid Ole32 OleAut32)
endif()

# find z
find_zlib()

# find Alembic
find_alembic()

# find OpenGL
find_opengl()

# find TBB
find_tbb()

# find boost
find_boost()

# Build plugin
build_plugin()

