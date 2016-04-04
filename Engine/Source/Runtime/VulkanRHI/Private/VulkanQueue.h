// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved..

/*=============================================================================
	VulkanQueue.h: Private Vulkan RHI definitions.
=============================================================================*/

#pragma once

#include "VulkanRHIPrivate.h"

class FVulkanDevice;
class FVulkanCmdBuffer;
struct FVulkanSemaphore;
class FVulkanSwapChain;

namespace VulkanRHI
{
	class FFence;
}

class FVulkanQueue
{
public:
	FVulkanQueue(FVulkanDevice* InDevice, uint32 InFamilyIndex, uint32 InQueueIndex);

	~FVulkanQueue();

	inline uint32 GetFamilyIndex() const
	{
		return FamilyIndex;
	}

	void Submit(FVulkanCmdBuffer* CmdBuffer);

	void SubmitBlocking(FVulkanCmdBuffer* CmdBuffer);

#if VULKAN_USE_NEW_COMMAND_BUFFERS
#else
	void Submit2(VkCommandBuffer CmdBuffer, VulkanRHI::FFence* Fence);
	uint32 AquireImageIndex(FVulkanSwapChain* Swapchain);
	void Present(FVulkanSwapChain* Swapchain, uint32 ImageIndex);
#endif

	inline VkQueue GetHandle() const
	{
		return Queue;
	}

private:
#if VULKAN_USE_NEW_COMMAND_BUFFERS
#else
	PFN_vkAcquireNextImageKHR AcquireNextImageKHR;
	PFN_vkQueuePresentKHR QueuePresentKHR;

	FVulkanSemaphore* ImageAcquiredSemaphore[VULKAN_NUM_IMAGE_BUFFERS];
	FVulkanSemaphore* RenderingCompletedSemaphore[VULKAN_NUM_IMAGE_BUFFERS];
	VulkanRHI::FFence* Fences[VULKAN_NUM_IMAGE_BUFFERS];
	uint32	CurrentFenceIndex;

	uint32 CurrentImageIndex;	// Perhaps move this to the FVulkanSwapChain?
#endif
	VkQueue Queue;
	uint32 FamilyIndex;
	uint32 QueueIndex;
	FVulkanDevice* Device;
};
