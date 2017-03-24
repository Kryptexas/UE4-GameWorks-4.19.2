# Install script for directory: D:/Alembic_Github/alembic/lib/Alembic/AbcGeom

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "D:/Alembic_Github/alembic/install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/Alembic/AbcGeom" TYPE FILE FILES
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/All.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/Foundation.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/ArchiveBounds.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/IGeomBase.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/OGeomBase.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/GeometryScope.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/SchemaInfoDeclarations.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/OLight.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/ILight.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/FilmBackXformOp.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/CameraSample.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/ICamera.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/OCamera.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/Basis.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/CurveType.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/ICurves.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/OCurves.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/FaceSetExclusivity.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/OFaceSet.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/IFaceSet.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/ONuPatch.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/INuPatch.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/OGeomParam.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/IGeomParam.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/OPoints.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/IPoints.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/OPolyMesh.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/IPolyMesh.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/OSubD.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/ISubD.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/Visibility.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/XformOp.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/XformSample.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/IXform.h"
    "D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/OXform.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/Alembic_Github/alembic/lib/Alembic/AbcGeom/Tests/cmake_install.cmake")

endif()

