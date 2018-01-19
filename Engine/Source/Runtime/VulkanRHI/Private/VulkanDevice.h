// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved..

/*=============================================================================
	VulkanDevice.h: Private Vulkan RHI definitions.
=============================================================================*/

#pragma once

#include "VulkanMemory.h"

class FOLDVulkanDescriptorPool;
#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
class FVulkanDescriptorPoolsManager;
#endif
class FVulkanCommandListContext;
class FVulkanOcclusionQueryPool;
class FOLDVulkanQueryPool;

class FVulkanDevice
{
public:
	FVulkanDevice(VkPhysicalDevice Gpu);

	~FVulkanDevice();

	// Returns true if this is a viable candidate for main GPU
	bool QueryGPU(int32 DeviceIndex);

	void InitGPU(int32 DeviceIndex);

	void CreateDevice();

	void PrepareForDestroy();
	void Destroy();

	void WaitUntilIdle();

	inline FVulkanQueue* GetGraphicsQueue()
	{
		return GfxQueue;
	}

	inline FVulkanQueue* GetComputeQueue()
	{
		return ComputeQueue;
	}

	inline FVulkanQueue* GetTransferQueue()
	{
		return TransferQueue;
	}

	inline FVulkanQueue* GetPresentQueue()
	{
		return PresentQueue;
	}

	inline VkPhysicalDevice GetPhysicalHandle() const
	{
		return Gpu;
	}

	inline const VkPhysicalDeviceProperties& GetDeviceProperties() const
	{
		return GpuProps;
	}

	inline const VkPhysicalDeviceLimits& GetLimits() const
	{
		return GpuProps.limits;
	}

#if VULKAN_ENABLE_DESKTOP_HMD_SUPPORT
	inline const VkPhysicalDeviceIDPropertiesKHR& GetDeviceIdProperties() const
	{
		check(GetOptionalExtensions().HasKHRGetPhysicalDeviceProperties2);
		return GpuIdProps;
	}
#endif

	inline const VkPhysicalDeviceFeatures& GetFeatures() const
	{
		return Features;
	}

	inline bool HasUnifiedMemory() const
	{
		return MemoryManager.HasUnifiedMemory();
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

	inline VkImageView GetDefaultImageView() const
	{
		return DefaultImageView;
	}

	inline const VkFormatProperties* GetFormatProperties() const
	{
		return FormatProperties;
	}

	inline VulkanRHI::FDeviceMemoryManager& GetMemoryManager()
	{
		return MemoryManager;
	}

	inline VulkanRHI::FResourceHeapManager& GetResourceHeapManager()
	{
		return ResourceHeapManager;
	}

	inline VulkanRHI::FDeferredDeletionQueue& GetDeferredDeletionQueue()
	{
		return DeferredDeletionQueue;
	}

	inline VulkanRHI::FStagingManager& GetStagingManager()
	{
		return StagingManager;
	}

	inline VulkanRHI::FFenceManager& GetFenceManager()
	{
		return FenceManager;
	}

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	inline FVulkanDescriptorPoolsManager& GetDescriptorPoolsManager()
	{
		return *DescriptorPoolsManager;
	}
#endif

	inline FVulkanCommandListContext& GetImmediateContext()
	{
		return *ImmediateContext;
	}

	inline FVulkanCommandListContext& GetImmediateComputeContext()
	{
		return *ComputeContext;
	}

	void NotifyDeletedRenderTarget(VkImage Image);
	void NotifyDeletedImage(VkImage Image);

#if VULKAN_ENABLE_DRAW_MARKERS
	PFN_vkCmdDebugMarkerBeginEXT GetCmdDbgMarkerBegin() const
	{
		return CmdDbgMarkerBegin;
	}

	PFN_vkCmdDebugMarkerEndEXT GetCmdDbgMarkerEnd() const
	{
		return CmdDbgMarkerEnd;
	}

	PFN_vkDebugMarkerSetObjectNameEXT GetDebugMarkerSetObjectName() const
	{
		return DebugMarkerSetObjectName;
	}
#endif

	void PrepareForCPURead();

	void SubmitCommandsAndFlushGPU();

	inline FOLDVulkanBufferedQueryPool& FindAvailableQueryPool(TArray<FOLDVulkanBufferedQueryPool*>& Pools, VkQueryType QueryType)
	{
		// First try to find An available one
		for (int32 Index = 0; Index < Pools.Num(); ++Index)
		{
			FOLDVulkanBufferedQueryPool* Pool = Pools[Index];
			if (Pool->HasRoom())
			{
				return *Pool;
			}
		}

		// None found, so allocate new Pool
		FOLDVulkanBufferedQueryPool* Pool = new FOLDVulkanBufferedQueryPool(this, QueryType == VK_QUERY_TYPE_OCCLUSION ? NUM_OCCLUSION_QUERIES_PER_POOL : NUM_TIMESTAMP_QUERIES_PER_POOL, QueryType);
		Pools.Add(Pool);
		return *Pool;
	}
	inline FOLDVulkanBufferedQueryPool& FindAvailableOcclusionQueryPool()
	{
		return FindAvailableQueryPool(OcclusionQueryPools, VK_QUERY_TYPE_OCCLUSION);
	}

	inline FOLDVulkanBufferedQueryPool& FindAvailableTimestampQueryPool()
	{
		return FindAvailableQueryPool(TimestampQueryPools, VK_QUERY_TYPE_TIMESTAMP);
	}

	inline class FVulkanPipelineStateCache* GetPipelineStateCache()
	{
		return PipelineStateCache;
	}

	void NotifyDeletedGfxPipeline(class FVulkanGraphicsPipelineState* Pipeline);
	void NotifyDeletedComputePipeline(class FVulkanComputePipeline* Pipeline);

	FVulkanCommandListContext* AcquireDeferredContext();
	void ReleaseDeferredContext(FVulkanCommandListContext* InContext);

	struct FOptionalVulkanDeviceExtensions
	{
		uint32 HasKHRMaintenance1 : 1;
		uint32 HasMirrorClampToEdge : 1;
		uint32 HasKHRExternalMemoryCapabilities : 1;
		uint32 HasKHRGetPhysicalDeviceProperties2 : 1;
	};
	inline const FOptionalVulkanDeviceExtensions& GetOptionalExtensions() const { return OptionalDeviceExtensions;  }

	void SetupPresentQueue(const VkSurfaceKHR& Surface);

private:
	void MapFormatSupport(EPixelFormat UEFormat, VkFormat VulkanFormat);
	void MapFormatSupport(EPixelFormat UEFormat, VkFormat VulkanFormat, int32 BlockBytes);
	void SetComponentMapping(EPixelFormat UEFormat, VkComponentSwizzle r, VkComponentSwizzle g, VkComponentSwizzle b, VkComponentSwizzle a);

	void SubmitCommands(FVulkanCommandListContext* Context);

	VkPhysicalDevice Gpu;
	VkPhysicalDeviceProperties GpuProps;
#if VULKAN_ENABLE_DESKTOP_HMD_SUPPORT
	VkPhysicalDeviceIDPropertiesKHR GpuIdProps;
#endif
	VkPhysicalDeviceFeatures Features;
	
	VkDevice Device;

	VulkanRHI::FDeviceMemoryManager MemoryManager;

	VulkanRHI::FResourceHeapManager ResourceHeapManager;

	VulkanRHI::FDeferredDeletionQueue DeferredDeletionQueue;

	VulkanRHI::FStagingManager StagingManager;

	VulkanRHI::FFenceManager FenceManager;

#if VULKAN_USE_DESCRIPTOR_POOL_MANAGER
	FVulkanDescriptorPoolsManager* DescriptorPoolsManager = nullptr;
#endif

	FVulkanSamplerState* DefaultSampler;
	FVulkanSurface* DefaultImage;
	VkImageView DefaultImageView;

	TArray<VkQueueFamilyProperties> QueueFamilyProps;
	VkFormatProperties FormatProperties[VK_FORMAT_RANGE_SIZE];
	// Info for formats that are not in the core Vulkan spec (i.e. extensions)
	mutable TMap<VkFormat, VkFormatProperties> ExtensionFormatProperties;

	TArray<FOLDVulkanBufferedQueryPool*> OcclusionQueryPools;
	TArray<FOLDVulkanBufferedQueryPool*> TimestampQueryPools;

	FVulkanQueue* GfxQueue;
	FVulkanQueue* ComputeQueue;
	FVulkanQueue* TransferQueue;
	FVulkanQueue* PresentQueue;

	VkComponentMapping PixelFormatComponentMapping[PF_MAX];

	FVulkanCommandListContext* ImmediateContext;
	FVulkanCommandListContext* ComputeContext;
	TArray<FVulkanCommandListContext*> CommandContexts;

	void GetDeviceExtensions(TArray<const ANSICHAR*>& OutDeviceExtensions, TArray<const ANSICHAR*>& OutDeviceLayers, bool& bOutDebugMarkers);

	void ParseOptionalDeviceExtensions(const TArray<const ANSICHAR*>& DeviceExtensions);
	FOptionalVulkanDeviceExtensions OptionalDeviceExtensions;

	void SetupFormats();

#if VULKAN_ENABLE_DRAW_MARKERS
	PFN_vkCmdDebugMarkerBeginEXT CmdDbgMarkerBegin;
	PFN_vkCmdDebugMarkerEndEXT CmdDbgMarkerEnd;
	PFN_vkDebugMarkerSetObjectNameEXT DebugMarkerSetObjectName;
	friend class FVulkanCommandListContext;
#endif

	class FVulkanPipelineStateCache* PipelineStateCache;
	friend class FVulkanDynamicRHI;
};
