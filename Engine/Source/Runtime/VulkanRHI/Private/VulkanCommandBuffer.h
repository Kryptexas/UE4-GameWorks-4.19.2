// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved..

/*=============================================================================
	VulkanCommandBuffer.h: Private Vulkan RHI definitions.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "VulkanConfiguration.h"
#if VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
#include "ArrayView.h"
#include "VulkanDescriptorSets.h"
#endif

class FVulkanDevice;
class FVulkanCommandBufferPool;
class FVulkanCommandBufferManager;
class FVulkanRenderTargetLayout;
class FVulkanQueue;
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
class FVulkanDescriptorPoolSet;
#endif

namespace VulkanRHI
{
	class FFence;
}

class FVulkanCmdBuffer
{
protected:
	friend class FVulkanCommandBufferManager;
	friend class FVulkanCommandBufferPool;
	friend class FVulkanQueue;

	FVulkanCmdBuffer(FVulkanDevice* InDevice, FVulkanCommandBufferPool* InCommandBufferPool);
	~FVulkanCmdBuffer();

public:
	FVulkanCommandBufferPool* GetOwner()
	{
		return CommandBufferPool;
	}

	inline bool IsInsideRenderPass() const
	{
		return State == EState::IsInsideRenderPass;
	}

	inline bool IsOutsideRenderPass() const
	{
		return State == EState::IsInsideBegin;
	}

	inline bool HasBegun() const
	{
		return State == EState::IsInsideBegin || State == EState::IsInsideRenderPass;
	}

	inline bool HasEnded() const
	{
		return State == EState::HasEnded;
	}

	inline bool IsSubmitted() const
	{
		return State == EState::Submitted;
	}

	inline VkCommandBuffer GetHandle()
	{
		return CommandBufferHandle;
	}

	void BeginRenderPass(const FVulkanRenderTargetLayout& Layout, class FVulkanRenderPass* RenderPass, class FVulkanFramebuffer* Framebuffer, const VkClearValue* AttachmentClearValues);

	void EndRenderPass()
	{
		check(IsInsideRenderPass());
		VulkanRHI::vkCmdEndRenderPass(CommandBufferHandle);
		State = EState::IsInsideBegin;
	}

	void End();

	inline volatile uint64 GetFenceSignaledCounter() const
	{
		return FenceSignaledCounter;
	}

	inline volatile uint64 GetSubmittedFenceCounter() const
	{
		return SubmittedFenceCounter;
	}

	inline bool HasValidTiming() const
	{
		return (Timing != nullptr) && (FMath::Abs((int64)FenceSignaledCounter - (int64)LastValidTiming) < 3);
	}
	
	void Begin();

	enum class EState
	{
		ReadyForBegin,
		IsInsideBegin,
		IsInsideRenderPass,
		HasEnded,
		Submitted,
	};

	bool bNeedsDynamicStateSet;
	bool bHasPipeline;
	bool bHasViewport;
	bool bHasScissor;
	bool bHasStencilRef;

	VkViewport CurrentViewport;
	VkRect2D CurrentScissor;
	uint32 CurrentStencilRef;

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	FVulkanDescriptorPoolSet* CurrentDescriptorPoolSet = nullptr;
#endif

#if VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
	TArrayView<VkDescriptorSet> AllocateDescriptorSets(const FVulkanLayout& Layout);
	void SetDescriptorSetsFence(const FVulkanLayout& Layout);
#endif

private:
	FVulkanDevice* Device;
	VkCommandBuffer CommandBufferHandle;
	EState State;

	// Do not cache this pointer as it might change depending on VULKAN_REUSE_FENCES
	VulkanRHI::FFence* Fence;

	// Last value passed after the fence got signaled
	volatile uint64 FenceSignaledCounter;
	// Last value when we submitted the cmd buffer; useful to track down if something waiting for the fence has actually been submitted
	volatile uint64 SubmittedFenceCounter;

	void RefreshFenceStatus();
	void InitializeTimings(FVulkanCommandListContext* InContext);

	FVulkanCommandBufferPool* CommandBufferPool;

	FVulkanGPUTiming* Timing;
	uint64 LastValidTiming;

	friend class FVulkanDynamicRHI;
};

class FVulkanCommandBufferPool
{
public:
	FVulkanCommandBufferPool(FVulkanDevice* InDevice);

	~FVulkanCommandBufferPool();

/*
	inline FVulkanCmdBuffer* GetActiveCmdBuffer()
	{
		if (UploadCmdBuffer)
		{
			SubmitUploadCmdBuffer(false);
		}

		return ActiveCmdBuffer;
	}

	inline bool HasPendingUploadCmdBuffer() const
	{
		return UploadCmdBuffer != nullptr;
	}

	inline bool HasPendingActiveCmdBuffer() const
	{
		return ActiveCmdBuffer != nullptr;
	}

	FVulkanCmdBuffer* GetUploadCmdBuffer();

	void SubmitUploadCmdBuffer(bool bWaitForFence);
	void SubmitActiveCmdBuffer(bool bWaitForFence);

	void WaitForCmdBuffer(FVulkanCmdBuffer* CmdBuffer, float TimeInSecondsToWait = 1.0f);
	*/
	void RefreshFenceStatus();
	/*
	void PrepareForNewActiveCommandBuffer();
*/
	inline VkCommandPool GetHandle() const
	{
		check(Handle != VK_NULL_HANDLE);
		return Handle;
	}

#if VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
	TArrayView<VkDescriptorSet> AllocateDescriptorSets(FVulkanCmdBuffer* CmdBuffer, const FVulkanLayout& Layout);
	void ResetDescriptors(FVulkanCmdBuffer* CmdBuffer);
	void SetDescriptorSetsFence(FVulkanCmdBuffer* CmdBuffer, const FVulkanLayout& Layout);
#endif

private:
	FVulkanDevice* Device;
	VkCommandPool Handle;
	//FVulkanCmdBuffer* ActiveCmdBuffer;

#if VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
	TMap<uint32, VulkanRHI::FDescriptorSetsAllocator*> DSAllocators;
#endif

	FVulkanCmdBuffer* Create();

	TArray<FVulkanCmdBuffer*> CmdBuffers;

	void Create(uint32 QueueFamilyIndex);
	friend class FVulkanCommandBufferManager;
};

class FVulkanCommandBufferManager
{
public:
	FVulkanCommandBufferManager(FVulkanDevice* InDevice, FVulkanCommandListContext* InContext);
	~FVulkanCommandBufferManager();

	inline FVulkanCmdBuffer* GetActiveCmdBuffer()
	{
		if (UploadCmdBuffer)
		{
			SubmitUploadCmdBuffer(false);
		}

		return ActiveCmdBuffer;
	}

	inline bool HasPendingUploadCmdBuffer() const
	{
		return UploadCmdBuffer != nullptr;
	}

	inline bool HasPendingActiveCmdBuffer() const
	{
		return ActiveCmdBuffer != nullptr;
	}

	VULKANRHI_API FVulkanCmdBuffer* GetUploadCmdBuffer();

	VULKANRHI_API void SubmitUploadCmdBuffer(bool bWaitForFence);
	void SubmitActiveCmdBuffer(bool bWaitForFence);

	void WaitForCmdBuffer(FVulkanCmdBuffer* CmdBuffer, float TimeInSecondsToWait = 1.0f);

	void RefreshFenceStatus()
	{
		Pool.RefreshFenceStatus();
	}

	void PrepareForNewActiveCommandBuffer();

	inline VkCommandPool GetHandle() const
	{
		return Pool.GetHandle();
	}

	uint32 CalculateGPUTime();

private:
	FVulkanDevice* Device;
	FVulkanCommandBufferPool Pool;
	FVulkanQueue* Queue;
	FVulkanCmdBuffer* ActiveCmdBuffer;
	FVulkanCmdBuffer* UploadCmdBuffer;
};


#if VULKAN_USE_PER_PIPELINE_DESCRIPTOR_POOLS
inline TArrayView<VkDescriptorSet> FVulkanCmdBuffer::AllocateDescriptorSets(const FVulkanLayout& Layout)
{
	return CommandBufferPool->AllocateDescriptorSets(this, Layout);
}

inline void FVulkanCmdBuffer::SetDescriptorSetsFence(const FVulkanLayout& Layout)
{
	CommandBufferPool->SetDescriptorSetsFence(this, Layout);
}
#endif
