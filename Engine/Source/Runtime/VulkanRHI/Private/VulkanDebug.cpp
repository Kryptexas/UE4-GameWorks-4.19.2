// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanDebug.cpp: Vulkan device RHI implementation.
=============================================================================*/

#include "VulkanRHIPrivate.h"

#if VK_HEADER_VERSION < 8 && (VK_API_VERSION < VK_MAKE_VERSION(1, 0, 3))
#include <vulkan/vk_ext_debug_report.h>
#endif

#define VULKAN_ENABLE_API_DUMP_DETAILED				0

#define CREATE_MSG_CALLBACK							"vkCreateDebugReportCallbackEXT"
#define DESTROY_MSG_CALLBACK						"vkDestroyDebugReportCallbackEXT"

DEFINE_LOG_CATEGORY(LogVulkanRHI);

#if VULKAN_HAS_DEBUGGING_ENABLED

static VkBool32 VKAPI_PTR DebugReportFunction(
	VkDebugReportFlagsEXT			MsgFlags,
	VkDebugReportObjectTypeEXT		ObjType,
    uint64_t						SrcObject,
    size_t							Location,
    int32							MsgCode,
	const ANSICHAR*					LayerPrefix,
	const ANSICHAR*					Msg,
    void*							UserData)
{
	if (MsgFlags != VK_DEBUG_REPORT_ERROR_BIT_EXT && 
		MsgFlags != VK_DEBUG_REPORT_WARNING_BIT_EXT &&
		MsgFlags != VK_DEBUG_REPORT_INFORMATION_BIT_EXT &&
		MsgFlags != VK_DEBUG_REPORT_DEBUG_BIT_EXT)
	{
		ensure(0);
	}

	if (MsgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		// Reaching this line should trigger a break/assert. 
		// Check to see if this is a code we've seen before.
		FString LayerCode = FString::Printf(TEXT("%s%d"), ANSI_TO_TCHAR(LayerPrefix), MsgCode);
		static TSet<FString> SeenCodes;
		if (!SeenCodes.Contains(LayerCode))
		{
			FString Message = FString::Printf(TEXT("VK ERROR: [%s] Code %d : %s"), ANSI_TO_TCHAR(LayerPrefix), MsgCode, ANSI_TO_TCHAR(Msg));
			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("VulkanRHI: %s\n"), *Message);
			UE_LOG(LogVulkanRHI, Error, TEXT("%s"), *Message);
    
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
			if (!FCStringAnsi::Strcmp(LayerPrefix, "MEM"))
			{
				((FVulkanDynamicRHI*)GDynamicRHI)->DumpMemory();
			}
#endif

			// Break debugger on first instance of each message. 
			// Continuing will ignore the error and suppress this message in future.
			bool bIgnoreInFuture = true;
			ensureAlways(0);
			if (bIgnoreInFuture)
			{
				SeenCodes.Add(LayerCode);
			}
		}
		return VK_FALSE;
	}
	else if (MsgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		FString Message = FString::Printf(TEXT("VK WARNING: [%s] Code %d : %s\n"), ANSI_TO_TCHAR(LayerPrefix), MsgCode, ANSI_TO_TCHAR(Msg));
		FPlatformMisc::LowLevelOutputDebugStringf(TEXT("VulkanRHI: %s\n"), *Message);
		UE_LOG(LogVulkanRHI, Warning, TEXT("%s"), *Message);
		return VK_FALSE;
	}
#if VULKAN_ENABLE_API_DUMP
	else if (MsgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
	{
#if !VULKAN_ENABLE_API_DUMP_DETAILED
		if (!FCStringAnsi::Strcmp(LayerPrefix, "MEM") || !FCStringAnsi::Strcmp(LayerPrefix, "DS"))
		{
			// Skip Mem messages
		}
		else
#endif
		{
			FString Message = FString::Printf(TEXT("VK INFO: [%s] Code %d : %s\n"), ANSI_TO_TCHAR(LayerPrefix), MsgCode, ANSI_TO_TCHAR(Msg));
			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("VulkanRHI: %s\n"), *Message);
			UE_LOG(LogVulkanRHI, Display, TEXT("%s"), *Message);
		}

		return VK_FALSE;
	}
#if VULKAN_ENABLE_API_DUMP_DETAILED
	else if (MsgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
	{
		FString Message = FString::Printf(TEXT("VK DEBUG: [%s] Code %d : %s\n"), ANSI_TO_TCHAR(LayerPrefix), MsgCode, ANSI_TO_TCHAR(Msg));
		FPlatformMisc::LowLevelOutputDebugStringf(TEXT("VulkanRHI: %s\n"), *Message);
		UE_LOG(LogVulkanRHI, Display, TEXT("%s"), *Message);
		return VK_FALSE;
	}
#endif
#endif

	return VK_TRUE;
}

void FVulkanDynamicRHI::SetupDebugLayerCallback()
{
#if !VULKAN_DISABLE_DEBUG_CALLBACK
	PFN_vkCreateDebugReportCallbackEXT CreateMsgCallback = (PFN_vkCreateDebugReportCallbackEXT)(void*)vkGetInstanceProcAddr(Instance, CREATE_MSG_CALLBACK);
	if (CreateMsgCallback)
	{
		VkDebugReportCallbackCreateInfoEXT CreateInfo;
		FMemory::Memzero(CreateInfo);
		CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
		CreateInfo.pfnCallback = DebugReportFunction;
		CreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
#if VULKAN_ENABLE_API_DUMP
		CreateInfo.flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
#if VULKAN_ENABLE_API_DUMP_DETAILED
		CreateInfo.flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
#endif
#endif
		VkResult Result = CreateMsgCallback(Instance, &CreateInfo, nullptr, &MsgCallback);
		switch (Result)
		{
		case VK_SUCCESS:
			break;
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			UE_LOG(LogVulkanRHI, Warning, TEXT("CreateMsgCallback: out of host memory/CreateMsgCallback Failure; debug reporting skipped"));
			break;
		default:
			UE_LOG(LogVulkanRHI, Warning, TEXT("CreateMsgCallback: unknown failure %d/CreateMsgCallback Failure; debug reporting skipped"), (int32)Result);
			break;
		}
	}
	else
	{
		UE_LOG(LogVulkanRHI, Warning, TEXT("GetProcAddr: Unable to find vkDbgCreateMsgCallback/vkGetInstanceProcAddr; debug reporting skipped!"));
	}
#endif
}

void FVulkanDynamicRHI::RemoveDebugLayerCallback()
{
#if !VULKAN_DISABLE_DEBUG_CALLBACK
	if (MsgCallback != VK_NULL_HANDLE)
	{
		PFN_vkDestroyDebugReportCallbackEXT DestroyMsgCallback = (PFN_vkDestroyDebugReportCallbackEXT)(void*)vkGetInstanceProcAddr(Instance, DESTROY_MSG_CALLBACK);
		checkf(DestroyMsgCallback, TEXT("GetProcAddr: Unable to find vkDbgCreateMsgCallback\vkGetInstanceProcAddr Failure"));
		DestroyMsgCallback(Instance, MsgCallback, nullptr);
	}
#endif
}


#if VULKAN_ENABLE_DUMP_LAYER
namespace VulkanRHI
{
	static FString DebugLog;
	static int32 DebugLine = 1;

	static const TCHAR* TenTabs = TEXT("\t\t\t\t\t\t\t\t\t\t");

	void FlushDebugWrapperLog()
	{
		if (DebugLog.Len() > 0)
		{
			GLog->Flush();
			UE_LOG(LogVulkanRHI, Display, TEXT("Vulkan Wrapper Log:\n%s"), *DebugLog);
			GLog->Flush();
			DebugLog = TEXT("");
		}
	}

	static FString GetVkResultErrorString(VkResult Result)
	{
		FString ErrorString;
		switch (Result)
		{
#define VKSWITCHCASE(x)	case x: ErrorString = TEXT(#x); break;
			VKSWITCHCASE(VK_SUCCESS)
			VKSWITCHCASE(VK_NOT_READY)
			VKSWITCHCASE(VK_TIMEOUT)
			VKSWITCHCASE(VK_EVENT_SET)
			VKSWITCHCASE(VK_EVENT_RESET)
			VKSWITCHCASE(VK_INCOMPLETE)
			VKSWITCHCASE(VK_ERROR_OUT_OF_HOST_MEMORY)
			VKSWITCHCASE(VK_ERROR_OUT_OF_DEVICE_MEMORY)
			VKSWITCHCASE(VK_ERROR_INITIALIZATION_FAILED)
			VKSWITCHCASE(VK_ERROR_DEVICE_LOST)
			VKSWITCHCASE(VK_ERROR_MEMORY_MAP_FAILED)
			VKSWITCHCASE(VK_ERROR_LAYER_NOT_PRESENT)
			VKSWITCHCASE(VK_ERROR_EXTENSION_NOT_PRESENT)
			VKSWITCHCASE(VK_ERROR_FEATURE_NOT_PRESENT)
			VKSWITCHCASE(VK_ERROR_INCOMPATIBLE_DRIVER)
			VKSWITCHCASE(VK_ERROR_TOO_MANY_OBJECTS)
			VKSWITCHCASE(VK_ERROR_FORMAT_NOT_SUPPORTED)
			VKSWITCHCASE(VK_ERROR_SURFACE_LOST_KHR)
			VKSWITCHCASE(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
			VKSWITCHCASE(VK_SUBOPTIMAL_KHR)
			VKSWITCHCASE(VK_ERROR_OUT_OF_DATE_KHR)
			VKSWITCHCASE(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)
			VKSWITCHCASE(VK_ERROR_VALIDATION_FAILED_EXT)
			VKSWITCHCASE(VK_ERROR_INVALID_SHADER_NV)
#undef VKSWITCHCASE
		default:
			ErrorString = FString::Printf(TEXT("Unknown VkResult %d"), (int32)Result);
			break;
		}

		return ErrorString;
	}

	static FString GetImageLayoutString(VkImageLayout Layout)
	{
		FString String;
		switch (Layout)
		{
#define VKSWITCHCASE(x)	case x: String = TEXT(#x); break;
			VKSWITCHCASE(VK_IMAGE_LAYOUT_UNDEFINED)
			VKSWITCHCASE(VK_IMAGE_LAYOUT_GENERAL)
			VKSWITCHCASE(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			VKSWITCHCASE(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			VKSWITCHCASE(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
			VKSWITCHCASE(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			VKSWITCHCASE(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			VKSWITCHCASE(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			VKSWITCHCASE(VK_IMAGE_LAYOUT_PREINITIALIZED)
			VKSWITCHCASE(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
#undef VKSWITCHCASE
		default:
			String = FString::Printf(TEXT("Unknown VkImageLayout %d"), (int32)Layout);
			break;
		}

		return String;
	}

	static FString GetStageMaskString(VkPipelineStageFlags Flags)
	{
		return FString::Printf(TEXT("VkPipelineStageFlags=0x%x"), (uint32)Flags);
	}

	void PrintfBeginResult(const FString& String)
	{
		DebugLog += FString::Printf(TEXT("[GLOBAL METHOD]   %8d: %s"), DebugLine++, *String);
	}

	void PrintfBegin(const FString& String)
	{
		DebugLog += FString::Printf(TEXT("[GLOBAL METHOD]   %8d: %s\n"), DebugLine++, *String);
	}

	void DevicePrintfBeginResult(VkDevice Device, const FString& String)
	{
		DebugLog += FString::Printf(TEXT("[%p]%8d: %s"), Device, DebugLine++, *String);
	}

	void DevicePrintfBegin(VkDevice Device, const FString& String)
	{
		DebugLog += FString::Printf(TEXT("[%p]%8d: %s\n"), Device, DebugLine++, *String);
	}

	void PrintResult(VkResult Result)
	{
		DebugLog += FString::Printf(TEXT(" -> %s\n"), *GetVkResultErrorString(Result));
	}

	void PrintResultAndPointer(VkResult Result, void* Handle)
	{
		DebugLog += FString::Printf(TEXT(" -> %s => %p\n"), *GetVkResultErrorString(Result), Handle);
	}

	void PrintResultAndNamedHandle(VkResult Result, const TCHAR* HandleName, void* Handle)
	{
		DebugLog += FString::Printf(TEXT(" -> %s => %s=%p\n"), *GetVkResultErrorString(Result), HandleName, Handle);
	}

	void DumpPhysicalDeviceProperties(VkPhysicalDeviceMemoryProperties* Properties)
	{
		DebugLog += FString::Printf(TEXT(" -> VkPhysicalDeviceMemoryProperties[...]\n"));
	}

	void DumpAllocateMemory(VkDevice Device, const VkMemoryAllocateInfo* AllocateInfo, VkDeviceMemory* Memory)
	{
		DevicePrintfBeginResult(Device, FString::Printf(TEXT("vkAllocateMemory(OutMem=%p)\n"), AllocateInfo, Memory));
		DebugLog += TenTabs[3];
		DebugLog += FString::Printf(TEXT("VkMemoryAllocateInfo %p: Size=%d, MemoryTypeIndex=%d"), AllocateInfo, (uint32)AllocateInfo->allocationSize, AllocateInfo->memoryTypeIndex);
	}

	void DumpMemoryRequirements(VkMemoryRequirements* MemoryRequirements)
	{
		DebugLog += TenTabs[3];
		DebugLog += FString::Printf(TEXT("VkMemoryRequirements: Size=%d Align=%d MemoryTypeBits=0x%x\n"), (uint32)MemoryRequirements->size, (uint32)MemoryRequirements->alignment, MemoryRequirements->memoryTypeBits);
	}

	void DumpBufferCreate(VkDevice Device, const VkBufferCreateInfo* CreateInfo, VkBuffer* Buffer)
	{
		DevicePrintfBeginResult(Device, FString::Printf(TEXT("vkCreateBuffer(Info=%p, OutBuffer=%p)[...]"), CreateInfo, Buffer));
	}

	void DumpBufferViewCreate(VkDevice Device, const VkBufferViewCreateInfo* CreateInfo, VkBufferView* BufferView)
	{
		DevicePrintfBeginResult(Device, FString::Printf(TEXT("VkBufferViewCreateInfo(Info=%p, OutBufferView=%p)[...]"), CreateInfo, BufferView));
	}

	void DumpImageCreate(VkDevice Device, const VkImageCreateInfo* CreateInfo)
	{
		ensure(0);
	}

	void DumpFenceCreate(VkDevice Device, const VkFenceCreateInfo* CreateInfo, VkFence* Fence)
	{
		DevicePrintfBeginResult(Device, FString::Printf(TEXT("vkCreateFence(CreateInfo=%p, OutFence=%p)[...]"), CreateInfo, Fence));
	}
	
	void DumpFenceList(uint32 FenceCount, const VkFence* Fences)
	{
		for (uint32 Index = 0; Index < 0; ++Index)
		{
			DebugLog += TenTabs[2];
			DebugLog += FString::Printf(TEXT("Fence[%d]=%p\n"), Index, Fences[Index]);
		}
	}
	
	void DumpMappedMemoryRanges(uint32 memoryRangeCount, const VkMappedMemoryRange* MemoryRanges)
	{
		ensure(0);
	}

	void DumpResolveImage(VkCommandBuffer CommandBuffer, VkImage SrcImage, VkImageLayout SrcImageLayout, VkImage DstImage, VkImageLayout DstImageLayout, uint32 RegionCount, const VkImageResolve* Regions)
	{
		PrintfBeginResult(FString::Printf(TEXT("vkCmdResolveImage(CmdBuffer=%p, SrcImage=%p, SrcImageLayout=%s, DestImage=%p, DestImageLayout=%s, NumRegions=%d, Regions=%p)[...]"),
			CommandBuffer, SrcImage, *GetImageLayoutString(SrcImageLayout), DstImage, *GetImageLayoutString(DstImageLayout), RegionCount, Regions));
		for (uint32 Index = 0; Index < RegionCount; ++Index)
		{
			DebugLog += TenTabs[3];
			DebugLog += FString::Printf(TEXT("Region %d: "), Index);
/*
			typedef struct VkImageResolve {
				VkImageSubresourceLayers    srcSubresource;
				VkOffset3D                  srcOffset;
				VkImageSubresourceLayers    dstSubresource;
				VkOffset3D                  dstOffset;
				VkExtent3D                  extent;

*/
		}
	}

	void DumpFreeDescriptorSets(VkDevice Device, VkDescriptorPool DescriptorPool, uint32 DescriptorSetCount, const VkDescriptorSet* DescriptorSets)
	{
		DevicePrintfBegin(Device, FString::Printf(TEXT("vkFreeDescriptorSets(Pool=%p, Count=%d, Sets=%p)"), DescriptorPool, DescriptorSetCount, DescriptorSets));
		for (uint32 Index = 0; Index < DescriptorSetCount; ++Index)
		{
			DebugLog += TenTabs[3];
			DebugLog += FString::Printf(TEXT("Set %d: %p\n"), Index, DescriptorSets[Index]);
		}
	}

	void DumpCreateInstance(const VkInstanceCreateInfo* CreateInfo, VkInstance* Instance)
	{
		PrintfBegin(FString::Printf(TEXT("vkCreateInstance(Info=%p, OutInstance=%p)[...]"), CreateInfo, Instance));
	}

	void DumpEnumeratePhysicalDevicesEpilog(uint32* PhysicalDeviceCount, VkPhysicalDevice* PhysicalDevices)
	{
		if (PhysicalDeviceCount)
		{
			DebugLog += TenTabs[3];
			DebugLog += FString::Printf(TEXT("OutCount=%d\n"), *PhysicalDeviceCount);
			if (PhysicalDevices)
			{
				for (uint32 Index = 0; Index < *PhysicalDeviceCount; ++Index)
				{
					DebugLog += TenTabs[2];
					DebugLog += FString::Printf(TEXT("OutDevice[%d]=%p\n"), Index, PhysicalDevices[Index]);
				}
			}
		}
	}

	void DumpCmdPipelineBarrier(VkCommandBuffer CommandBuffer, VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask, VkDependencyFlags DependencyFlags, uint32 MemoryBarrierCount, const VkMemoryBarrier* MemoryBarriers, uint32 BufferMemoryBarrierCount, const VkBufferMemoryBarrier* BufferMemoryBarriers, uint32 ImageMemoryBarrierCount, const VkImageMemoryBarrier* ImageMemoryBarriers)
	{
		PrintfBegin(FString::Printf(TEXT("vkCmdPipelineBarrier(CmdBuffer=%p, SrcMask=%s, DestMask=%s, Flags=%d, MemCount=%d, Mem=%p, BufferCount=%d, Buffers=%p, ImageCount=%d, Images=%p)[...]"), CommandBuffer, *GetStageMaskString(SrcStageMask), *GetStageMaskString(DstStageMask), (uint32)DependencyFlags, MemoryBarrierCount, MemoryBarriers, BufferMemoryBarrierCount, BufferMemoryBarriers, ImageMemoryBarrierCount, ImageMemoryBarriers));
	}

	void DumpCreateDescriptorSetLayout(VkDevice Device, const VkDescriptorSetLayoutCreateInfo* CreateInfo, VkDescriptorSetLayout* SetLayout)
	{
		DevicePrintfBegin(Device, FString::Printf(TEXT("vkCreateDescriptorSetLayout(Info=%p, OutLayout=%p)[...]"), CreateInfo, SetLayout));
	}

	void DumpAllocateDescriptorSets(VkDevice Device, const VkDescriptorSetAllocateInfo* AllocateInfo, VkDescriptorSet* DescriptorSets)
	{
		DevicePrintfBegin(Device, FString::Printf(TEXT("vkAllocateDescriptorSets(Info=%p, OutSets=%p)[...]"), AllocateInfo, DescriptorSets));
	}

	void DumpCreateRenderPass(VkDevice Device, const VkRenderPassCreateInfo* CreateInfo, VkRenderPass* RenderPass)
	{
		DevicePrintfBegin(Device, FString::Printf(TEXT("vkCreateRenderPass(Info=%p, OutRenderPass=%p)[...]"), CreateInfo, RenderPass));
	}

	void DumpQueueSubmit(VkQueue Queue, uint32 SubmitCount, const VkSubmitInfo* Submits, VkFence Fence)
	{
		// Probably good point to submit
		FlushDebugWrapperLog();

		PrintfBeginResult(FString::Printf(TEXT("vkQueueSubmit(Queue=%p, Count=%d, Submits=%p, Fence=%p)[...]"), Queue, SubmitCount, Submits, Fence));
	}

	void DumpCreateShaderModule(VkDevice Device, const VkShaderModuleCreateInfo* CreateInfo, VkShaderModule* ShaderModule)
	{
		DevicePrintfBeginResult(Device, FString::Printf(TEXT("vkCreateShaderModule(CreateInfo=%p, OutShaderModule=%p)[...]"), CreateInfo, ShaderModule));
	}

	static void HandleFlushWrapperLog(const TArray<FString>& Args)
	{
		FlushDebugWrapperLog();
	}

	static FAutoConsoleCommand CVarRHIThreadEnable(
		TEXT("r.Vulkan.FlushLog"),
		TEXT("\n"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&HandleFlushWrapperLog)
		);

	static struct FGlobalDumpLog
	{
		~FGlobalDumpLog()
		{
			FlushDebugWrapperLog();
		}
	} GGlobalDumpLogInstance;
}
#endif	// VULKAN_ENABLE_DUMP_LAYER
#endif // VULKAN_HAS_DEBUGGING_ENABLED
