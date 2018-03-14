// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "VulkanLinuxPlatform.h"
#include "../VulkanRHIPrivate.h"
#include <dlfcn.h>
#include <SDL.h>



// Vulkan function pointers
#define DEFINE_VK_ENTRYPOINTS(Type,Func) Type VulkanDynamicAPI::Func = NULL;
ENUM_VK_ENTRYPOINTS_ALL(DEFINE_VK_ENTRYPOINTS)


void* FVulkanLinuxPlatform::VulkanLib = nullptr;
bool FVulkanLinuxPlatform::bAttemptedLoad = false;

bool FVulkanLinuxPlatform::LoadVulkanLibrary()
{
	if (bAttemptedLoad)
	{
		return (VulkanLib != nullptr);
	}
	bAttemptedLoad = true;

	// try to load libvulkan.so
	VulkanLib = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);

	if (VulkanLib == nullptr)
	{
		return false;
	}

	bool bFoundAllEntryPoints = true;
#define CHECK_VK_ENTRYPOINTS(Type,Func) if (VulkanDynamicAPI::Func == NULL) { bFoundAllEntryPoints = false; UE_LOG(LogRHI, Warning, TEXT("Failed to find entry point for %s"), TEXT(#Func)); }

	// Initialize all of the entry points we have to query manually
#define GET_VK_ENTRYPOINTS(Type,Func) VulkanDynamicAPI::Func = (Type)dlsym(VulkanLib, #Func);
	ENUM_VK_ENTRYPOINTS_BASE(GET_VK_ENTRYPOINTS);
	ENUM_VK_ENTRYPOINTS_BASE(CHECK_VK_ENTRYPOINTS);
	if (!bFoundAllEntryPoints)
	{
		dlclose(VulkanLib);
		VulkanLib = nullptr;
		return false;
	}

	ENUM_VK_ENTRYPOINTS_OPTIONAL(GET_VK_ENTRYPOINTS);
	//ENUM_VK_ENTRYPOINTS_OPTIONAL(CHECK_VK_ENTRYPOINTS);

	return true;
}

bool FVulkanLinuxPlatform::LoadVulkanInstanceFunctions(VkInstance inInstance)
{
	bool bFoundAllEntryPoints = true;
#define CHECK_VK_ENTRYPOINTS(Type,Func) if (VulkanDynamicAPI::Func == NULL) { bFoundAllEntryPoints = false; UE_LOG(LogRHI, Warning, TEXT("Failed to find entry point for %s"), TEXT(#Func)); }

#define GETINSTANCE_VK_ENTRYPOINTS(Type, Func) VulkanDynamicAPI::Func = (Type)VulkanDynamicAPI::vkGetInstanceProcAddr(inInstance, #Func);
	ENUM_VK_ENTRYPOINTS_INSTANCE(GETINSTANCE_VK_ENTRYPOINTS);
	ENUM_VK_ENTRYPOINTS_INSTANCE(CHECK_VK_ENTRYPOINTS);

	return bFoundAllEntryPoints;
}

void FVulkanLinuxPlatform::FreeVulkanLibrary()
{
	if (VulkanLib != nullptr)
	{
#define CLEAR_VK_ENTRYPOINTS(Type,Func) VulkanDynamicAPI::Func = nullptr;
		ENUM_VK_ENTRYPOINTS_ALL(CLEAR_VK_ENTRYPOINTS);

		dlclose(VulkanLib);
		VulkanLib = nullptr;
	}
	bAttemptedLoad = false;
}



void FVulkanLinuxPlatform::GetInstanceExtensions(TArray<const ANSICHAR*>& OutExtensions)
{
	// we don't hardcode the extensions in Linux, we query SDL
	static TArray<const ANSICHAR*> CachedLinuxExtensions;
	if (CachedLinuxExtensions.Num() == 0)
	{
		uint32_t Count = 0;
		auto RequiredExtensions = SDL_VK_GetRequiredInstanceExtensions(&Count);
		for (int32 i = 0; i < Count; i++)
		{
			CachedLinuxExtensions.Add(RequiredExtensions[i]);
		}
	}
	OutExtensions.Append(CachedLinuxExtensions);
}

void FVulkanLinuxPlatform::GetDeviceExtensions(TArray<const ANSICHAR*>& OutExtensions)
{
	// Nothing here!
}

void FVulkanLinuxPlatform::CreateSurface(void* WindowHandle, VkInstance Instance, VkSurfaceKHR* OutSurface)
{
	if (SDL_VK_CreateSurface((SDL_Window*)WindowHandle, (SDL_VkInstance)Instance, (SDL_VkSurface*)OutSurface) == SDL_FALSE)
	{
		UE_LOG(LogInit, Error, TEXT("Error initializing SDL Vulkan Surface: %s"), SDL_GetError());
		check(0);
	}
}
