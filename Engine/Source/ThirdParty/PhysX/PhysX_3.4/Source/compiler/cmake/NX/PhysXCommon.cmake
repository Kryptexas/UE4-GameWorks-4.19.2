#
# Build PhysXCommon
#

SET(PHYSX_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../../)

SET(COMMON_SRC_DIR ${PHYSX_SOURCE_DIR}/Common/src)
SET(GU_SOURCE_DIR ${PHYSX_SOURCE_DIR}/GeomUtils)

SET(PHYSXCOMMON_LIBTYPE STATIC)

SET(PXCOMMON_PLATFORM_INCLUDES
	${NINTENDO_SDK_ROOT}/Include	
	${NINTENDO_SDK_ROOT}/Common/Configs/Targets/${NX_TARGET_DEVKIT}/Include
)


# Use generator expressions to set config specific preprocessor definitions
SET(PXCOMMON_COMPILE_DEFS

	# Common to all configurations
	${PHYSX_NX_COMPILE_DEFS};

	$<$<CONFIG:debug>:${PHYSX_NX_DEBUG_COMPILE_DEFS};>
	$<$<CONFIG:checked>:${PHYSX_NX_CHECKED_COMPILE_DEFS};>
	$<$<CONFIG:profile>:${PHYSX_NX_PROFILE_COMPILE_DEFS};>
	$<$<CONFIG:release>:${PHYSX_NX_RELEASE_COMPILE_DEFS};>
)

# include common PhysXCommon settings
INCLUDE(../common/PhysXCommon.cmake)

TARGET_LINK_LIBRARIES(PhysXCommon PUBLIC PxFoundation)

FILE(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/PhysXCommon.vcxproj.user" INPUT  "${CMAKE_MODULE_PATH}/NX/Microsoft.Cpp.${NX_TARGET_DEVKIT}.user.props" CONDITION  1)





