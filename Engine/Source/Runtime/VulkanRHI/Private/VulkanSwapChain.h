// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanSwapChain.h: Vulkan viewport RHI definitions.
=============================================================================*/

#pragma once

namespace VulkanRHI
{
	class FFence;
}

class FVulkanQueue;

class FVulkanSwapChain
{
public:
	FVulkanSwapChain(VkInstance InInstance, FVulkanDevice& InDevice, void* WindowHandle, EPixelFormat& InOutPixelFormat, uint32 Width, uint32 Height,
		uint32* InOutDesiredNumBackBuffers, TArray<VkImage>& OutImages);

	void Destroy();

	bool Present(FVulkanQueue* Queue, FVulkanSemaphore* BackBufferRenderingDoneSemaphore);

protected:
	VkSwapchainKHR SwapChain;
	FVulkanDevice& Device;

	VkSurfaceKHR Surface;

	int32 CurrentImageIndex;
	int32 SemaphoreIndex;
	uint32 NumPresentCalls;
	uint32 NumAcquireCalls;
	VkInstance Instance;
	TArray<FVulkanSemaphore*> ImageAcquiredSemaphore;
	TArray<VulkanRHI::FFence*> ImageAcquiredFences;

	int32 AcquireImageIndex(FVulkanSemaphore** OutSemaphore);

	friend class FVulkanViewport;
	friend class FVulkanQueue;
};
