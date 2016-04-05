// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved..

/*=============================================================================
	VulkanCommandBuffer.h: Private Vulkan RHI definitions.
=============================================================================*/

#pragma once

#include "VulkanRHIPrivate.h"

class FVulkanDevice;
class FVulkanCommandBufferManager;

namespace VulkanRHI
{
	class FFence;
}

class FVulkanCmdBuffer
{
protected:
	friend class FVulkanCommandBufferManager;
	friend class FVulkanQueue;

	FVulkanCmdBuffer(FVulkanDevice* InDevice, FVulkanCommandBufferManager* InCommandBufferManager);
	~FVulkanCmdBuffer();

public:
#if VULKAN_USE_NEW_COMMAND_BUFFERS
/*
	inline bool IsWritable() const
	{
		return State == EState::IsInsideBegin || State == EState::IsInsideRenderPass;
	}
*/
	FVulkanCommandBufferManager* GetOwner()
	{
		return CommandBufferManager;
	}

	inline bool IsInsideRenderPass() const
	{
		return State == EState::IsInsideRenderPass;
	}

	inline bool IsOutsideRenderPass() const
	{
		return State == EState::IsInsideBegin;
	}

	inline bool HasEnded() const
	{
		return State == EState::HasEnded;
	}

	inline VkCommandBuffer GetHandle()
	{
		return CommandBufferHandle;
	}

	void BeginRenderPass(const FVulkanRenderTargetLayout& Layout, VkRenderPass RenderPass, VkFramebuffer Framebuffer, const VkClearValue* AttachmentClearValues);

	void EndRenderPass()
	{
		check(IsInsideRenderPass());
		vkCmdEndRenderPass(CommandBufferHandle);
		State = EState::IsInsideBegin;
	}

	void End()
	{
		check(IsOutsideRenderPass());
		VERIFYVULKANRESULT(vkEndCommandBuffer(GetHandle()));
		State = EState::HasEnded;
	}

	inline VulkanRHI::FFence* GetFence()
	{
		return Fence;
	}

	inline uint64 GetFenceSignaledCounter() const
	{
		return FenceSignaledCounter;
	}
#else
	VkCommandBuffer& GetHandle(const VkBool32 MakeDirty = true);
#endif

	void Begin();
#if VULKAN_USE_NEW_COMMAND_BUFFERS
	enum class EState
	{
		ReadyForBegin,
		IsInsideBegin,
		IsInsideRenderPass,
		HasEnded,
		Submitted,
	};
#else

	void Reset();

	void End();

	VkBool32 GetIsWriting() const;
	VkBool32 GetIsEmpty() const;
#endif
private:
	FVulkanDevice* Device;
	VkCommandBuffer CommandBufferHandle;
#if VULKAN_USE_NEW_COMMAND_BUFFERS
	EState State;
	VulkanRHI::FFence* Fence;
	uint64 FenceSignaledCounter;
	void RefreshFenceStatus();
#else
	VkBool32 IsWriting;
	VkBool32 IsEmpty;
#endif

	FVulkanCommandBufferManager* CommandBufferManager;
};

class FVulkanCommandBufferManager
{
public:
	FVulkanCommandBufferManager(FVulkanDevice* InDevice);

	~FVulkanCommandBufferManager();

#if VULKAN_USE_NEW_COMMAND_BUFFERS
	FVulkanCmdBuffer* GetActiveCmdBuffer()
	{
		return ActiveCmdBuffer;
	}

	void RefreshFenceStatus();
	void PrepareForNewActiveCommandBuffer();
#else
	FVulkanCmdBuffer* Create();

	void Destroy(FVulkanCmdBuffer* CmdBuffer);

	void Submit(FVulkanCmdBuffer* CmdBuffer);

private:
	friend class FVulkanCmdBuffer;
	friend class FVulkanDynamicRHI;
#endif
	inline VkCommandPool GetHandle() const
	{
		check(Handle != VK_NULL_HANDLE);
		return Handle;
	}
private:
	FVulkanDevice* Device;
	VkCommandPool Handle;
#if VULKAN_USE_NEW_COMMAND_BUFFERS
	FVulkanCmdBuffer* ActiveCmdBuffer;
	FVulkanCmdBuffer* Create();
#else
#endif
	TArray<FVulkanCmdBuffer*> CmdBuffers;
};
