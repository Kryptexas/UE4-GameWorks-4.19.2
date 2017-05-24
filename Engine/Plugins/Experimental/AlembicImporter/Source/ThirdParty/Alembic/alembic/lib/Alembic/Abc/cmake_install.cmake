# Install script for directory: D:/Alembic_Github/alembic/lib/Alembic/Abc

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/Alembic/Abc" TYPE FILE FILES
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/All.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/Base.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/ErrorHandler.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/Foundation.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/Argument.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/ArchiveInfo.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/IArchive.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/IArrayProperty.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/IBaseProperty.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/ICompoundProperty.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/IObject.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/ISampleSelector.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/IScalarProperty.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/ISchema.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/ISchemaObject.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/ITypedArrayProperty.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/ITypedScalarProperty.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/OArchive.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/OArrayProperty.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/OBaseProperty.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/OCompoundProperty.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/OObject.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/OScalarProperty.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/OSchema.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/OSchemaObject.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/OTypedArrayProperty.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/OTypedScalarProperty.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/Reference.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/SourceName.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/TypedArraySample.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Abc/TypedPropertyTraits.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/Alembic_Github/alembic/lib/Alembic/Abc/Tests/cmake_install.cmake")

endif()

