// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "VulkanWindowsPlatform.h"
#include "../VulkanRHIPrivate.h"

#include "AllowWindowsPlatformTypes.h"
static HMODULE GVulkanDLLModule = nullptr;

bool FVulkanWindowsPlatform::LoadVulkanLibrary()
{
	// Try to load the vulkan dll, as not everyone has the sdk installed
	GVulkanDLLModule = ::LoadLibraryW(TEXT("vulkan-1.dll"));
	return GVulkanDLLModule != nullptr;
}

namespace VulkanRHI
{
	extern PFN_vkGetPhysicalDeviceProperties2KHR GVkGetPhysicalDeviceProperties2KHR;
}

bool FVulkanWindowsPlatform::LoadVulkanInstanceFunctions(VkInstance inInstance)
{
	if (!GVulkanDLLModule)
	{
		return false;
	}

	PFN_vkGetInstanceProcAddr GetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)FPlatformProcess::GetDllExport(GVulkanDLLModule, TEXT("vkGetInstanceProcAddr"));

	if (!GetInstanceProcAddr)
	{
		return false;
	}

#pragma warning(push)
#pragma warning(disable : 4191) // warning C4191: 'type cast': unsafe conversion
	VulkanRHI::GVkGetPhysicalDeviceProperties2KHR = (PFN_vkGetPhysicalDeviceProperties2KHR)GetInstanceProcAddr(inInstance, "vkGetPhysicalDeviceProperties2KHR");
#pragma warning(pop) // restore 4191

	return true;
}

void FVulkanWindowsPlatform::FreeVulkanLibrary()
{
	if (GVulkanDLLModule != nullptr)
	{
		::FreeLibrary(GVulkanDLLModule);
		GVulkanDLLModule = nullptr;
	}
}

#include "HideWindowsPlatformTypes.h"



void FVulkanWindowsPlatform::GetInstanceExtensions(TArray<const ANSICHAR*>& OutExtensions)
{
	// windows surface extension
	OutExtensions.Add(VK_KHR_SURFACE_EXTENSION_NAME);
	OutExtensions.Add(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
}


void FVulkanWindowsPlatform::GetDeviceExtensions(TArray<const ANSICHAR*>& OutExtensions)
{
	// Nothing here
}

void FVulkanWindowsPlatform::CreateSurface(void* WindowHandle, VkInstance Instance, VkSurfaceKHR* OutSurface)
{
	VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo;
	FMemory::Memzero(SurfaceCreateInfo);
	SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	SurfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
	SurfaceCreateInfo.hwnd = (HWND)WindowHandle;
	VERIFYVULKANRESULT(vkCreateWin32SurfaceKHR(Instance, &SurfaceCreateInfo, nullptr, OutSurface));
}
