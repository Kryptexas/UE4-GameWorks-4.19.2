// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanSwapChain.h: Vulkan viewport RHI definitions.
=============================================================================*/

#include "VulkanRHIPrivate.h"
#include "VulkanSwapChain.h"
#include "VulkanPlatform.h"

int32 GShouldCpuWaitForFence = 1;
static FAutoConsoleVariableRef CVarCpuWaitForFence(
	TEXT("r.vulkan.CpuWaitForFence"),
	GShouldCpuWaitForFence,
	TEXT("Whether to have the Cpu wait for the fence in AcquireImageIndex"),
	ECVF_RenderThreadSafe
);


extern FAutoConsoleVariable GCVarDelayAcquireBackBuffer;

FVulkanSwapChain::FVulkanSwapChain(VkInstance InInstance, FVulkanDevice& InDevice, void* WindowHandle, EPixelFormat& InOutPixelFormat, uint32 Width, uint32 Height,
	uint32* InOutDesiredNumBackBuffers, TArray<VkImage>& OutImages)
	: SwapChain(VK_NULL_HANDLE)
	, Device(InDevice)
	, Surface(VK_NULL_HANDLE)
	, CurrentImageIndex(-1)
	, SemaphoreIndex(0)
	, NumPresentCalls(0)
	, NumAcquireCalls(0)
	, Instance(InInstance)
{
	// let the platform create the surface
	FVulkanPlatform::CreateSurface(WindowHandle, Instance, &Surface);

	// Find Pixel format for presentable images
	VkSurfaceFormatKHR CurrFormat;
	FMemory::Memzero(CurrFormat);
	{
		uint32 NumFormats;
		VERIFYVULKANRESULT_EXPANDED(VulkanRHI::vkGetPhysicalDeviceSurfaceFormatsKHR(Device.GetPhysicalHandle(), Surface, &NumFormats, nullptr));
		check(NumFormats > 0);

		TArray<VkSurfaceFormatKHR> Formats;
		Formats.AddZeroed(NumFormats);
		VERIFYVULKANRESULT_EXPANDED(VulkanRHI::vkGetPhysicalDeviceSurfaceFormatsKHR(Device.GetPhysicalHandle(), Surface, &NumFormats, Formats.GetData()));

		if (InOutPixelFormat != PF_Unknown)
		{
			bool bFound = false;
			if (GPixelFormats[InOutPixelFormat].Supported)
			{
				VkFormat Requested = (VkFormat)GPixelFormats[InOutPixelFormat].PlatformFormat;
				for (int32 Index = 0; Index < Formats.Num(); ++Index)
				{
					if (Formats[Index].format == Requested)
					{
						CurrFormat = Formats[Index];
						bFound = true;
						break;
					}
				}

				if (!bFound)
				{
					UE_LOG(LogVulkanRHI, Warning, TEXT("Requested PixelFormat %d not supported by this swapchain! Falling back to supported swapchain formats..."), (uint32)InOutPixelFormat);
					InOutPixelFormat = PF_Unknown;
				}
			}
			else
			{
				UE_LOG(LogVulkanRHI, Warning, TEXT("Requested PixelFormat %d not supported by this Vulkan implementation!"), (uint32)InOutPixelFormat);
				InOutPixelFormat = PF_Unknown;
			}
		}

		if (InOutPixelFormat == PF_Unknown)
		{
			for (int32 Index = 0; Index < Formats.Num(); ++Index)
			{
				// Reverse lookup
				check(Formats[Index].format != VK_FORMAT_UNDEFINED);
				for (int32 PFIndex = 0; PFIndex < PF_MAX; ++PFIndex)
				{
					if (Formats[Index].format == GPixelFormats[PFIndex].PlatformFormat)
					{
						InOutPixelFormat = (EPixelFormat)PFIndex;
						CurrFormat = Formats[Index];
						UE_LOG(LogVulkanRHI, Verbose, TEXT("No swapchain format requested, picking up VulkanFormat %d"), (uint32)CurrFormat.format);
						break;
					}
				}

				if (InOutPixelFormat != PF_Unknown)
				{
					break;
				}
			}
		}

		if (InOutPixelFormat == PF_Unknown)
		{
			UE_LOG(LogVulkanRHI, Warning, TEXT("Can't find a proper pixel format for the swapchain, trying to pick up the first available"));
			VkFormat PlatformFormat = UEToVkFormat(InOutPixelFormat, false);
			bool bSupported = false;
			for (int32 Index = 0; Index < Formats.Num(); ++Index)
			{
				if (Formats[Index].format == PlatformFormat)
				{
					bSupported = true;
					CurrFormat = Formats[Index];
					break;
				}
			}

			check(bSupported);
		}

		if (InOutPixelFormat == PF_Unknown)
		{
			FString Msg;
			for (int32 Index = 0; Index < Formats.Num(); ++Index)
			{
				if (Index == 0)
				{
					Msg += TEXT("(");
				}
				else
				{
					Msg += TEXT(", ");
				}
				Msg += FString::Printf(TEXT("%d"), (int32)Formats[Index].format);
			}
			if (Formats.Num())
			{
				Msg += TEXT(")");
			}
			UE_LOG(LogVulkanRHI, Fatal, TEXT("Unable to find a pixel format for the swapchain; swapchain returned %d Vulkan formats %s"), Formats.Num(), *Msg);
		}
	}

	VkFormat PlatformFormat = UEToVkFormat(InOutPixelFormat, false);

	Device.SetupPresentQueue(Surface);

	// Fetch present mode
	VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;
	if (FVulkanPlatform::SupportsQuerySurfaceProperties())
	{
		uint32 NumFoundPresentModes = 0;
		VERIFYVULKANRESULT(VulkanRHI::vkGetPhysicalDeviceSurfacePresentModesKHR(Device.GetPhysicalHandle(), Surface, &NumFoundPresentModes, nullptr));
		check(NumFoundPresentModes > 0);

		TArray<VkPresentModeKHR> FoundPresentModes;
		FoundPresentModes.AddZeroed(NumFoundPresentModes);
		VERIFYVULKANRESULT(VulkanRHI::vkGetPhysicalDeviceSurfacePresentModesKHR(Device.GetPhysicalHandle(), Surface, &NumFoundPresentModes, FoundPresentModes.GetData()));

		bool bFoundDesiredMode = false;
		for (size_t i = 0; i < NumFoundPresentModes; i++)
		{
			if (FoundPresentModes[i] == PresentMode)
			{
				bFoundDesiredMode = true;
				break;
			}
		}
		if (!bFoundDesiredMode)
		{
			UE_LOG(LogVulkanRHI, Warning, TEXT("Couldn't find Present Mode %d!"), (int32)PresentMode);
			PresentMode = FoundPresentModes[0];
		}
	}

	// Check the surface properties and formats
	
	VkSurfaceCapabilitiesKHR SurfProperties;
	VERIFYVULKANRESULT_EXPANDED(VulkanRHI::vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device.GetPhysicalHandle(),
		Surface,
		&SurfProperties));
	VkSurfaceTransformFlagBitsKHR PreTransform;
	if (SurfProperties.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		PreTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		PreTransform = SurfProperties.currentTransform;
	}

	VkCompositeAlphaFlagBitsKHR CompositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	if (SurfProperties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
	{
		CompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	}
	
	// 0 means no limit, so use the requested number
	uint32 DesiredNumBuffers = SurfProperties.maxImageCount > 0 ? FMath::Clamp(*InOutDesiredNumBackBuffers, SurfProperties.minImageCount, SurfProperties.maxImageCount) : *InOutDesiredNumBackBuffers;

	uint32 SizeX = FVulkanPlatform::SupportsQuerySurfaceProperties() ? (SurfProperties.currentExtent.width == 0xFFFFFFFF ? Width : SurfProperties.currentExtent.width) : Width;
	uint32 SizeY = FVulkanPlatform::SupportsQuerySurfaceProperties() ? (SurfProperties.currentExtent.height == 0xFFFFFFFF ? Height : SurfProperties.currentExtent.height) : Height;
	//FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Create swapchain: %ux%u \n"), SizeX, SizeY);


	VkSwapchainCreateInfoKHR SwapChainInfo;
	FMemory::Memzero(SwapChainInfo);
	SwapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	SwapChainInfo.surface = Surface;
	SwapChainInfo.minImageCount = DesiredNumBuffers;
	SwapChainInfo.imageFormat = CurrFormat.format;
	SwapChainInfo.imageColorSpace = CurrFormat.colorSpace;
	SwapChainInfo.imageExtent.width = SizeX;
	SwapChainInfo.imageExtent.height = SizeY;
	SwapChainInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	//if (GCVarDelayAcquireBackBuffer->GetInt() != 0) Android does not use DelayAcquireBackBuffer, still has to have VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
	{
		SwapChainInfo.imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	}
	SwapChainInfo.preTransform = PreTransform;
	SwapChainInfo.imageArrayLayers = 1;
	SwapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	SwapChainInfo.presentMode = PresentMode;
	SwapChainInfo.oldSwapchain = VK_NULL_HANDLE;
	SwapChainInfo.clipped = VK_TRUE;
	SwapChainInfo.compositeAlpha = CompositeAlpha;

	*InOutDesiredNumBackBuffers = DesiredNumBuffers;

	{
		//#todo-rco: Crappy workaround
		if (SwapChainInfo.imageExtent.width == 0)
		{
			SwapChainInfo.imageExtent.width = Width;
		}
		if (SwapChainInfo.imageExtent.height == 0)
		{
			SwapChainInfo.imageExtent.height = Height;
		}
	}

	VkBool32 bSupportsPresent;
	VERIFYVULKANRESULT(VulkanRHI::vkGetPhysicalDeviceSurfaceSupportKHR(Device.GetPhysicalHandle(), Device.GetPresentQueue()->GetFamilyIndex(), Surface, &bSupportsPresent));
	ensure(bSupportsPresent);

	//ensure(SwapChainInfo.imageExtent.width >= SurfProperties.minImageExtent.width && SwapChainInfo.imageExtent.width <= SurfProperties.maxImageExtent.width);
	//ensure(SwapChainInfo.imageExtent.height >= SurfProperties.minImageExtent.height && SwapChainInfo.imageExtent.height <= SurfProperties.maxImageExtent.height);

	VERIFYVULKANRESULT_EXPANDED(VulkanRHI::vkCreateSwapchainKHR(Device.GetInstanceHandle(), &SwapChainInfo, nullptr, &SwapChain));

	uint32 NumSwapChainImages;
	VERIFYVULKANRESULT_EXPANDED(VulkanRHI::vkGetSwapchainImagesKHR(Device.GetInstanceHandle(), SwapChain, &NumSwapChainImages, nullptr));

	OutImages.AddUninitialized(NumSwapChainImages);
	VERIFYVULKANRESULT_EXPANDED(VulkanRHI::vkGetSwapchainImagesKHR(Device.GetInstanceHandle(), SwapChain, &NumSwapChainImages, OutImages.GetData()));

#if VULKAN_USE_IMAGE_ACQUIRE_FENCES
	ImageAcquiredFences.AddUninitialized(NumSwapChainImages);
	VulkanRHI::FFenceManager& FenceMgr = Device.GetFenceManager();
	for (uint32 BufferIndex = 0; BufferIndex < NumSwapChainImages; ++BufferIndex)
	{
		ImageAcquiredFences[BufferIndex] = Device.GetFenceManager().AllocateFence(true);
	}
#endif
	ImageAcquiredSemaphore.AddUninitialized(DesiredNumBuffers);
	for (uint32 BufferIndex = 0; BufferIndex < DesiredNumBuffers; ++BufferIndex)
	{
		ImageAcquiredSemaphore[BufferIndex] = new FVulkanSemaphore(Device);
	}
}

void FVulkanSwapChain::Destroy()
{
	VulkanRHI::vkDestroySwapchainKHR(Device.GetInstanceHandle(), SwapChain, nullptr);
	SwapChain = VK_NULL_HANDLE;

#if VULKAN_USE_IMAGE_ACQUIRE_FENCES
	VulkanRHI::FFenceManager& FenceMgr = Device.GetFenceManager();
	for (int32 Index = 0; Index < ImageAcquiredFences.Num(); ++Index)
	{
		FenceMgr.ReleaseFence(ImageAcquiredFences[Index]);
	}
#endif

	//#todo-rco: Enqueue for deletion as we first need to destroy the cmd buffers and queues otherwise validation fails
	for (int BufferIndex = 0; BufferIndex < ImageAcquiredSemaphore.Num(); ++BufferIndex)
	{
		delete ImageAcquiredSemaphore[BufferIndex];
	}

	VulkanRHI::vkDestroySurfaceKHR(Instance, Surface, nullptr);
	Surface = VK_NULL_HANDLE;
}

int32 FVulkanSwapChain::AcquireImageIndex(FVulkanSemaphore** OutSemaphore)
{
	// Get the index of the next swapchain image we should render to.
	// We'll wait with an "infinite" timeout, the function will block until an image is ready.
	// The ImageAcquiredSemaphore[ImageAcquiredSemaphoreIndex] will get signaled when the image is ready (upon function return).
	uint32 ImageIndex = 0;
	const int32 PrevSemaphoreIndex = SemaphoreIndex;
	SemaphoreIndex = (SemaphoreIndex + 1) % ImageAcquiredSemaphore.Num();

	// If we have not called present for any of the swapchain images, it will cause a crash/hang
	checkf(!(NumAcquireCalls == ImageAcquiredSemaphore.Num() - 1 && NumPresentCalls == 0), TEXT("vkAcquireNextImageKHR will fail as no images have been presented before acquiring all of them"));
#if VULKAN_USE_IMAGE_ACQUIRE_FENCES
	VulkanRHI::FFenceManager& FenceMgr = Device.GetFenceManager();
	FenceMgr.ResetFence(ImageAcquiredFences[SemaphoreIndex]);
#endif
	VkResult Result = VulkanRHI::vkAcquireNextImageKHR(
		Device.GetInstanceHandle(),
		SwapChain,
		UINT64_MAX,
		ImageAcquiredSemaphore[SemaphoreIndex]->GetHandle(),
#if VULKAN_USE_IMAGE_ACQUIRE_FENCES
		ImageAcquiredFences[SemaphoreIndex]->GetHandle(),
#else
		VK_NULL_HANDLE,
#endif
		&ImageIndex);
	if (Result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		SemaphoreIndex = PrevSemaphoreIndex;
		return (int32)EStatus::OutOfDate;
	}

	if (Result == VK_ERROR_SURFACE_LOST_KHR)
	{
		SemaphoreIndex = PrevSemaphoreIndex;
		return (int32)EStatus::SurfaceLost;
	}

	++NumAcquireCalls;
	*OutSemaphore = ImageAcquiredSemaphore[SemaphoreIndex];

	if (Result == VK_ERROR_VALIDATION_FAILED_EXT)
	{
		extern TAutoConsoleVariable<int32> GValidationCvar;
		if (GValidationCvar.GetValueOnRenderThread() == 0)
		{
			UE_LOG(LogVulkanRHI, Fatal, TEXT("vkAcquireNextImageKHR failed with Validation error. Try running with r.Vulkan.EnableValidation=1 to get information from the driver"));
		}
	}
	else
	{
		checkf(Result == VK_SUCCESS || Result == VK_SUBOPTIMAL_KHR, TEXT("vkAcquireNextImageKHR failed Result = %d"), int32(Result));
	}
	CurrentImageIndex = (int32)ImageIndex;
	
#if VULKAN_USE_IMAGE_ACQUIRE_FENCES
	{
		SCOPE_CYCLE_COUNTER(STAT_VulkanWaitSwapchain);
		bool bResult = FenceMgr.WaitForFence(ImageAcquiredFences[SemaphoreIndex], UINT64_MAX);
		ensure(bResult);
	}
#endif
	return CurrentImageIndex;
}

FVulkanSwapChain::EStatus FVulkanSwapChain::Present(FVulkanQueue* GfxQueue, FVulkanQueue* PresentQueue, FVulkanSemaphore* BackBufferRenderingDoneSemaphore)
{
	if (CurrentImageIndex == -1)
	{
		// Skip present silently if image has not been acquired
		return EStatus::Healthy;
	}

	//ensure(GfxQueue == PresentQueue);

	VkPresentInfoKHR Info;
	FMemory::Memzero(Info);
	Info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	VkSemaphore Semaphore = VK_NULL_HANDLE;
	if (BackBufferRenderingDoneSemaphore)
	{
		Info.waitSemaphoreCount = 1;
		Semaphore = BackBufferRenderingDoneSemaphore->GetHandle();
		Info.pWaitSemaphores = &Semaphore;
	}
	Info.swapchainCount = 1;
	Info.pSwapchains = &SwapChain;
	Info.pImageIndices = (uint32*)&CurrentImageIndex;

	{
		SCOPE_CYCLE_COUNTER(STAT_VulkanQueuePresent);
		VkResult PresentResult = VulkanRHI::vkQueuePresentKHR(PresentQueue->GetHandle(), &Info);
		if (PresentResult == VK_ERROR_OUT_OF_DATE_KHR)
		{
			return EStatus::OutOfDate;
		}

		if (PresentResult == VK_ERROR_SURFACE_LOST_KHR)
		{
			return EStatus::SurfaceLost;
		}

		if (PresentResult != VK_SUCCESS && PresentResult != VK_SUBOPTIMAL_KHR)
		{
			VERIFYVULKANRESULT(PresentResult);
		}
	}

	++NumPresentCalls;

	return EStatus::Healthy;
}


void FVulkanDevice::SetupPresentQueue(const VkSurfaceKHR& Surface)
{
	if (!PresentQueue)
	{
		const auto SupportsPresent = [Surface](VkPhysicalDevice PhysicalDevice, FVulkanQueue* Queue)
		{
			VkBool32 bSupportsPresent = VK_FALSE;
			const uint32 FamilyIndex = Queue->GetFamilyIndex();
			VERIFYVULKANRESULT(VulkanRHI::vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, FamilyIndex, Surface, &bSupportsPresent));
			if (bSupportsPresent)
			{
				UE_LOG(LogVulkanRHI, Display, TEXT("Queue Family %d: Supports Present"), FamilyIndex);
			}
			return (bSupportsPresent == VK_TRUE);
		};

		bool bGfx = SupportsPresent(Gpu, GfxQueue);
		checkf(bGfx, TEXT("Graphics Queue doesn't support present!"));
		bool bCompute = SupportsPresent(Gpu, ComputeQueue);
		if (TransferQueue->GetFamilyIndex() != GfxQueue->GetFamilyIndex() && TransferQueue->GetFamilyIndex() != ComputeQueue->GetFamilyIndex())
		{
			SupportsPresent(Gpu, TransferQueue);
		}
		if (ComputeQueue->GetFamilyIndex() != GfxQueue->GetFamilyIndex() && bCompute)
		{
			PresentQueue = ComputeQueue;
		}
		else
		{
			PresentQueue = GfxQueue;
		}
	}
}
