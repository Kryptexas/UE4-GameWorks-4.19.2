// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanSwapChain.h: Vulkan viewport RHI definitions.
=============================================================================*/

#pragma once

class FVulkanQueue;

class FVulkanSwapChain
{
public:
#if VULKAN_USE_NEW_COMMAND_BUFFERS
#else
#if !PLATFORM_WINDOWS
	//@HACK : maybe NUM_BUFFERS at VulkanViewport should be moved to here?
	enum { NUM_BUFFERS = 2 };
#endif
#endif

	FVulkanSwapChain(VkInstance Instance, FVulkanDevice& InDevice, void* WindowHandle, EPixelFormat& InOutPixelFormat, uint32 Width, uint32 Height,
		uint32* InOutDesiredNumBackBuffers, TArray<VkImage>& OutImages);

	void Destroy();

#if VULKAN_USE_NEW_COMMAND_BUFFERS
	void Present(FVulkanQueue* Queue);
#else
#endif

protected:
	VkSwapchainKHR SwapChain;
	FVulkanDevice& Device;

	PFN_vkCreateSwapchainKHR CreateSwapchainKHR;
	PFN_vkDestroySwapchainKHR DestroySwapchainKHR;
	PFN_vkGetSwapchainImagesKHR GetSwapchainImagesKHR;
	PFN_vkQueuePresentKHR QueuePresentKHR;
	PFN_vkAcquireNextImageKHR AcquireNextImageKHR;

	VkSurfaceKHR Surface;

#if VULKAN_USE_NEW_COMMAND_BUFFERS
	int32 CurrentImageIndex;
	TArray<FVulkanSemaphore*> ImageAcquiredSemaphore;
	TArray<FVulkanSemaphore*> RenderingCompletedSemaphore;
#else
#endif

#if VULKAN_USE_NEW_COMMAND_BUFFERS
	int32 AcquireImageIndex();
	friend class FVulkanViewport;
#else
#endif
	friend class FVulkanQueue;
};
