// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanQueue.cpp: Vulkan Queue implementation.
=============================================================================*/

#include "VulkanRHIPrivate.h"
#include "VulkanQueue.h"
#include "VulkanSwapChain.h"
#include "VulkanMemory.h"

static int32 GWaitForIdleOnSubmit = 0;
static FAutoConsoleVariableRef CVarVulkanWaitForIdleOnSubmit(
	TEXT("r.Vulkan.WaitForIdleOnSubmit"),
	GWaitForIdleOnSubmit,
	TEXT("Waits for the GPU to be idle on every submit. Useful for tracking GPU hangs.\n")
	TEXT(" 0: Do not wait(default)\n")
	TEXT(" 1: Wait"),
	ECVF_Default
	);

FVulkanQueue::FVulkanQueue(FVulkanDevice* InDevice, uint32 InFamilyIndex, uint32 InQueueIndex)
	: Queue(VK_NULL_HANDLE)
	, FamilyIndex(InFamilyIndex)
	, QueueIndex(InQueueIndex)
	, Device(InDevice)
{
	vkGetDeviceQueue(Device->GetInstanceHandle(), FamilyIndex, InQueueIndex, &Queue);
}

FVulkanQueue::~FVulkanQueue()
{
	check(Device);
}

void FVulkanQueue::Submit(FVulkanCmdBuffer* CmdBuffer)
{
	check(CmdBuffer->HasEnded());

	VkSubmitInfo SubmitInfo;
	FMemory::Memzero(SubmitInfo);

	auto* Fence = CmdBuffer->GetFence();
	check(!Fence->IsSignaled());

	const VkCommandBuffer CmdBuffers[] = { CmdBuffer->GetHandle() };

	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = CmdBuffers;
	VERIFYVULKANRESULT(vkQueueSubmit(Queue, 1, &SubmitInfo, Fence->GetHandle()));

	if (GWaitForIdleOnSubmit != 0)
	{
		VERIFYVULKANRESULT(vkQueueWaitIdle(Queue));
	}

	CmdBuffer->State = FVulkanCmdBuffer::EState::Submitted;

	CmdBuffer->GetOwner()->RefreshFenceStatus();
}

void FVulkanQueue::SubmitBlocking(FVulkanCmdBuffer* CmdBuffer)
{
#if 1//VULKAN_USE_NEW_COMMAND_BUFFERS
	check(0);
#else
	check(CmdBuffer);
	CmdBuffer->End();

	const VkCommandBuffer CmdBufs[] = { CmdBuffer->GetHandle(false) };

	VkSubmitInfo SubmitInfo;
	FMemory::Memzero(SubmitInfo);
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = CmdBufs;

	auto* Fence = Device->GetFenceManager().AllocateFence();
	VERIFYVULKANRESULT(vkQueueSubmit(Queue, 1, &SubmitInfo, Fence->GetHandle()));

	bool bSuccess = Device->GetFenceManager().WaitForFence(Fence, 0xFFFFFFFF);
	check(bSuccess);

	Device->GetFenceManager().ReleaseFence(Fence);
#endif
}
