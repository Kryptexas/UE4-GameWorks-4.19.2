// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

// the platform interface, and empty implementations for platforms that don't need em
class FVulkanGenericPlatform 
{
public:
	static bool LoadVulkanLibrary() { return true; }
	static bool LoadVulkanInstanceFunctions(VkInstance inInstance) { return true; }
	static void FreeVulkanLibrary() {}

	// Array of extensions for the platform - required to implement!
	static void GetInstanceExtensions(TArray<const ANSICHAR*>& OutExtensions);
	static void GetDeviceExtensions(TArray<const ANSICHAR*>& OutExtensions);

	// create the platform-specific surface object - required
	static void CreateSurface(VkSurfaceKHR* OutSurface);

	// most platforms support BC* but not ASTC*
	static bool SupportsBCTextureFormats() { return true; }
	static bool SupportsASTCTextureFormats() { return false; }

	// most platforms can query the surface for the present mode, and size, etc
	static bool SupportsQuerySurfaceProperties() { return true; }
};
