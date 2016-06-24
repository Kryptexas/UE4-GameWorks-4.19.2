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

void FVulkanQueue::Submit(FVulkanCmdBuffer* CmdBuffer, FVulkanSemaphore* WaitSemaphore, VkPipelineStageFlags WaitStageFlags, FVulkanSemaphore* SignalSemaphore)
{
	check(CmdBuffer->HasEnded());

	VulkanRHI::FFence* Fence = CmdBuffer->Fence;
	check(!Fence->IsSignaled());

	const VkCommandBuffer CmdBuffers[] = { CmdBuffer->GetHandle() };

	VkSemaphore Semaphores[2];

	VkSubmitInfo SubmitInfo;
	FMemory::Memzero(SubmitInfo);
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = CmdBuffers;
	if (SignalSemaphore)
	{
		SubmitInfo.signalSemaphoreCount = 1;
		Semaphores[0] = SignalSemaphore->GetHandle();
		SubmitInfo.pSignalSemaphores = &Semaphores[0];
	}
	if (WaitSemaphore)
	{
		SubmitInfo.waitSemaphoreCount = 1;
		Semaphores[1] = WaitSemaphore->GetHandle();
		SubmitInfo.pWaitSemaphores = &Semaphores[1];
		SubmitInfo.pWaitDstStageMask = &WaitStageFlags;
	}
	//FPlatformMisc::LowLevelOutputDebugStringf(TEXT("*** VkQueueSubmit CmdBuffer %p Fence %p\n"), (void*)CmdBuffer->GetHandle(), (void*)Fence->GetHandle());
	VERIFYVULKANRESULT(vkQueueSubmit(Queue, 1, &SubmitInfo, Fence->GetHandle()));

	if (GWaitForIdleOnSubmit != 0)
	{
		VERIFYVULKANRESULT(vkQueueWaitIdle(Queue));
	}

	CmdBuffer->State = FVulkanCmdBuffer::EState::Submitted;

	CmdBuffer->GetOwner()->RefreshFenceStatus();
}
