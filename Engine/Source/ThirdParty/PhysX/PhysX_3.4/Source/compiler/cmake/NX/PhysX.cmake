#
# Build PhysX (PROJECT not SOLUTION)
#

SET(PHYSX_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../../)

SET(PX_SOURCE_DIR ${PHYSX_SOURCE_DIR}/PhysX/src)
SET(MD_SOURCE_DIR ${PHYSX_SOURCE_DIR}/PhysXMetaData)

SET(PHYSX_PLATFORM_SRC_FILES
	${PX_SOURCE_DIR}/nx/NpMiddlewareInfo.cpp
	${PX_SOURCE_DIR}/nx/NpMiddlewareInfo.h
)

SET(PHYSX_PLATFORM_INCLUDES
	${NINTENDO_SDK_ROOT}/Include	
	${NINTENDO_SDK_ROOT}/Common/Configs/Targets/${NX_TARGET_DEVKIT}/Include
)

SET(PHYSX_COMPILE_DEFS

	# Common to all configurations
	${PHYSX_NX_COMPILE_DEFS};

	$<$<CONFIG:debug>:${PHYSX_NX_DEBUG_COMPILE_DEFS};>
	$<$<CONFIG:checked>:${PHYSX_NX_CHECKED_COMPILE_DEFS};>
	$<$<CONFIG:profile>:${PHYSX_NX_PROFILE_COMPILE_DEFS};>
	$<$<CONFIG:release>:${PHYSX_NX_RELEASE_COMPILE_DEFS};>
)

SET(PHYSX_LIBTYPE STATIC)

# include common PhysX settings
INCLUDE(../common/PhysX.cmake)

TARGET_LINK_LIBRARIES(PhysX PRIVATE LowLevel LowLevelAABB LowLevelCloth LowLevelDynamics LowLevelParticles PxTask SceneQuery SimulationController  PUBLIC PhysXCommon PxFoundation PxPvdSDK)

FILE(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/PhysX.vcxproj.user" INPUT  "${CMAKE_MODULE_PATH}/NX/Microsoft.Cpp.${NX_TARGET_DEVKIT}.user.props" CONDITION  1)
