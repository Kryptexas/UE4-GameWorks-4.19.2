// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanQueue.cpp: Vulkan Queue implementation.
=============================================================================*/

#include "VulkanRHIPrivate.h"
#include "VulkanQueue.h"
#include "VulkanSwapChain.h"
#include "VulkanMemory.h"

static int32 GWaitForIdleOnSubmit = 0;
static FAutoConsoleVariableRef CVarVulkanUseExternalShaderCompiler(
	TEXT("r.Vulkan.WaitForIdleOnSubmit"),
	GWaitForIdleOnSubmit,
	TEXT("Waits for the GPU to be idle on every submit. Useful for tracking GPU hangs.\n")
	TEXT(" 0: Do not wait(default)\n")
	TEXT(" 1: Wait"),
	ECVF_Default
	);

FVulkanQueue::FVulkanQueue(FVulkanDevice* InDevice, uint32 InFamilyIndex, uint32 InQueueIndex)
	: 
#if VULKAN_USE_NEW_COMMAND_BUFFERS
#else
	AcquireNextImageKHR(VK_NULL_HANDLE)
	, QueuePresentKHR(VK_NULL_HANDLE)
	, CurrentFenceIndex(0)
	, CurrentImageIndex(-1)
	, 
#endif
	Queue(VK_NULL_HANDLE)
	, FamilyIndex(InFamilyIndex)
	, QueueIndex(InQueueIndex)
	, Device(InDevice)
{
#if VULKAN_USE_NEW_COMMAND_BUFFERS
#else
	check(Device);
	for (int BufferIndex = 0; BufferIndex < VULKAN_NUM_IMAGE_BUFFERS; ++BufferIndex)
	{
		ImageAcquiredSemaphore[BufferIndex] = new FVulkanSemaphore(*Device);
		RenderingCompletedSemaphore[BufferIndex] = new FVulkanSemaphore(*Device);
		Fences[BufferIndex] = nullptr;
	}
#endif
	vkGetDeviceQueue(Device->GetInstanceHandle(), FamilyIndex, InQueueIndex, &Queue);

#if VULKAN_USE_NEW_COMMAND_BUFFERS
#else
	AcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)(void*)vkGetDeviceProcAddr(Device->GetInstanceHandle(), "vkAcquireNextImageKHR");
	check(AcquireNextImageKHR);

	QueuePresentKHR = (PFN_vkQueuePresentKHR)(void*)vkGetDeviceProcAddr(Device->GetInstanceHandle(), "vkQueuePresentKHR");
	check(QueuePresentKHR);
#endif
}

FVulkanQueue::~FVulkanQueue()
{
	check(Device);

#if VULKAN_USE_NEW_COMMAND_BUFFERS
#else
	if (Fences[CurrentFenceIndex] != VK_NULL_HANDLE)
	{
		Device->GetFenceManager().WaitForFence(Fences[CurrentFenceIndex], UINT32_MAX);
	}

	for (int BufferIndex = 0; BufferIndex < VULKAN_NUM_IMAGE_BUFFERS; ++BufferIndex)
	{
		delete ImageAcquiredSemaphore[BufferIndex];
		ImageAcquiredSemaphore[BufferIndex] = nullptr;
		delete RenderingCompletedSemaphore[BufferIndex];
		RenderingCompletedSemaphore[BufferIndex] = nullptr;

		if (Fences[BufferIndex])
		{
			Device->GetFenceManager().ReleaseFence(Fences[BufferIndex]);
			Fences[BufferIndex] = nullptr;
		}
	}
#endif
}

#if VULKAN_USE_NEW_COMMAND_BUFFERS
#else
uint32 FVulkanQueue::AquireImageIndex(FVulkanSwapChain* Swapchain)
{	
	check(Device);
	check(Swapchain);

	//@TODO: Clean up code once the API and SDK is more robust.
	//@SMEDIS: Since we only have one queue, we shouldn't actually need any semaphores in this .cpp file.
	//@SMEDIS: I also don't think we need the Fences[] array.

	CurrentFenceIndex = (CurrentFenceIndex + 1) % VULKAN_NUM_IMAGE_BUFFERS;

	if (Fences[CurrentFenceIndex] != VK_NULL_HANDLE)
	{
		#if 0
			VkResult Result = vkGetFenceStatus(Device->GetInstanceHandle(), Fences[CurrentFenceIndex]);
			if(Result >= VK_SUCCESS)
		#endif
		{
			// Grab the next fence and re-use it.
			Device->GetFenceManager().WaitForFence(Fences[CurrentFenceIndex], UINT32_MAX);
			Device->GetFenceManager().ResetFence(Fences[CurrentFenceIndex]);
		}
	}

	// Get the index of the next swapchain image we should render to.
	// We'll wait with an "infinite" timeout, the function will block until an image is ready.
	// The ImageAcquiredSemaphore[ImageAcquiredSemaphoreIndex] will get signaled when the image is ready (upon function return).
	// The Fences[CurrentFenceIndex] will also get signaled when the image is ready (upon function return).
	// Note: Queues can still be filled in on the CPU side, but won't execute until the semaphore is signaled.
	CurrentImageIndex = -1;
	VkResult Result = AcquireNextImageKHR(
		Device->GetInstanceHandle(),
		Swapchain->SwapChain,
		UINT64_MAX,
		ImageAcquiredSemaphore[CurrentFenceIndex]->GetHandle(),
		VK_NULL_HANDLE,
		&CurrentImageIndex);
	checkf(Result == VK_SUCCESS || Result == VK_SUBOPTIMAL_KHR, TEXT("AcquireNextImageKHR failed Result = %d"), int32(Result));

	return CurrentImageIndex;
}
#endif

#if VULKAN_USE_NEW_COMMAND_BUFFERS
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
#else
void FVulkanQueue::Submit(FVulkanCmdBuffer* CmdBuffer)
{
	check(CmdBuffer);
	CmdBuffer->End();

	const VkCommandBuffer CmdBuffers[] = { CmdBuffer->GetHandle(false) };
	const VkPipelineStageFlags WaitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo SubmitInfo;
	FMemory::Memzero(SubmitInfo);

	VkSemaphore Local[2] =
		{ 
			ImageAcquiredSemaphore[CurrentFenceIndex]->GetHandle(),
			RenderingCompletedSemaphore[CurrentFenceIndex]->GetHandle()
		};
	SubmitInfo.waitSemaphoreCount = 1;	// 0;	// 1;
	SubmitInfo.pWaitSemaphores = &Local[0];
	SubmitInfo.pWaitDstStageMask = &WaitDstStageMask;
	SubmitInfo.signalSemaphoreCount = 1;	// 0;	// 1;
	SubmitInfo.pSignalSemaphores = &Local[1];
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = CmdBuffers;

	if (!Fences[CurrentFenceIndex])
	{
		Fences[CurrentFenceIndex] = Device->GetFenceManager().AllocateFence();
	}
	VERIFYVULKANRESULT(vkQueueSubmit(Queue, 1, &SubmitInfo, Fences[CurrentFenceIndex]->GetHandle()));

	if (GWaitForIdleOnSubmit != 0)
	{
		VERIFYVULKANRESULT(vkQueueWaitIdle(Queue));
	}
}
#endif

#if !VULKAN_USE_NEW_COMMAND_BUFFERS
void FVulkanQueue::Submit2(VkCommandBuffer CmdBuffer, VulkanRHI::FFence* Fence)
{
	VkSubmitInfo SubmitInfo;
	FMemory::Memzero(SubmitInfo);
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &CmdBuffer;
	check(!Fence || !Fence->IsSignaled());
	VERIFYVULKANRESULT(vkQueueSubmit(Queue, 1, &SubmitInfo, Fence ? Fence->GetHandle() : VK_NULL_HANDLE));

	if (GWaitForIdleOnSubmit != 0)
	{
		VERIFYVULKANRESULT(vkQueueWaitIdle(Queue));
	}
}
#endif

void FVulkanQueue::SubmitBlocking(FVulkanCmdBuffer* CmdBuffer)
{
#if VULKAN_USE_NEW_COMMAND_BUFFERS
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

#if VULKAN_USE_NEW_COMMAND_BUFFERS
#else
void FVulkanQueue::Present(FVulkanSwapChain* Swapchain, uint32 ImageIndex)
{
	check(Swapchain);

	VkPresentInfoKHR Info;
	FMemory::Memzero(Info);
	Info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	Info.waitSemaphoreCount = 1;	// 0;	// 1;
	VkSemaphore Local = RenderingCompletedSemaphore[CurrentFenceIndex]->GetHandle();
	Info.pWaitSemaphores = &Local;
	Info.swapchainCount = 1;
	Info.pSwapchains = &Swapchain->SwapChain;
	Info.pImageIndices = &ImageIndex;
	VkResult OutResult = VK_SUCCESS;
	Info.pResults = &OutResult;
	VERIFYVULKANRESULT(QueuePresentKHR(Queue, &Info));
	VERIFYVULKANRESULT(OutResult);
}
#endif
