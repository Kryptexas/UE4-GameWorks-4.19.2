// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanCommandWrappers.h: Wrap all Vulkan API functions so we can add our own 'layers'
=============================================================================*/

#pragma once 

namespace VulkanRHI
{
#if VULKAN_ENABLE_DUMP_LAYER
	void PrintfBeginResult(const FString& String);
	void DevicePrintfBeginResult(VkDevice Device, const FString& String);
	void PrintfBegin(const FString& String);
	void DevicePrintfBegin(VkDevice Device, const FString& String);
	void PrintResult(VkResult Result);
	void PrintResultAndNamedHandle(VkResult Result, const TCHAR* HandleName, void* Handle);
	void PrintResultAndPointer(VkResult Result, void* Handle);
	void DumpPhysicalDeviceProperties(VkPhysicalDeviceMemoryProperties* Properties);
	void DumpAllocateMemory(VkDevice Device, const VkMemoryAllocateInfo* AllocateInfo, VkDeviceMemory* Memory);
	void DumpMemoryRequirements(VkMemoryRequirements* MemoryRequirements);
	void DumpBufferCreate(VkDevice Device, const VkBufferCreateInfo* CreateInfo, VkBuffer* Buffer);
	void DumpBufferViewCreate(VkDevice Device, const VkBufferViewCreateInfo* CreateInfo, VkBufferView* BufferView);
	void DumpImageCreate(VkDevice Device, const VkImageCreateInfo* CreateInfo);
	void DumpFenceCreate(VkDevice Device, const VkFenceCreateInfo* CreateInfo, VkFence* Fence);
	void DumpFenceList(uint32 FenceCount, const VkFence* Fences);
	void DumpMappedMemoryRanges(uint32 MemoryRangeCount, const VkMappedMemoryRange* pMemoryRanges);
	void DumpResolveImage(VkCommandBuffer CommandBuffer, VkImage SrcImage, VkImageLayout SrcImageLayout, VkImage DstImage, VkImageLayout DstImageLayout, uint32 RegionCount, const VkImageResolve* Regions);
	void DumpFreeDescriptorSets(VkDevice Device, VkDescriptorPool DescriptorPool, uint32 DescriptorSetCount, const VkDescriptorSet* DescriptorSets);
	void DumpCreateInstance(const VkInstanceCreateInfo* CreateInfo, VkInstance* Instance);
	void DumpEnumeratePhysicalDevicesEpilog(uint32* PhysicalDeviceCount, VkPhysicalDevice* PhysicalDevices);
	void DumpCmdPipelineBarrier(VkCommandBuffer CommandBuffer, VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask, VkDependencyFlags DependencyFlags, uint32 MemoryBarrierCount, const VkMemoryBarrier* MemoryBarriers, uint32 BufferMemoryBarrierCount, const VkBufferMemoryBarrier* BufferMemoryBarriers, uint32 ImageMemoryBarrierCount, const VkImageMemoryBarrier* ImageMemoryBarriers);
	void DumpCreateDescriptorSetLayout(VkDevice Device, const VkDescriptorSetLayoutCreateInfo* CreateInfo, VkDescriptorSetLayout* SetLayout);
	void DumpAllocateDescriptorSets(VkDevice Device, const VkDescriptorSetAllocateInfo* AllocateInfo, VkDescriptorSet* DescriptorSets);
	void DumpCreateRenderPass(VkDevice Device, const VkRenderPassCreateInfo* CreateInfo, VkRenderPass* RenderPass);
	void DumpQueueSubmit(VkQueue Queue, uint32 SubmitCount, const VkSubmitInfo* Submits, VkFence Fence);
	void DumpCreateShaderModule(VkDevice Device, const VkShaderModuleCreateInfo* CreateInfo, VkShaderModule* ShaderModule);
#else
	#define DevicePrintfBeginResult(d, x)
	#define PrintfBeginResult(x)
	#define DevicePrintfBegin(d, x)
	#define PrintfBegin(x)
	#define PrintResult(x)
	#define PrintResultAndNamedHandle(r, m_, h)
	#define PrintResultAndPointer(r, h)
	#define DumpPhysicalDeviceProperties(x)
	#define DumpAllocateMemory(d, i, m)
	#define DumpMemoryRequirements(x)
	#define DumpBufferCreate(d, i, b)
	#define DumpBufferViewCreate(d, i, v)
	#define DumpImageCreate(d, x, i)
	#define DumpFenceCreate(d, x, f)
	#define DumpFenceList(c, p)
	#define DumpMappedMemoryRanges(c, p)
	#define DumpResolveImage(c, si, sil, di, dil, rc, r)
	#define DumpFreeDescriptorSets(d, p, c, s)
	#define DumpCreateInstance(c, i)
	#define DumpEnumeratePhysicalDevicesEpilog(c, d)
	#define DumpCmdPipelineBarrier(c, sm, dm, df, mc, mb, bc, bb, ic, ib)
	#define DumpCreateDescriptorSetLayout(d, i, s)
	#define DumpAllocateDescriptorSets(d, i, s)
	#define DumpCreateRenderPass(d, i, r)
	#define DumpQueueSubmit(q, c, s, f)
	#define DumpCreateShaderModule(d, i, s)
#endif

	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateInstance(const VkInstanceCreateInfo* CreateInfo, const VkAllocationCallbacks* Allocator, VkInstance* Instance)
	{
		DumpCreateInstance(CreateInfo, Instance);

		VkResult Result = ::vkCreateInstance(CreateInfo, Allocator, Instance);

		PrintResultAndNamedHandle(Result, TEXT("Instance"), *Instance);
		return Result;
	}

	static FORCEINLINE_DEBUGGABLE void  vkDestroyInstance(VkInstance Instance, const VkAllocationCallbacks* Allocator)
	{
		PrintfBegin(FString::Printf(TEXT("vkDestroyInstance(Instance=%p)"), Instance));

		::vkDestroyInstance(Instance, Allocator);
	}

	static FORCEINLINE_DEBUGGABLE VkResult  vkEnumeratePhysicalDevices(VkInstance Instance, uint32* PhysicalDeviceCount, VkPhysicalDevice* PhysicalDevices)
	{
		PrintfBegin(FString::Printf(TEXT("vkEnumeratePhysicalDevices(Instance=%p, Count=%p, Devices=%p)"), Instance, PhysicalDeviceCount, PhysicalDevices));

		VkResult Result = ::vkEnumeratePhysicalDevices(Instance, PhysicalDeviceCount, PhysicalDevices);

		DumpEnumeratePhysicalDevicesEpilog(PhysicalDeviceCount, PhysicalDevices);

		return Result;
	}
#if 0
	static FORCEINLINE_DEBUGGABLE void  vkGetPhysicalDeviceFeatures(
		VkPhysicalDevice                            physicalDevice,
		VkPhysicalDeviceFeatures*                   pFeatures);

	static FORCEINLINE_DEBUGGABLE void  vkGetPhysicalDeviceFormatProperties(
		VkPhysicalDevice                            physicalDevice,
		VkFormat                                    format,
		VkFormatProperties*                         pFormatProperties);

	static FORCEINLINE_DEBUGGABLE VkResult  vkGetPhysicalDeviceImageFormatProperties(
		VkPhysicalDevice                            physicalDevice,
		VkFormat                                    format,
		VkImageType                                 type,
		VkImageTiling                               tiling,
		VkImageUsageFlags                           usage,
		VkImageCreateFlags                          flags,
		VkImageFormatProperties*                    pImageFormatProperties);

	static FORCEINLINE_DEBUGGABLE void  vkGetPhysicalDeviceProperties(
		VkPhysicalDevice                            physicalDevice,
		VkPhysicalDeviceProperties*                 pProperties);

	static FORCEINLINE_DEBUGGABLE void  vkGetPhysicalDeviceQueueFamilyProperties(
		VkPhysicalDevice                            physicalDevice,
		uint32*                                   pQueueFamilyPropertyCount,
		VkQueueFamilyProperties*                    pQueueFamilyProperties);
#endif
	static FORCEINLINE_DEBUGGABLE void  vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice PhysicalDevice, VkPhysicalDeviceMemoryProperties* MemoryProperties)
	{
		PrintfBegin(FString::Printf(TEXT("vkGetPhysicalDeviceMemoryProperties(OutProp=%p)[...]"), MemoryProperties));

		::vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, MemoryProperties);

		DumpPhysicalDeviceProperties(MemoryProperties);
	}
#if 0
	static FORCEINLINE_DEBUGGABLE PFN_vkVoidFunction  vkGetInstanceProcAddr(
		VkInstance                                  instance,
		const char*                                 pName);

	static FORCEINLINE_DEBUGGABLE PFN_vkVoidFunction  vkGetDeviceProcAddr(
		VkDevice                                    Device,
		const char*                                 pName);

	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateDevice(
		VkPhysicalDevice                            physicalDevice,
		const VkDeviceCreateInfo*                   CreateInfo,
		const VkAllocationCallbacks*                Allocator,
		VkDevice*                                   pDevice);

	static FORCEINLINE_DEBUGGABLE void  vkDestroyDevice(
		VkDevice                                    Device,
		const VkAllocationCallbacks*                Allocator);

	static FORCEINLINE_DEBUGGABLE VkResult  vkEnumerateInstanceExtensionProperties(
		const char*                                 pLayerName,
		uint32*                                   pPropertyCount,
		VkExtensionProperties*                      pProperties);

	static FORCEINLINE_DEBUGGABLE VkResult  vkEnumerateDeviceExtensionProperties(
		VkPhysicalDevice                            physicalDevice,
		const char*                                 pLayerName,
		uint32*                                   pPropertyCount,
		VkExtensionProperties*                      pProperties);

	static FORCEINLINE_DEBUGGABLE VkResult  vkEnumerateInstanceLayerProperties(
		uint32*                                   pPropertyCount,
		VkLayerProperties*                          pProperties);

	static FORCEINLINE_DEBUGGABLE VkResult  vkEnumerateDeviceLayerProperties(
		VkPhysicalDevice                            physicalDevice,
		uint32*                                   pPropertyCount,
		VkLayerProperties*                          pProperties);
#endif
	static FORCEINLINE_DEBUGGABLE void  vkGetDeviceQueue(VkDevice Device, uint32 QueueFamilyIndex, uint32 QueueIndex, VkQueue* Queue)
	{
		DevicePrintfBeginResult(Device, FString::Printf(TEXT("vkGetDeviceQueue(QueueFamilyIndex=%d, QueueIndex=%d, OutQueue=%p)"), QueueFamilyIndex, QueueIndex, Queue));

		::vkGetDeviceQueue(Device, QueueFamilyIndex, QueueIndex, Queue);

		PrintResultAndNamedHandle(VK_SUCCESS, TEXT("Queue"), *Queue);
	}

	static FORCEINLINE_DEBUGGABLE VkResult  vkQueueSubmit(VkQueue Queue, uint32 SubmitCount, const VkSubmitInfo* Submits, VkFence Fence)
	{
		DumpQueueSubmit(Queue, SubmitCount, Submits, Fence);

		VkResult Result = ::vkQueueSubmit(Queue, SubmitCount, Submits, Fence);

		PrintResult(Result);
		return Result;
	}
#if 0
	static FORCEINLINE_DEBUGGABLE VkResult  vkQueueWaitIdle(
		VkQueue                                     queue);

	static FORCEINLINE_DEBUGGABLE VkResult  vkDeviceWaitIdle(
		VkDevice                                    Device);
#endif
	static FORCEINLINE_DEBUGGABLE VkResult  vkAllocateMemory(VkDevice Device, const VkMemoryAllocateInfo* AllocateInfo, const VkAllocationCallbacks* Allocator, VkDeviceMemory* Memory)
	{
		DumpAllocateMemory(Device, AllocateInfo, Memory);

		VkResult Result = ::vkAllocateMemory(Device, AllocateInfo, Allocator, Memory);

		PrintResultAndNamedHandle(Result, TEXT("DevMem"), *Memory);
		return Result;
	}

	static FORCEINLINE_DEBUGGABLE void  vkFreeMemory(VkDevice Device, VkDeviceMemory Memory, const VkAllocationCallbacks* Allocator)
	{
		DevicePrintfBegin(Device, FString::Printf(TEXT("vkFreeMemory(DevMem=%p)"), Memory));

		::vkFreeMemory(Device, Memory, Allocator);
	}

	static FORCEINLINE_DEBUGGABLE VkResult  vkMapMemory(VkDevice Device, VkDeviceMemory Memory, VkDeviceSize Offset, VkDeviceSize Size, VkMemoryMapFlags Flags, void** Data)
	{
		DevicePrintfBeginResult(Device, FString::Printf(TEXT("vkMapMemory(DevMem=%p, Off=%d, Size=%d, Flags=0x%x, OutData=%p)"), Memory, (uint32)Offset, (uint32)Size, Flags, Data));

		VkResult Result = ::vkMapMemory(Device, Memory, Offset, Size, Flags, Data);

		PrintResultAndPointer(Result, *Data);
		return Result;
	}

	static FORCEINLINE_DEBUGGABLE void  vkUnmapMemory(
		VkDevice Device,
		VkDeviceMemory Memory)
	{
		DevicePrintfBegin(Device, FString::Printf(TEXT("vkUnmapMemory(DevMem=%p)"), Memory));
		::vkUnmapMemory(Device, Memory);
	}

	static FORCEINLINE_DEBUGGABLE VkResult  vkFlushMappedMemoryRanges(VkDevice Device, uint32 MemoryRangeCount, const VkMappedMemoryRange* MemoryRanges)
	{
		DumpMappedMemoryRanges(MemoryRangeCount, MemoryRanges);
		DevicePrintfBeginResult(Device, FString::Printf(TEXT("vkFlushMappedMemoryRanges(Count=%d, Ranges=%p)"), MemoryRangeCount, MemoryRanges));

		VkResult Result = ::vkFlushMappedMemoryRanges(Device, MemoryRangeCount, MemoryRanges);

		PrintResult(Result);
		return Result;
	}

	static FORCEINLINE_DEBUGGABLE VkResult  vkInvalidateMappedMemoryRanges(VkDevice Device, uint32 MemoryRangeCount, const VkMappedMemoryRange* MemoryRanges)
	{
		DumpMappedMemoryRanges(MemoryRangeCount, MemoryRanges);
		DevicePrintfBeginResult(Device, FString::Printf(TEXT("vkInvalidateMappedMemoryRanges(Count=%d, Ranges=%p)"), MemoryRangeCount, MemoryRanges));

		VkResult Result = ::vkInvalidateMappedMemoryRanges(Device, MemoryRangeCount, MemoryRanges);

		PrintResult(Result);
		return Result;
	}
#if 0
	static FORCEINLINE_DEBUGGABLE void  vkGetDeviceMemoryCommitment(
		VkDevice                                    Device,
		VkDeviceMemory                              Memory,
		VkDeviceSize*                               pCommittedMemoryInBytes);
#endif
	static FORCEINLINE_DEBUGGABLE VkResult  vkBindBufferMemory(VkDevice Device, VkBuffer Buffer, VkDeviceMemory Memory, VkDeviceSize MemoryOffset)
	{
		DevicePrintfBeginResult(Device, FString::Printf(TEXT("vkBindBufferMemory(Buffer=%p, DevMem=%p, MemOff=%d)"), Buffer, Memory, (uint32)MemoryOffset));

		VkResult Result = ::vkBindBufferMemory(Device, Buffer, Memory, MemoryOffset);

		PrintResult(Result);
		return Result;
	}

	static FORCEINLINE_DEBUGGABLE VkResult  vkBindImageMemory(VkDevice Device, VkImage Image, VkDeviceMemory Memory, VkDeviceSize MemoryOffset)
	{
		DevicePrintfBeginResult(Device, FString::Printf(TEXT("vkBindImageMemory(Image=%p, DevMem=%p, MemOff=%d)"), Image, Memory, (uint32)MemoryOffset));

		VkResult Result = ::vkBindImageMemory(Device, Image, Memory, MemoryOffset);

		PrintResult(Result);
		return Result;
	}

	static FORCEINLINE_DEBUGGABLE void  vkGetBufferMemoryRequirements(VkDevice Device, VkBuffer Buffer, VkMemoryRequirements* MemoryRequirements)
	{
		DevicePrintfBegin(Device, FString::Printf(TEXT("vkGetBufferMemoryRequirements(Buffer=%p, OutReq=%p)"), Buffer, MemoryRequirements));

		::vkGetBufferMemoryRequirements(Device, Buffer, MemoryRequirements);

		DumpMemoryRequirements(MemoryRequirements);
	}
#if 0
	static FORCEINLINE_DEBUGGABLE void  vkGetImageMemoryRequirements(
		VkDevice                                    Device,
		VkImage                                     Image,
		VkMemoryRequirements*                       MemoryRequirements);

	static FORCEINLINE_DEBUGGABLE void  vkGetImageSparseMemoryRequirements(
		VkDevice                                    Device,
		VkImage                                     Image,
		uint32*                                   pSparseMemoryRequirementCount,
		VkSparseImageMemoryRequirements*            pSparseMemoryRequirements);

	static FORCEINLINE_DEBUGGABLE void  vkGetPhysicalDeviceSparseImageFormatProperties(
		VkPhysicalDevice                            physicalDevice,
		VkFormat                                    format,
		VkImageType                                 type,
		VkSampleCountFlagBits                       samples,
		VkImageUsageFlags                           usage,
		VkImageTiling                               tiling,
		uint32*                                   pPropertyCount,
		VkSparseImageFormatProperties*              pProperties);

	static FORCEINLINE_DEBUGGABLE VkResult  vkQueueBindSparse(
		VkQueue                                     queue,
		uint32                                    bindInfoCount,
		const VkBindSparseInfo*                     pBindInfo,
		VkFence                                     Fence);
#endif
	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateFence(VkDevice Device, const VkFenceCreateInfo* CreateInfo, const VkAllocationCallbacks* Allocator, VkFence* Fence)
	{
		DumpFenceCreate(Device, CreateInfo, Fence);

		VkResult Result = ::vkCreateFence(Device, CreateInfo, Allocator, Fence);

		PrintResultAndNamedHandle(Result, TEXT("Fence"), *Fence);
		return Result;
	}

	static FORCEINLINE_DEBUGGABLE void  vkDestroyFence(VkDevice Device, VkFence Fence, const VkAllocationCallbacks* Allocator)
	{
		DevicePrintfBegin(Device, FString::Printf(TEXT("vkDestroyFence(Fence=%p)"), Fence));

		::vkDestroyFence(Device, Fence, Allocator);
	}

	static FORCEINLINE_DEBUGGABLE VkResult  vkResetFences(VkDevice Device, uint32 FenceCount, const VkFence* Fences)
	{
		DevicePrintfBegin(Device, FString::Printf(TEXT("vkResetFences(Count=%d, Fences=%p)"), FenceCount, Fences));
		DumpFenceList(FenceCount, Fences);

		VkResult Result = ::vkResetFences(Device, FenceCount, Fences);

		PrintResult(Result);
		return Result;
	}

	static FORCEINLINE_DEBUGGABLE VkResult  vkGetFenceStatus(VkDevice Device, VkFence Fence)
	{
		DevicePrintfBeginResult(Device, FString::Printf(TEXT("vkGetFenceStatus(Fence=%p)"), Fence));

		VkResult Result = ::vkGetFenceStatus(Device, Fence);

		PrintResult(Result);
		return Result;
	}

	static FORCEINLINE_DEBUGGABLE VkResult  vkWaitForFences(VkDevice Device, uint32 FenceCount, const VkFence* Fences, VkBool32 bWaitAll, uint64_t Timeout)
	{
		DevicePrintfBegin(Device, FString::Printf(TEXT("vkWaitForFences(Count=%p, Fences=%d, WaitAll=%d, Timeout=%p)"), FenceCount, Fences, (uint32)bWaitAll, Timeout));
		DumpFenceList(FenceCount, Fences);

		VkResult Result = ::vkWaitForFences(Device, FenceCount, Fences, bWaitAll, Timeout);

		PrintResult(Result);
		return Result;
	}
#if 0
	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateSemaphore(
		VkDevice                                    Device,
		const VkSemaphoreCreateInfo*                CreateInfo,
		const VkAllocationCallbacks*                Allocator,
		VkSemaphore*                                pSemaphore);

	static FORCEINLINE_DEBUGGABLE void  vkDestroySemaphore(
		VkDevice                                    Device,
		VkSemaphore                                 semaphore,
		const VkAllocationCallbacks*                Allocator);

	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateEvent(
		VkDevice                                    Device,
		const VkEventCreateInfo*                    CreateInfo,
		const VkAllocationCallbacks*                Allocator,
		VkEvent*                                    pEvent);

	static FORCEINLINE_DEBUGGABLE void  vkDestroyEvent(
		VkDevice                                    Device,
		VkEvent                                     event,
		const VkAllocationCallbacks*                Allocator);

	static FORCEINLINE_DEBUGGABLE VkResult  vkGetEventStatus(
		VkDevice                                    Device,
		VkEvent                                     event);

	static FORCEINLINE_DEBUGGABLE VkResult  vkSetEvent(
		VkDevice                                    Device,
		VkEvent                                     event);

	static FORCEINLINE_DEBUGGABLE VkResult  vkResetEvent(
		VkDevice                                    Device,
		VkEvent                                     event);

	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateQueryPool(
		VkDevice                                    Device,
		const VkQueryPoolCreateInfo*                CreateInfo,
		const VkAllocationCallbacks*                Allocator,
		VkQueryPool*                                pQueryPool);

	static FORCEINLINE_DEBUGGABLE void  vkDestroyQueryPool(
		VkDevice                                    Device,
		VkQueryPool                                 queryPool,
		const VkAllocationCallbacks*                Allocator);

	static FORCEINLINE_DEBUGGABLE VkResult  vkGetQueryPoolResults(
		VkDevice                                    Device,
		VkQueryPool                                 queryPool,
		uint32                                    firstQuery,
		uint32                                    queryCount,
		size_t                                      dataSize,
		void*                                       pData,
		VkDeviceSize                                stride,
		VkQueryResultFlags                          flags);
#endif
	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateBuffer(VkDevice Device, const VkBufferCreateInfo* CreateInfo, const VkAllocationCallbacks* Allocator, VkBuffer* Buffer)
	{
		DumpBufferCreate(Device, CreateInfo, Buffer);

		VkResult Result = ::vkCreateBuffer(Device, CreateInfo, Allocator, Buffer);

		PrintResultAndNamedHandle(Result, TEXT("Buffer"), *Buffer);
		return Result;
	}

	static FORCEINLINE_DEBUGGABLE void  vkDestroyBuffer(VkDevice Device, VkBuffer Buffer, const VkAllocationCallbacks* Allocator)
	{
		DevicePrintfBegin(Device, FString::Printf(TEXT("vkDestroyBuffer(Buffer=%p)"), Buffer));

		::vkDestroyBuffer(Device, Buffer, Allocator);
	}

	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateBufferView(VkDevice Device, const VkBufferViewCreateInfo* CreateInfo, const VkAllocationCallbacks* Allocator, VkBufferView* View)
	{
		DumpBufferViewCreate(Device, CreateInfo, View);
		DevicePrintfBeginResult(Device, FString::Printf(TEXT("vkCreateBufferView(Info=%p, OutView=%p)"), CreateInfo, View));

		VkResult Result = ::vkCreateBufferView(Device, CreateInfo, Allocator, View);

		PrintResultAndNamedHandle(Result, TEXT("BufferView"), *View);
		return Result;
	}

	static FORCEINLINE_DEBUGGABLE void  vkDestroyBufferView(VkDevice Device, VkBufferView BufferView, const VkAllocationCallbacks* Allocator)
	{
		DevicePrintfBegin(Device, FString::Printf(TEXT("vkDestroyBufferView(BufferView=%p)"), BufferView));

		::vkDestroyBufferView(Device, BufferView, Allocator);
	}
#if 0
	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateImage(
		VkDevice                                    Device,
		const VkImageCreateInfo*                    CreateInfo,
		const VkAllocationCallbacks*                Allocator,
		VkImage*                                    pImage);

	static FORCEINLINE_DEBUGGABLE void  vkDestroyImage(
		VkDevice                                    Device,
		VkImage                                     Image,
		const VkAllocationCallbacks*                Allocator);

	static FORCEINLINE_DEBUGGABLE void  vkGetImageSubresourceLayout(
		VkDevice                                    Device,
		VkImage                                     Image,
		const VkImageSubresource*                   pSubresource,
		VkSubresourceLayout*                        pLayout);

	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateImageView(
		VkDevice                                    Device,
		const VkImageViewCreateInfo*                CreateInfo,
		const VkAllocationCallbacks*                Allocator,
		VkImageView*                                pView);

	static FORCEINLINE_DEBUGGABLE void  vkDestroyImageView(
		VkDevice                                    Device,
		VkImageView                                 imageView,
		const VkAllocationCallbacks*                Allocator);
#endif
	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateShaderModule(
		VkDevice Device,
		const VkShaderModuleCreateInfo* CreateInfo,
		const VkAllocationCallbacks* Allocator,
		VkShaderModule* ShaderModule)
	{
		DumpCreateShaderModule(Device, CreateInfo, ShaderModule);
		VkResult Result = ::vkCreateShaderModule(Device, CreateInfo, Allocator,ShaderModule);

		PrintResultAndNamedHandle(Result, TEXT("ShaderModule"), *ShaderModule);

		return Result;
	}

	static FORCEINLINE_DEBUGGABLE void  vkDestroyShaderModule(VkDevice Device, VkShaderModule ShaderModule, const VkAllocationCallbacks* Allocator)
	{
		DevicePrintfBegin(Device, FString::Printf(TEXT("vkDestroyShaderModule(ShaderModule=%p)"), ShaderModule));

		::vkDestroyShaderModule(Device, ShaderModule, Allocator);
	}

#if 0
	static FORCEINLINE_DEBUGGABLE VkResult  vkCreatePipelineCache(
		VkDevice                                    Device,
		const VkPipelineCacheCreateInfo*            CreateInfo,
		const VkAllocationCallbacks*                Allocator,
		VkPipelineCache*                            pPipelineCache);

	static FORCEINLINE_DEBUGGABLE void  vkDestroyPipelineCache(
		VkDevice                                    Device,
		VkPipelineCache                             pipelineCache,
		const VkAllocationCallbacks*                Allocator);

	static FORCEINLINE_DEBUGGABLE VkResult  vkGetPipelineCacheData(
		VkDevice                                    Device,
		VkPipelineCache                             pipelineCache,
		size_t*                                     pDataSize,
		void*                                       pData);

	static FORCEINLINE_DEBUGGABLE VkResult  vkMergePipelineCaches(
		VkDevice                                    Device,
		VkPipelineCache                             dstCache,
		uint32                                    srcCacheCount,
		const VkPipelineCache*                      pSrcCaches);

	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateGraphicsPipelines(
		VkDevice                                    Device,
		VkPipelineCache                             pipelineCache,
		uint32                                    createInfoCount,
		const VkGraphicsPipelineCreateInfo*         pCreateInfos,
		const VkAllocationCallbacks*                Allocator,
		VkPipeline*                                 pPipelines);

	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateComputePipelines(
		VkDevice                                    Device,
		VkPipelineCache                             pipelineCache,
		uint32                                    createInfoCount,
		const VkComputePipelineCreateInfo*          pCreateInfos,
		const VkAllocationCallbacks*                Allocator,
		VkPipeline*                                 pPipelines);

	static FORCEINLINE_DEBUGGABLE void  vkDestroyPipeline(
		VkDevice                                    Device,
		VkPipeline                                  pipeline,
		const VkAllocationCallbacks*                Allocator);

	static FORCEINLINE_DEBUGGABLE VkResult  vkCreatePipelineLayout(
		VkDevice                                    Device,
		const VkPipelineLayoutCreateInfo*           CreateInfo,
		const VkAllocationCallbacks*                Allocator,
		VkPipelineLayout*                           pPipelineLayout);

	static FORCEINLINE_DEBUGGABLE void  vkDestroyPipelineLayout(
		VkDevice                                    Device,
		VkPipelineLayout                            pipelineLayout,
		const VkAllocationCallbacks*                Allocator);

	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateSampler(
		VkDevice                                    Device,
		const VkSamplerCreateInfo*                  CreateInfo,
		const VkAllocationCallbacks*                Allocator,
		VkSampler*                                  pSampler);

	static FORCEINLINE_DEBUGGABLE void  vkDestroySampler(
		VkDevice                                    Device,
		VkSampler                                   sampler,
		const VkAllocationCallbacks*                Allocator);
#endif
	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateDescriptorSetLayout(VkDevice Device, const VkDescriptorSetLayoutCreateInfo* CreateInfo, const VkAllocationCallbacks* Allocator, VkDescriptorSetLayout* SetLayout)
	{
		DumpCreateDescriptorSetLayout(Device, CreateInfo, SetLayout);

		VkResult Result = ::vkCreateDescriptorSetLayout(Device, CreateInfo, Allocator, SetLayout);

		PrintResultAndNamedHandle(Result, TEXT("DescriptorSetLayout"), *SetLayout);
		return Result;
	}

	static FORCEINLINE_DEBUGGABLE void  vkDestroyDescriptorSetLayout(VkDevice Device, VkDescriptorSetLayout DescriptorSetLayout, const VkAllocationCallbacks* Allocator)
	{
		DevicePrintfBegin(Device, FString::Printf(TEXT("vkDestroyDescriptorSetLayout(DescriptorSetLayout=%p)"), DescriptorSetLayout));

		::vkDestroyDescriptorSetLayout(Device, DescriptorSetLayout, Allocator);
	}
#if 0
	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateDescriptorPool(
		VkDevice                                    Device,
		const VkDescriptorPoolCreateInfo*           CreateInfo,
		const VkAllocationCallbacks*                Allocator,
		VkDescriptorPool*                           pDescriptorPool);

	static FORCEINLINE_DEBUGGABLE void  vkDestroyDescriptorPool(
		VkDevice                                    Device,
		VkDescriptorPool                            descriptorPool,
		const VkAllocationCallbacks*                Allocator);

	static FORCEINLINE_DEBUGGABLE VkResult  vkResetDescriptorPool(
		VkDevice                                    Device,
		VkDescriptorPool                            descriptorPool,
		VkDescriptorPoolResetFlags                  flags);
#endif
	static FORCEINLINE_DEBUGGABLE VkResult  vkAllocateDescriptorSets(VkDevice Device, const VkDescriptorSetAllocateInfo* AllocateInfo, VkDescriptorSet* DescriptorSets)
	{
		DumpAllocateDescriptorSets(Device, AllocateInfo, DescriptorSets);

		VkResult Result = ::vkAllocateDescriptorSets(Device, AllocateInfo, DescriptorSets);

		PrintResultAndNamedHandle(Result, TEXT("DescriptorSet"), *DescriptorSets);
		return Result;
	}

	static FORCEINLINE_DEBUGGABLE VkResult  vkFreeDescriptorSets(VkDevice Device, VkDescriptorPool DescriptorPool, uint32 DescriptorSetCount, const VkDescriptorSet* DescriptorSets)
	{
		DumpFreeDescriptorSets(Device, DescriptorPool, DescriptorSetCount, DescriptorSets);

		VkResult Result = ::vkFreeDescriptorSets(Device, DescriptorPool, DescriptorSetCount, DescriptorSets);

		PrintResult(Result);
		return Result;
	}
#if 0
	static FORCEINLINE_DEBUGGABLE void  vkUpdateDescriptorSets(
		VkDevice                                    Device,
		uint32                                    descriptorWriteCount,
		const VkWriteDescriptorSet*                 pDescriptorWrites,
		uint32                                    descriptorCopyCount,
		const VkCopyDescriptorSet*                  pDescriptorCopies);

	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateFramebuffer(
		VkDevice                                    Device,
		const VkFramebufferCreateInfo*              CreateInfo,
		const VkAllocationCallbacks*                Allocator,
		VkFramebuffer*                              pFramebuffer);

	static FORCEINLINE_DEBUGGABLE void  vkDestroyFramebuffer(
		VkDevice                                    Device,
		VkFramebuffer                               framebuffer,
		const VkAllocationCallbacks*                Allocator);
#endif
	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateRenderPass(VkDevice Device, const VkRenderPassCreateInfo* CreateInfo, const VkAllocationCallbacks* Allocator, VkRenderPass* RenderPass)
	{
		DumpCreateRenderPass(Device, CreateInfo, RenderPass);
		VkResult Result = ::vkCreateRenderPass(Device, CreateInfo, Allocator, RenderPass);

		PrintResultAndNamedHandle(Result, TEXT("RenderPass"), *RenderPass);
		return Result;
	}

	static FORCEINLINE_DEBUGGABLE void  vkDestroyRenderPass(VkDevice Device, VkRenderPass RenderPass, const VkAllocationCallbacks* Allocator)
	{
		DevicePrintfBegin(Device, FString::Printf(TEXT("vkDestroyRenderPass(RenderPass=%p)"), RenderPass));

		::vkDestroyRenderPass(Device, RenderPass, Allocator);
	}
#if 0
	static FORCEINLINE_DEBUGGABLE void  vkGetRenderAreaGranularity(
		VkDevice                                    Device,
		VkRenderPass                                renderPass,
		VkExtent2D*                                 pGranularity);

	static FORCEINLINE_DEBUGGABLE VkResult  vkCreateCommandPool(
		VkDevice                                    Device,
		const VkCommandPoolCreateInfo*              CreateInfo,
		const VkAllocationCallbacks*                Allocator,
		VkCommandPool*                              pCommandPool);

	static FORCEINLINE_DEBUGGABLE void  vkDestroyCommandPool(
		VkDevice                                    Device,
		VkCommandPool                               commandPool,
		const VkAllocationCallbacks*                Allocator);

	static FORCEINLINE_DEBUGGABLE VkResult  vkResetCommandPool(
		VkDevice                                    Device,
		VkCommandPool                               commandPool,
		VkCommandPoolResetFlags                     flags);

	static FORCEINLINE_DEBUGGABLE VkResult  vkAllocateCommandBuffers(
		VkDevice                                    Device,
		const VkCommandBufferAllocateInfo*          pAllocateInfo,
		VkCommandBuffer*                            pCommandBuffers);

	static FORCEINLINE_DEBUGGABLE void  vkFreeCommandBuffers(
		VkDevice                                    Device,
		VkCommandPool                               commandPool,
		uint32                                    commandBufferCount,
		const VkCommandBuffer*                      pCommandBuffers);

	static FORCEINLINE_DEBUGGABLE VkResult  vkBeginCommandBuffer(
		VkCommandBuffer                             commandBuffer,
		const VkCommandBufferBeginInfo*             pBeginInfo);

	static FORCEINLINE_DEBUGGABLE VkResult  vkEndCommandBuffer(
		VkCommandBuffer                             commandBuffer);

	static FORCEINLINE_DEBUGGABLE VkResult  vkResetCommandBuffer(
		VkCommandBuffer                             commandBuffer,
		VkCommandBufferResetFlags                   flags);

	static FORCEINLINE_DEBUGGABLE void  vkCmdBindPipeline(
		VkCommandBuffer                             commandBuffer,
		VkPipelineBindPoint                         pipelineBindPoint,
		VkPipeline                                  pipeline);

	static FORCEINLINE_DEBUGGABLE void  vkCmdSetViewport(
		VkCommandBuffer                             commandBuffer,
		uint32                                    firstViewport,
		uint32                                    viewportCount,
		const VkViewport*                           pViewports);

	static FORCEINLINE_DEBUGGABLE void  vkCmdSetScissor(
		VkCommandBuffer                             commandBuffer,
		uint32                                    firstScissor,
		uint32                                    scissorCount,
		const VkRect2D*                             pScissors);

	static FORCEINLINE_DEBUGGABLE void  vkCmdSetLineWidth(
		VkCommandBuffer                             commandBuffer,
		float                                       lineWidth);

	static FORCEINLINE_DEBUGGABLE void  vkCmdSetDepthBias(
		VkCommandBuffer                             commandBuffer,
		float                                       depthBiasConstantFactor,
		float                                       depthBiasClamp,
		float                                       depthBiasSlopeFactor);

	static FORCEINLINE_DEBUGGABLE void  vkCmdSetBlendConstants(
		VkCommandBuffer                             commandBuffer,
		const float                                 blendConstants[4]);

	static FORCEINLINE_DEBUGGABLE void  vkCmdSetDepthBounds(
		VkCommandBuffer                             commandBuffer,
		float                                       minDepthBounds,
		float                                       maxDepthBounds);

	static FORCEINLINE_DEBUGGABLE void  vkCmdSetStencilCompareMask(
		VkCommandBuffer                             commandBuffer,
		VkStencilFaceFlags                          faceMask,
		uint32                                    compareMask);

	static FORCEINLINE_DEBUGGABLE void  vkCmdSetStencilWriteMask(
		VkCommandBuffer                             commandBuffer,
		VkStencilFaceFlags                          faceMask,
		uint32                                    writeMask);

	static FORCEINLINE_DEBUGGABLE void  vkCmdSetStencilReference(
		VkCommandBuffer                             commandBuffer,
		VkStencilFaceFlags                          faceMask,
		uint32                                    reference);

	static FORCEINLINE_DEBUGGABLE void  vkCmdBindDescriptorSets(
		VkCommandBuffer                             commandBuffer,
		VkPipelineBindPoint                         pipelineBindPoint,
		VkPipelineLayout                            layout,
		uint32                                    firstSet,
		uint32                                    descriptorSetCount,
		const VkDescriptorSet*                      pDescriptorSets,
		uint32                                    dynamicOffsetCount,
		const uint32*                             pDynamicOffsets);

	static FORCEINLINE_DEBUGGABLE void  vkCmdBindIndexBuffer(
		VkCommandBuffer                             commandBuffer,
		VkBuffer                                    Buffer,
		VkDeviceSize                                offset,
		VkIndexType                                 indexType);

	static FORCEINLINE_DEBUGGABLE void  vkCmdBindVertexBuffers(
		VkCommandBuffer                             commandBuffer,
		uint32                                    firstBinding,
		uint32                                    bindingCount,
		const VkBuffer*                             pBuffers,
		const VkDeviceSize*                         pOffsets);

	static FORCEINLINE_DEBUGGABLE void  vkCmdDraw(
		VkCommandBuffer                             commandBuffer,
		uint32                                    vertexCount,
		uint32                                    instanceCount,
		uint32                                    firstVertex,
		uint32                                    firstInstance);

	static FORCEINLINE_DEBUGGABLE void  vkCmdDrawIndexed(
		VkCommandBuffer                             commandBuffer,
		uint32                                    indexCount,
		uint32                                    instanceCount,
		uint32                                    firstIndex,
		int32_t                                     vertexOffset,
		uint32                                    firstInstance);

	static FORCEINLINE_DEBUGGABLE void  vkCmdDrawIndirect(
		VkCommandBuffer                             commandBuffer,
		VkBuffer                                    Buffer,
		VkDeviceSize                                offset,
		uint32                                    drawCount,
		uint32                                    stride);

	static FORCEINLINE_DEBUGGABLE void  vkCmdDrawIndexedIndirect(
		VkCommandBuffer                             commandBuffer,
		VkBuffer                                    Buffer,
		VkDeviceSize                                offset,
		uint32                                    drawCount,
		uint32                                    stride);

	static FORCEINLINE_DEBUGGABLE void  vkCmdDispatch(
		VkCommandBuffer                             commandBuffer,
		uint32                                    x,
		uint32                                    y,
		uint32                                    z);

	static FORCEINLINE_DEBUGGABLE void  vkCmdDispatchIndirect(
		VkCommandBuffer                             commandBuffer,
		VkBuffer                                    Buffer,
		VkDeviceSize                                offset);

	static FORCEINLINE_DEBUGGABLE void  vkCmdCopyBuffer(
		VkCommandBuffer                             commandBuffer,
		VkBuffer                                    srcBuffer,
		VkBuffer                                    dstBuffer,
		uint32                                    regionCount,
		const VkBufferCopy*                         pRegions);

	static FORCEINLINE_DEBUGGABLE void  vkCmdCopyImage(
		VkCommandBuffer                             commandBuffer,
		VkImage                                     srcImage,
		VkImageLayout                               srcImageLayout,
		VkImage                                     dstImage,
		VkImageLayout                               dstImageLayout,
		uint32                                    regionCount,
		const VkImageCopy*                          pRegions);

	static FORCEINLINE_DEBUGGABLE void  vkCmdBlitImage(
		VkCommandBuffer                             commandBuffer,
		VkImage                                     srcImage,
		VkImageLayout                               srcImageLayout,
		VkImage                                     dstImage,
		VkImageLayout                               dstImageLayout,
		uint32                                    regionCount,
		const VkImageBlit*                          pRegions,
		VkFilter                                    filter);

	static FORCEINLINE_DEBUGGABLE void  vkCmdCopyBufferToImage(
		VkCommandBuffer                             commandBuffer,
		VkBuffer                                    srcBuffer,
		VkImage                                     dstImage,
		VkImageLayout                               dstImageLayout,
		uint32                                    regionCount,
		const VkBufferImageCopy*                    pRegions);

	static FORCEINLINE_DEBUGGABLE void  vkCmdCopyImageToBuffer(
		VkCommandBuffer                             commandBuffer,
		VkImage                                     srcImage,
		VkImageLayout                               srcImageLayout,
		VkBuffer                                    dstBuffer,
		uint32                                    regionCount,
		const VkBufferImageCopy*                    pRegions);

	static FORCEINLINE_DEBUGGABLE void  vkCmdUpdateBuffer(
		VkCommandBuffer                             commandBuffer,
		VkBuffer                                    dstBuffer,
		VkDeviceSize                                dstOffset,
		VkDeviceSize                                dataSize,
		const uint32*                             pData);

	static FORCEINLINE_DEBUGGABLE void  vkCmdFillBuffer(
		VkCommandBuffer                             commandBuffer,
		VkBuffer                                    dstBuffer,
		VkDeviceSize                                dstOffset,
		VkDeviceSize                                size,
		uint32                                    data);

	static FORCEINLINE_DEBUGGABLE void  vkCmdClearColorImage(
		VkCommandBuffer                             commandBuffer,
		VkImage                                     Image,
		VkImageLayout                               imageLayout,
		const VkClearColorValue*                    pColor,
		uint32                                    rangeCount,
		const VkImageSubresourceRange*              pRanges);

	static FORCEINLINE_DEBUGGABLE void  vkCmdClearDepthStencilImage(
		VkCommandBuffer                             commandBuffer,
		VkImage                                     Image,
		VkImageLayout                               imageLayout,
		const VkClearDepthStencilValue*             pDepthStencil,
		uint32                                    rangeCount,
		const VkImageSubresourceRange*              pRanges);

	static FORCEINLINE_DEBUGGABLE void  vkCmdClearAttachments(
		VkCommandBuffer                             commandBuffer,
		uint32                                    attachmentCount,
		const VkClearAttachment*                    pAttachments,
		uint32                                    rectCount,
		const VkClearRect*                          pRects);
#endif
	static FORCEINLINE_DEBUGGABLE void  vkCmdResolveImage(
		VkCommandBuffer CommandBuffer,
		VkImage SrcImage, VkImageLayout SrcImageLayout,
		VkImage DstImage, VkImageLayout DstImageLayout,
		uint32 RegionCount, const VkImageResolve* Regions)
	{
		DumpResolveImage(CommandBuffer, SrcImage, SrcImageLayout, DstImage, DstImageLayout, RegionCount, Regions);

		::vkCmdResolveImage(CommandBuffer, SrcImage, SrcImageLayout, DstImage, DstImageLayout, RegionCount, Regions);
	}
#if 0
	static FORCEINLINE_DEBUGGABLE void  vkCmdSetEvent(
		VkCommandBuffer                             commandBuffer,
		VkEvent                                     event,
		VkPipelineStageFlags                        stageMask);

	static FORCEINLINE_DEBUGGABLE void  vkCmdResetEvent(
		VkCommandBuffer                             commandBuffer,
		VkEvent                                     event,
		VkPipelineStageFlags                        stageMask);

	static FORCEINLINE_DEBUGGABLE void  vkCmdWaitEvents(
		VkCommandBuffer                             commandBuffer,
		uint32                                    eventCount,
		const VkEvent*                              pEvents,
		VkPipelineStageFlags                        srcStageMask,
		VkPipelineStageFlags                        dstStageMask,
		uint32                                    memoryBarrierCount,
		const VkMemoryBarrier*                      pMemoryBarriers,
		uint32                                    bufferMemoryBarrierCount,
		const VkBufferMemoryBarrier*                pBufferMemoryBarriers,
		uint32                                    imageMemoryBarrierCount,
		const VkImageMemoryBarrier*                 pImageMemoryBarriers);
#endif
	static FORCEINLINE_DEBUGGABLE void  vkCmdPipelineBarrier(
		VkCommandBuffer CommandBuffer, VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask, VkDependencyFlags DependencyFlags,
		uint32 MemoryBarrierCount, const VkMemoryBarrier* MemoryBarriers,
		uint32 BufferMemoryBarrierCount, const VkBufferMemoryBarrier* BufferMemoryBarriers,
		uint32 ImageMemoryBarrierCount, const VkImageMemoryBarrier* ImageMemoryBarriers)
	{
		DumpCmdPipelineBarrier(CommandBuffer, SrcStageMask, DstStageMask, DependencyFlags, MemoryBarrierCount, MemoryBarriers, BufferMemoryBarrierCount, BufferMemoryBarriers, ImageMemoryBarrierCount, ImageMemoryBarriers);

		::vkCmdPipelineBarrier(CommandBuffer, SrcStageMask, DstStageMask, DependencyFlags, MemoryBarrierCount, MemoryBarriers, BufferMemoryBarrierCount, BufferMemoryBarriers, ImageMemoryBarrierCount, ImageMemoryBarriers);
	}
#if 0
	static FORCEINLINE_DEBUGGABLE void  vkCmdBeginQuery(
		VkCommandBuffer                             commandBuffer,
		VkQueryPool                                 queryPool,
		uint32                                    query,
		VkQueryControlFlags                         flags);

	static FORCEINLINE_DEBUGGABLE void  vkCmdEndQuery(
		VkCommandBuffer                             commandBuffer,
		VkQueryPool                                 queryPool,
		uint32                                    query);

	static FORCEINLINE_DEBUGGABLE void  vkCmdResetQueryPool(
		VkCommandBuffer                             commandBuffer,
		VkQueryPool                                 queryPool,
		uint32                                    firstQuery,
		uint32                                    queryCount);

	static FORCEINLINE_DEBUGGABLE void  vkCmdWriteTimestamp(
		VkCommandBuffer                             commandBuffer,
		VkPipelineStageFlagBits                     pipelineStage,
		VkQueryPool                                 queryPool,
		uint32                                    query);

	static FORCEINLINE_DEBUGGABLE void  vkCmdCopyQueryPoolResults(
		VkCommandBuffer                             commandBuffer,
		VkQueryPool                                 queryPool,
		uint32                                    firstQuery,
		uint32                                    queryCount,
		VkBuffer                                    dstBuffer,
		VkDeviceSize                                dstOffset,
		VkDeviceSize                                stride,
		VkQueryResultFlags                          flags);

	static FORCEINLINE_DEBUGGABLE void  vkCmdPushConstants(
		VkCommandBuffer                             commandBuffer,
		VkPipelineLayout                            layout,
		VkShaderStageFlags                          stageFlags,
		uint32                                    offset,
		uint32                                    size,
		const void*                                 pValues);

	static FORCEINLINE_DEBUGGABLE void  vkCmdBeginRenderPass(
		VkCommandBuffer                             commandBuffer,
		const VkRenderPassBeginInfo*                pRenderPassBegin,
		VkSubpassContents                           contents);

	static FORCEINLINE_DEBUGGABLE void  vkCmdNextSubpass(
		VkCommandBuffer                             commandBuffer,
		VkSubpassContents                           contents);

	static FORCEINLINE_DEBUGGABLE void  vkCmdEndRenderPass(
		VkCommandBuffer                             commandBuffer);

	static FORCEINLINE_DEBUGGABLE void  vkCmdExecuteCommands(
		VkCommandBuffer                             commandBuffer,
		uint32                                    commandBufferCount,
		const VkCommandBuffer*                      pCommandBuffers);
#endif

#if VULKAN_ENABLE_DUMP_LAYER
	void FlushDebugWrapperLog();
#endif
}
