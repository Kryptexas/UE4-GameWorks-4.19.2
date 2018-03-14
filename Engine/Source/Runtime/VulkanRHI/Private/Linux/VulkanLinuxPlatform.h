// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#define VULKAN_HAS_PHYSICAL_DEVICE_PROPERTIES2		1
#define VULKAN_COMMANDWRAPPERS_ENABLE				0
#define VULKAN_DYNAMICALLYLOADED					1
#define VULKAN_SHOULD_DEBUG_IN_DEVELOPMENT			0
#define VULKAN_SHOULD_ENABLE_DRAW_MARKERS			0
#define VULKAN_SIGNAL_UNIMPLEMENTED()				checkf(false, TEXT("Unimplemented vulkan functionality: %s"), __PRETTY_FUNCTION__)


// this will get the vulkan function signatures 
#include "../VulkanLoader.h"

// and now, include the GenericPlatform class
#include "../VulkanGenericPlatform.h"

class FVulkanLinuxPlatform : public FVulkanGenericPlatform
{
public:
	static bool LoadVulkanLibrary();
	static bool LoadVulkanInstanceFunctions(VkInstance inInstance);
	static void FreeVulkanLibrary();

	static void GetInstanceExtensions(TArray<const ANSICHAR*>& OutExtensions);
	static void GetDeviceExtensions(TArray<const ANSICHAR*>& OutExtensions);

	static void CreateSurface(void* WindowHandle, VkInstance Instance, VkSurfaceKHR* OutSurface);

protected:
	static void* VulkanLib;
	static bool bAttemptedLoad;
};

typedef FVulkanLinuxPlatform FVulkanPlatform;
