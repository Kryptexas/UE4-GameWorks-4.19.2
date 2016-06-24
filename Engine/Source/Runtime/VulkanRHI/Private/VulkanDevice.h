// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved..

/*=============================================================================
	VulkanDevice.h: Private Vulkan RHI definitions.
=============================================================================*/

#pragma once

#include "VulkanMemory.h"

class FVulkanDescriptorPool;
class FVulkanCommandListContext;

class FVulkanDevice
{
public:
	enum
	{
		NumTimestampPools = 3,	// Must be the same size as the number of the backbuffer images
	};

	FVulkanDevice(VkPhysicalDevice Gpu);

	~FVulkanDevice();

	// Returns true if this is a viable candidate for main GPU
	bool QueryGPU(int32 DeviceIndex);

	void InitGPU(int32 DeviceIndex);

	void CreateDevice();

	void PrepareForDestroy();
	void Destroy();

	void WaitUntilIdle();

	inline FVulkanQueue* GetQueue()
	{
		check(Queue);
		return Queue;
	}

	inline VkPhysicalDevice GetPhysicalHandle()
	{
		check(Gpu != VK_NULL_HANDLE);
		return Gpu;
	}

	inline FVulkanRingBuffer* GetUBRingBuffer()
	{
		return UBRingBuffer;
	}

	inline const VkPhysicalDeviceProperties& GetDeviceProperties() const
	{
		return GpuProps;
	}

	inline const VkPhysicalDeviceLimits& GetLimits() const
	{
		return GpuProps.limits;
	}

	inline const VkPhysicalDeviceFeatures& GetFeatures() const
	{
		return Features;
	}

	bool IsFormatSupported(VkFormat Format) const;

	const VkComponentMapping& GetFormatComponentMapping(EPixelFormat UEFormat) const;
	
	inline VkDevice GetInstanceHandle()
	{
		check(Device != VK_NULL_HANDLE);
		return Device;
	}

	inline VkSampler GetDefaultSampler() const
	{
		return DefaultSampler->Sampler;
	}

	inline FVulkanTimestampQueryPool* GetTimestampQueryPool(uint32 Index)
	{
		check(Index < NumTimestampPools);
		return TimestampQueryPool[Index];
	}

	inline const VkFormatProperties* GetFormatProperties() const
	{
		return FormatProperties;
	}

	VulkanRHI::FDeviceMemoryManager& GetMemoryManager()
	{
		return MemoryManager;
	}

	VulkanRHI::FResourceHeapManager& GetResourceHeapManager()
	{
		return ResourceHeapManager;
	}

	VulkanRHI::FDeferredDeletionQueue& GetDeferredDeletionQueue()
	{
		return DeferredDeletionQueue;
	}

	VulkanRHI::FStagingManager& GetStagingManager()
	{
		return StagingManager;
	}

	FVulkanDescriptorPool* GetDescriptorPool()
	{
		return DescriptorPool;
	}

	VulkanRHI::FFenceManager& GetFenceManager()
	{
		return FenceManager;
	}

	FVulkanCommandListContext& GetImmediateContext()
	{
		return *ImmediateContext;
	}

	void NotifyDeletedRenderTarget(const FVulkanTextureBase* Texture);

#if VULKAN_ENABLE_DRAW_MARKERS

	typedef void(VKAPI_PTR *PFN_vkCmdDbgMarkerBegin)(VkCommandBuffer commandBuffer, const char *pMarker);
	typedef void(VKAPI_PTR *PFN_vkCmdDbgMarkerEnd)(VkCommandBuffer commandBuffer);

	PFN_vkCmdDbgMarkerBegin GetCmdDbgMarkerBegin() const
	{
		return VkCmdDbgMarkerBegin;
	}

	PFN_vkCmdDbgMarkerEnd GetCmdDbgMarkerEnd() const
	{
		return VkCmdDbgMarkerEnd;
	}
#endif

private:
	void MapFormatSupport(EPixelFormat UEFormat, VkFormat VulkanFormat);
	void MapFormatSupport(EPixelFormat UEFormat, VkFormat VulkanFormat, int32 BlockBytes);
	void SetComponentMapping(EPixelFormat UEFormat, VkComponentSwizzle r, VkComponentSwizzle g, VkComponentSwizzle b, VkComponentSwizzle a);

	VkPhysicalDevice Gpu;
	VkPhysicalDeviceProperties GpuProps;
	VkPhysicalDeviceFeatures Features;
	
	VkDevice Device;

	VulkanRHI::FDeviceMemoryManager MemoryManager;

	VulkanRHI::FResourceHeapManager ResourceHeapManager;

	VulkanRHI::FDeferredDeletionQueue DeferredDeletionQueue;

	VulkanRHI::FStagingManager StagingManager;

	VulkanRHI::FFenceManager FenceManager;

	FVulkanDescriptorPool* DescriptorPool;

	FVulkanSamplerState* DefaultSampler;

	TArray<VkQueueFamilyProperties> QueueFamilyProps;
	VkFormatProperties FormatProperties[VK_FORMAT_RANGE_SIZE];
	// Info for formats that are not in the core Vulkan spec (i.e. extensions)
	mutable TMap<VkFormat, VkFormatProperties> ExtensionFormatProperties;

	// Nullptr if not supported
	FVulkanTimestampQueryPool* TimestampQueryPool[NumTimestampPools];

	FVulkanQueue* Queue;

	VkComponentMapping PixelFormatComponentMapping[PF_MAX];

	FVulkanCommandListContext* ImmediateContext;

	FVulkanRingBuffer* UBRingBuffer;

	void GetDeviceExtensions(TArray<const ANSICHAR*>& OutDeviceExtensions, TArray<const ANSICHAR*>& OutDeviceLayers, bool& bOutDebugMarkers);
	void SetupFormats();

#if VULKAN_ENABLE_DRAW_MARKERS
	PFN_vkCmdDbgMarkerBegin VkCmdDbgMarkerBegin;
	PFN_vkCmdDbgMarkerEnd VkCmdDbgMarkerEnd;
	friend class FVulkanCommandListContext;
#endif

public:
	uint64 FrameCounter;

#if VULKAN_ENABLE_PIPELINE_CACHE
	class FVulkanPipelineStateCache* PipelineStateCache;
#endif
};
