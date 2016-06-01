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
class FVulkanCommandListContext;

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

	void Submit(FVulkanCmdBuffer* CmdBuffer, FVulkanSemaphore* WaitSemaphore, VkPipelineStageFlags WaitStageFlags, FVulkanSemaphore* SignalSemaphore);

	inline VkQueue GetHandle() const
	{
		return Queue;
	}

private:
	VkQueue Queue;
	uint32 FamilyIndex;
	uint32 QueueIndex;
	FVulkanDevice* Device;
};
