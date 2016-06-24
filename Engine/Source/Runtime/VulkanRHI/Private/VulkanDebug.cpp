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
			FString Message = FString::Printf(TEXT("VulkanRHI: VK ERROR: [%s] Code %d : %s\n"), ANSI_TO_TCHAR(LayerPrefix), MsgCode, ANSI_TO_TCHAR(Msg));
			FPlatformMisc::LowLevelOutputDebugStringf(*Message);
    
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
		FString Message = FString::Printf(TEXT("VulkanRHI: VK WARNING: [%s] Code %d : %s\n"), ANSI_TO_TCHAR(LayerPrefix), MsgCode, ANSI_TO_TCHAR(Msg));
		FPlatformMisc::LowLevelOutputDebugString(*Message);
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
			FString Message = FString::Printf(TEXT("VulkanRHI: VK INFO: [%s] Code %d : %s\n"), ANSI_TO_TCHAR(LayerPrefix), MsgCode, ANSI_TO_TCHAR(Msg));
			FPlatformMisc::LowLevelOutputDebugString(*Message);
		}

		return VK_FALSE;
	}
#if VULKAN_ENABLE_API_DUMP_DETAILED
	else if (MsgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
	{
		FString Message = FString::Printf(TEXT("VulkanRHI: VK DEBUG: [%s] Code %d : %s\n"), ANSI_TO_TCHAR(LayerPrefix), MsgCode, ANSI_TO_TCHAR(Msg));
		FPlatformMisc::LowLevelOutputDebugString(*Message);
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

#endif // VULKAN_HAS_DEBUGGING_ENABLED
