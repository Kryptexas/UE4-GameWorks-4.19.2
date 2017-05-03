#
# Build PxFoundation
#

SET(PXSHARED_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../../../src)

SET(LL_SOURCE_DIR ${PXSHARED_SOURCE_DIR}/foundation)

SET(PXFOUNDATION_LIBTYPE STATIC)

SET(PXFOUNDATION_PLATFORM_FILES
	${LL_SOURCE_DIR}/src/nx/PsNXSocket.cpp
	${LL_SOURCE_DIR}/src/nx/PsNXThread.cpp
	${LL_SOURCE_DIR}/src/nx/PsNXAtomic.cpp
	${LL_SOURCE_DIR}/src/nx/PsNXCpu.cpp
	${LL_SOURCE_DIR}/src/nx/PsNXFPU.cpp
	${LL_SOURCE_DIR}/src/nx/PsNXMutex.cpp
	${LL_SOURCE_DIR}/src/nx/PsNXPrintString.cpp
	${LL_SOURCE_DIR}/src/nx/PsNXSList.cpp	
	${LL_SOURCE_DIR}/src/nx/PsNXSync.cpp	
	${LL_SOURCE_DIR}/src/nx/PsNXTime.cpp
)

SET(PXFOUNDATION_PLATFORM_INCLUDES
	${LL_SOURCE_DIR}/include/nx
	${NINTENDO_SDK_ROOT}/Include	
	${NINTENDO_SDK_ROOT}/Common/Configs/Targets/${NX_TARGET_DEVKIT}/Include
)

SET(PXFOUNDATION_COMPILE_DEFS

	# Common to all configurations
	${PHYSX_NX_COMPILE_DEFS};

	$<$<CONFIG:debug>:${PHYSX_NX_DEBUG_COMPILE_DEFS};>
	$<$<CONFIG:checked>:${PHYSX_NX_CHECKED_COMPILE_DEFS};>
	$<$<CONFIG:profile>:${PHYSX_NX_PROFILE_COMPILE_DEFS};>
	$<$<CONFIG:release>:${PHYSX_NX_RELEASE_COMPILE_DEFS};>
)

# include PxFoundation common
INCLUDE(../common/PxFoundation.cmake)

FILE(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/PxFoundation.vcxproj.user" INPUT  "${CMAKE_MODULE_PATH}/NX/Microsoft.Cpp.${NX_TARGET_DEVKIT}.user.props" CONDITION  1)