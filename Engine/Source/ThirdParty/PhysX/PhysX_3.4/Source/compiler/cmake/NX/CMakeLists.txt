cmake_minimum_required(VERSION 3.3)
include(../common/CMakeLists.txt)


IF(NOT DEFINED TARGET_BUILD_PLATFORM) # Not defined, default to NX
	SET(TARGET_BUILD_PLATFORM "NX")
ENDIF()

SET(PLATFORM_LIST NX)

IF (NOT ${TARGET_BUILD_PLATFORM} IN_LIST PLATFORM_LIST)
	MESSAGE(FATAL_ERROR "Invalid platform:" ${TARGET_BUILD_PLATFORM})
ENDIF()

SET(NINTENDO_SDK_ROOT $ENV{NINTENDO_SDK_ROOT})

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -fstrict-aliasing -fno-omit-frame-pointer -fno-vectorize -fno-exceptions  -fno-builtin -fno-rtti  -funsigned-char  -mno-implicit-float")
	
SET(CMAKE_CXX_FLAGS_DEBUG "-fno-inline-functions -O0 -g")
SET(CMAKE_CXX_FLAGS_CHECKED "-O3 -g")
SET(CMAKE_CXX_FLAGS_PROFILE "-O3 -g")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -g")

SET(NX_CONFIG Release)

SET(PHYSX_NX_COMPILE_DEFS PX_PHYSX_STATIC_LIB)
SET(PHYSX_NX_DEBUG_COMPILE_DEFS _DEBUG;PX_DEBUG=1;PX_CHECKED=1;PX_SUPPORT_PVD=1;NN_SDK_BUILD_DEBUG)
SET(PHYSX_NX_CHECKED_COMPILE_DEFS NDEBUG;PX_CHECKED=1;PX_SUPPORT_PVD=1;NN_SDK_BUILD_RELEASE)
SET(PHYSX_NX_PROFILE_COMPILE_DEFS NDEBUG;PX_PROFILE=1;PX_SUPPORT_PVD=1;NN_SDK_BUILD_RELEASE)
SET(PHYSX_NX_RELEASE_COMPILE_DEFS NDEBUG;PX_SUPPORT_PVD=0;NN_SDK_BUILD_RELEASE)

# Include the PxShared-exports.cmake file that PxShared created. There's other ways to do this, but there's a bunch of issues with it in a CI environment
#  such as - we don't want to be installing any built artifacts on this server, etc. So for now we hack it.

# Add PxShared as a dependency so that we can use project references
ADD_SUBDIRECTORY(${PXSHARED_ROOT_DIR}/src/compiler/cmake/NX "${CMAKE_CURRENT_BINARY_DIR}/pxshared_bin")

# Include all of the projects
INCLUDE(LowLevel.cmake)
INCLUDE(LowLevelAABB.cmake)
INCLUDE(LowLevelCloth.cmake)
INCLUDE(LowLevelDynamics.cmake)
INCLUDE(LowLevelParticles.cmake)
INCLUDE(PhysX.cmake)
INCLUDE(PhysXCharacterKinematic.cmake)
INCLUDE(PhysXCommon.cmake)
INCLUDE(PhysXCooking.cmake)
INCLUDE(PhysXExtensions.cmake)
INCLUDE(PhysXVehicle.cmake)
INCLUDE(SceneQuery.cmake)
INCLUDE(SimulationController.cmake)
