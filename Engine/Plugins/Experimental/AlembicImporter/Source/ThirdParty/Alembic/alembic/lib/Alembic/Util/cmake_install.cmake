# Install script for directory: D:/Alembic_Github/alembic/lib/Alembic/Util

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/Alembic/Util" TYPE FILE FILES
    "D:/Alembic_Github/alembic/lib/Alembic/Util/Config.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Util/Digest.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Util/Dimensions.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Util/Exception.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Util/Export.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Util/Foundation.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Util/Murmur3.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Util/Naming.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Util/OperatorBool.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Util/PlainOldDataType.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Util/SpookyV2.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Util/TokenMap.h"
    "D:/Alembic_Github/alembic/lib/Alembic/Util/All.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/Alembic_Github/alembic/lib/Alembic/Util/Tests/cmake_install.cmake")

endif()

