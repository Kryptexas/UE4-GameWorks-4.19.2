// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanCommandBuffer.cpp: Vulkan device RHI implementation.
=============================================================================*/

#include "VulkanRHIPrivate.h"
#include "VulkanCommandBuffer.h"

FVulkanCmdBuffer::FVulkanCmdBuffer(FVulkanDevice* InDevice, FVulkanCommandBufferManager* InCommandBufferManager)
	: Device(InDevice)
	, CommandBufferManager(InCommandBufferManager)
	, CommandBufferHandle(VK_NULL_HANDLE)
#if VULKAN_USE_NEW_COMMAND_BUFFERS
	, State(EState::ReadyForBegin)
	, Fence(nullptr)
	, FenceSignaledCounter(0)
#else
	, IsWriting(VK_FALSE)
	, IsEmpty(VK_TRUE)
#endif
{
	check(Device);

	VkCommandBufferAllocateInfo CreateCmdBufInfo;
	FMemory::Memzero(CreateCmdBufInfo);
	CreateCmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	CreateCmdBufInfo.pNext = nullptr;
	CreateCmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	CreateCmdBufInfo.commandBufferCount = 1;
	CreateCmdBufInfo.commandPool = CommandBufferManager->GetHandle();

	VERIFYVULKANRESULT(vkAllocateCommandBuffers(Device->GetInstanceHandle(), &CreateCmdBufInfo, &CommandBufferHandle));
#if VULKAN_USE_NEW_COMMAND_BUFFERS
	Fence = Device->GetFenceManager().AllocateFence();
#endif
}

FVulkanCmdBuffer::~FVulkanCmdBuffer()
{
#if VULKAN_USE_NEW_COMMAND_BUFFERS
	auto& FenceManager = Device->GetFenceManager();
	FenceManager.WaitAndReleaseFence(Fence, 0xffffffff);
#else
	check(IsWriting == VK_FALSE);
	if (CommandBufferHandle != VK_NULL_HANDLE)
#endif
	{
		vkFreeCommandBuffers(Device->GetInstanceHandle(), CommandBufferManager->GetHandle(), 1, &CommandBufferHandle);
		CommandBufferHandle = VK_NULL_HANDLE;
	}
}

#if VULKAN_USE_NEW_COMMAND_BUFFERS
void FVulkanCmdBuffer::BeginRenderPass(const FVulkanRenderTargetLayout& Layout, VkRenderPass RenderPass, VkFramebuffer Framebuffer, const VkClearValue* AttachmentClearValues)
{
	check(IsOutsideRenderPass());

	VkRenderPassBeginInfo Info;
	FMemory::Memzero(Info);
	Info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	Info.renderPass = RenderPass;
	Info.framebuffer = Framebuffer;
	Info.renderArea.offset.x = 0;
	Info.renderArea.offset.y = 0;
	Info.renderArea.extent = Layout.GetExtent2D();
	Info.clearValueCount = Layout.GetNumAttachments();
	Info.pClearValues = AttachmentClearValues;

	vkCmdBeginRenderPass(CommandBufferHandle, &Info, VK_SUBPASS_CONTENTS_INLINE);

	State = EState::IsInsideRenderPass;
}
#else
VkCommandBuffer& FVulkanCmdBuffer::GetHandle(const VkBool32 WritingToCommandBuffer /* = true */)
{
	check(CommandBufferHandle != VK_NULL_HANDLE);
	IsEmpty &= !WritingToCommandBuffer;
	return CommandBufferHandle;
}
#endif


void FVulkanCmdBuffer::Begin()
{
#if VULKAN_USE_NEW_COMMAND_BUFFERS
	check(State == EState::ReadyForBegin);
#else
	check(!IsWriting);
	check(IsEmpty);
#endif
	VkCommandBufferBeginInfo CmdBufBeginInfo;
	FMemory::Memzero(CmdBufBeginInfo);
	CmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	CmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VERIFYVULKANRESULT(vkBeginCommandBuffer(CommandBufferHandle, &CmdBufBeginInfo));

#if VULKAN_USE_NEW_COMMAND_BUFFERS
	State = EState::IsInsideBegin;
#else
	IsWriting = VK_TRUE;
#endif
}

#if VULKAN_USE_NEW_COMMAND_BUFFERS
void FVulkanCmdBuffer::RefreshFenceStatus()
{
	if (State == EState::Submitted)
	{
		if (Fence->GetOwner()->IsFenceSignaled(Fence))
		{
			State = EState::ReadyForBegin;
			vkResetCommandBuffer(CommandBufferHandle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
			Fence->GetOwner()->ResetFence(Fence);
			++FenceSignaledCounter;
		}
	}
	else
	{
		check(!Fence->IsSignaled());
	}
}
#else
VkBool32 FVulkanCmdBuffer::GetIsWriting() const
{
	return IsWriting;
}

VkBool32 FVulkanCmdBuffer::GetIsEmpty() const
{
	return IsEmpty;
}

void FVulkanCmdBuffer::Reset()
{
	check(!IsWriting)

	VERIFYVULKANRESULT(vkResetCommandBuffer(GetHandle(), VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

	IsEmpty = VK_TRUE;
}

void FVulkanCmdBuffer::End()
{
	check(IsWriting)

	VERIFYVULKANRESULT(vkEndCommandBuffer(GetHandle()));

	IsWriting = VK_FALSE;
	IsEmpty = VK_TRUE;
}
#endif

FVulkanCommandBufferManager::FVulkanCommandBufferManager(FVulkanDevice* InDevice)
	: Device(InDevice)
	, Handle(VK_NULL_HANDLE)
#if VULKAN_USE_NEW_COMMAND_BUFFERS
	, ActiveCmdBuffer(nullptr)
#else
#endif
{
	check(Device);

	VkCommandPoolCreateInfo CmdPoolInfo;
	FMemory::Memzero(CmdPoolInfo);
	CmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	CmdPoolInfo.queueFamilyIndex = Device->GetQueue()->GetFamilyIndex();
	//#todo-rco: Should we use VK_COMMAND_POOL_CREATE_TRANSIENT_BIT?
	CmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VERIFYVULKANRESULT(vkCreateCommandPool(Device->GetInstanceHandle(), &CmdPoolInfo, nullptr, &Handle));

#if VULKAN_USE_NEW_COMMAND_BUFFERS
	ActiveCmdBuffer = Create();
	ActiveCmdBuffer->Begin();
#else
#endif
}

FVulkanCommandBufferManager::~FVulkanCommandBufferManager()
{
	for (int32 Index = 0; Index < CmdBuffers.Num(); ++Index)
	{
		FVulkanCmdBuffer* CmdBuffer = CmdBuffers[Index];
		delete CmdBuffer;
	}

	vkDestroyCommandPool(Device->GetInstanceHandle(), Handle, nullptr);
}

FVulkanCmdBuffer* FVulkanCommandBufferManager::Create()
{
	check(Device);
	FVulkanCmdBuffer* CmdBuffer = new FVulkanCmdBuffer(Device, this);
	CmdBuffers.Add(CmdBuffer);
	check(CmdBuffer);
	return CmdBuffer;
}

#if VULKAN_USE_NEW_COMMAND_BUFFERS
void FVulkanCommandBufferManager::RefreshFenceStatus()
{
	for (int32 Index = 0; Index < CmdBuffers.Num(); ++Index)
	{
		FVulkanCmdBuffer* CmdBuffer = CmdBuffers[Index];
		CmdBuffer->RefreshFenceStatus();
	}
}

void FVulkanCommandBufferManager::PrepareForNewActiveCommandBuffer()
{
	for (int32 Index = 0; Index < CmdBuffers.Num(); ++Index)
	{
		FVulkanCmdBuffer* CmdBuffer = CmdBuffers[Index];
		CmdBuffer->RefreshFenceStatus();
		if (CmdBuffer->State == FVulkanCmdBuffer::EState::ReadyForBegin)
		{
			ActiveCmdBuffer = CmdBuffer;
			ActiveCmdBuffer->Begin();
			return;
		}
		else
		{
			check(CmdBuffer->State == FVulkanCmdBuffer::EState::Submitted);
		}
	}

	// All cmd buffers are being executed still
	ActiveCmdBuffer = Create();
	ActiveCmdBuffer->Begin();
}
#else
void FVulkanCommandBufferManager::Destroy(FVulkanCmdBuffer* CmdBuffer)
{
	CmdBuffers.Remove(CmdBuffer);
	check(Device);
	if (CmdBuffer->GetIsWriting())
	{
		CmdBuffer->End();
	}
	vkFreeCommandBuffers(Device->GetInstanceHandle(), GetHandle(), 1, &CmdBuffer->CommandBufferHandle);
	CmdBuffer->CommandBufferHandle = VK_NULL_HANDLE;
	delete CmdBuffer;
}

void FVulkanCommandBufferManager::Submit(FVulkanCmdBuffer* CmdBuffer)
{
	Device->GetQueue()->Submit(CmdBuffer);
}
#endif
