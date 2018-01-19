// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanConfiguration.h: Control compilation of the runtime RHI.
=============================================================================*/

// Compiled with 1.0.65.1

#pragma once

#include "VulkanCommon.h"

// API version we want to target.
#ifndef UE_VK_API_VERSION
	#define UE_VK_API_VERSION	VK_MAKE_VERSION(1, 0, 1)
#endif

// by default, we enable debugging in Development builds, unless the platform says not to
#ifndef VULKAN_SHOULD_DEBUG_IN_DEVELOPMENT
	#define VULKAN_SHOULD_DEBUG_IN_DEVELOPMENT 1
#endif

// always debug in Debug
#define VULKAN_HAS_DEBUGGING_ENABLED UE_BUILD_DEBUG || (UE_BUILD_DEVELOPMENT && VULKAN_SHOULD_DEBUG_IN_DEVELOPMENT)

// constants we probably will change a few times
#define VULKAN_UB_RING_BUFFER_SIZE								(8 * 1024 * 1024)


// Enables the VK_LAYER_LUNARG_api_dump layer and the report VK_DEBUG_REPORT_INFORMATION_BIT_EXT flag
#define VULKAN_ENABLE_API_DUMP									0

// Enables logging wrappers per Vulkan call
#ifndef VULKAN_SHOULD_ENABLE_DRAW_MARKERS
	#define VULKAN_SHOULD_ENABLE_DRAW_MARKERS					1
#endif
#ifndef VULKAN_ENABLE_DUMP_LAYER
	#define VULKAN_ENABLE_DUMP_LAYER							0
#endif
#define VULKAN_ENABLE_DRAW_MARKERS								VULKAN_SHOULD_ENABLE_DRAW_MARKERS && !VULKAN_ENABLE_DUMP_LAYER

// Keep the Vk*CreateInfo stored per object for debugging
#define VULKAN_KEEP_CREATE_INFO									0

#ifndef VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
	#define VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS			0
#endif

#define VULKAN_HASH_POOLS_WITH_TYPES_USAGE_ID					1

#ifndef VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	#define VULKAN_USE_DESCRIPTOR_POOL_MANAGER					1
#endif

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER && VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
	#error Invalid combination!
#endif

#define VULKAN_SINGLE_ALLOCATION_PER_RESOURCE					0

#ifndef VULKAN_CUSTOM_MEMORY_MANAGER_ENABLED
	#define VULKAN_CUSTOM_MEMORY_MANAGER_ENABLED				1
#endif

#ifndef VULKAN_USE_QUERY_WAIT
	#define VULKAN_USE_QUERY_WAIT								0
#endif

#ifndef VULKAN_USE_IMAGE_ACQUIRE_FENCES
	#define VULKAN_USE_IMAGE_ACQUIRE_FENCES						1
#endif

#ifndef VULKAN_HAS_PHYSICAL_DEVICE_PROPERTIES2
	#define VULKAN_HAS_PHYSICAL_DEVICE_PROPERTIES2				0
#endif

#define VULKAN_RETAIN_BUFFERS									0

#define VULKAN_USE_MSAA_RESOLVE_ATTACHMENTS						1

#define VULKAN_ENABLE_AGGRESSIVE_STATS							0

#define VULKAN_ENABLE_RHI_DEBUGGING								1

#define VULKAN_REUSE_FENCES										1


#ifndef VULKAN_ENABLE_DESKTOP_HMD_SUPPORT
	#define VULKAN_ENABLE_DESKTOP_HMD_SUPPORT					0
#endif

#ifndef VULKAN_SIGNAL_UNIMPLEMENTED
	#define VULKAN_SIGNAL_UNIMPLEMENTED()
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogVulkanRHI, Log, All);
