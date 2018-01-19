// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

// moved the setup from VulkanRHIPrivate.h to the platform headers
#include "WindowsHWrapper.h"

#define VK_USE_PLATFORM_WIN32_KHR				1
#define VK_USE_PLATFORM_WIN32_KHX				1

//#define VULKAN_USE_NEW_QUERIES					1
//#define VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS	1
//#define VULKAN_USE_DESCRIPTOR_POOL_MANAGER			0


#define	VULKAN_CUSTOM_MEMORY_MANAGER_ENABLED	(PLATFORM_64BITS)
#define VULKAN_HAS_PHYSICAL_DEVICE_PROPERTIES2	1
#define VULKAN_ENABLE_DUMP_LAYER				0
#define VULKAN_COMMANDWRAPPERS_ENABLE			1
#define VULKAN_DYNAMICALLYLOADED				0
#define VULKAN_ENABLE_DESKTOP_HMD_SUPPORT		1
#define VULKAN_SIGNAL_UNIMPLEMENTED()			checkf(false, TEXT("Unimplemented vulkan functionality: %s"), TEXT(__FUNCTION__))

#include "AllowWindowsPlatformTypes.h"
	#include <vulkan.h>
#include "HideWindowsPlatformTypes.h"


// and now, include the GenericPlatform class
#include "../VulkanGenericPlatform.h"

class FVulkanWindowsPlatform : public FVulkanGenericPlatform
{
public:
	static bool LoadVulkanLibrary();
	static bool LoadVulkanInstanceFunctions(VkInstance inInstance);
	static void FreeVulkanLibrary();

	static void GetInstanceExtensions(TArray<const ANSICHAR*>& OutExtensions);
	static void GetDeviceExtensions(TArray<const ANSICHAR*>& OutExtensions);

	static void CreateSurface(void* WindowHandle, VkInstance Instance, VkSurfaceKHR* OutSurface);
};

typedef FVulkanWindowsPlatform FVulkanPlatform;
