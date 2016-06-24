// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanViewport.cpp: Vulkan viewport RHI implementation.
=============================================================================*/

#include "VulkanRHIPrivate.h"
#include "VulkanManager.h"
#include "VulkanSwapchain.h"
#include "VulkanPendingState.h"
#include "VulkanContext.h"


FVulkanViewport::FVulkanViewport(FVulkanDynamicRHI* InRHI, void* WindowHandle, uint32 InSizeX, uint32 InSizeY, bool bInIsFullscreen, EPixelFormat InPreferredPixelFormat)
	: RHI(InRHI)
	, SizeX(InSizeX)
	, SizeY(InSizeY)
	, bIsFullscreen(bInIsFullscreen)
	, PixelFormat(InPreferredPixelFormat)
	, CurrentBackBuffer(-1)
	, SwapChain(nullptr)
{
	check(IsInGameThread());
	RHI->Viewports.Add(this);

	// Make sure Instance is created
	RHI->InitInstance();

	uint32 DesiredNumBackBuffers = NUM_BUFFERS;

	TArray<VkImage> Images;
	SwapChain = new FVulkanSwapChain(
		RHI->Instance, *RHI->Device, WindowHandle,
		PixelFormat, InSizeX, InSizeY, 
		&DesiredNumBackBuffers,
		Images);

	check(Images.Num() == NUM_BUFFERS);

	FVulkanCmdBuffer* CmdBuffer = RHI->Device->GetImmediateContext().GetCommandBufferManager()->GetActiveCmdBuffer();
	check(CmdBuffer->IsOutsideRenderPass());
	for (int32 Index = 0, Count = Images.Num(); Index < Count; ++Index)
	{
		VkImage Image = Images[Index];

		// Constructor will set to color optimal
		BackBuffers[Index] = new FVulkanBackBuffer(*RHI->Device, PixelFormat, InSizeX, InSizeY, Image, TexCreate_Presentable | TexCreate_RenderTargetable, FClearValueBinding());

		FName Name = FName(*FString::Printf(TEXT("BackBuffer%d"), Index));
		BackBuffers[Index]->SetName(Name);
	}
}

FVulkanViewport::~FVulkanViewport()
{
	for (int32 Index = 0; Index < NUM_BUFFERS; ++Index)
	{
		BackBuffers[Index]->Destroy(*RHI->Device);
		BackBuffers[Index] = nullptr;
	}

	SwapChain->Destroy();
	delete SwapChain;

	RHI->Viewports.Remove(this);
}


FVulkanFramebuffer::FVulkanFramebuffer(FVulkanDevice& Device, const FRHISetRenderTargetsInfo& InRTInfo, const FVulkanRenderTargetLayout& RTLayout, const FVulkanRenderPass& RenderPass)
	: Framebuffer(VK_NULL_HANDLE)
	, NumColorAttachments(0)
	, BackBuffer(0)
{
	Attachments.Empty(RTLayout.GetNumAttachments());

	for (int32 Index = 0; Index < InRTInfo.NumColorRenderTargets; ++Index)
	{
		FRHITexture* RHITexture = InRTInfo.ColorRenderTarget[Index].Texture;
		if(!RHITexture)
		{
			continue;
		}

		FVulkanTextureBase* Texture = FVulkanTextureBase::Cast(RHITexture);
	#if VULKAN_USE_MSAA_RESOLVE_ATTACHMENTS
		if (Texture->MSAASurface)
		{
#if 1//VULKAN_USE_NEW_COMMAND_BUFFERS
			check(0);
#endif
			Attachments.Add(Texture->MSAAView.View);

		    // Create a write-barrier
		    WriteBarriers.AddZeroed();
		    VkImageMemoryBarrier& Barrier = WriteBarriers.Last();
		    Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		    //Barrier.pNext = NULL;
		    Barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		    Barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		    Barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		    Barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		    Barrier.image = Texture->Surface.Image;
		    Barrier.subresourceRange.aspectMask = Texture->MSAASurface->GetAspectMask();
		    //Barrier.subresourceRange.baseMipLevel = 0;
		    Barrier.subresourceRange.levelCount = 1;
		    //Barrier.subresourceRange.baseArrayLayer = 0;
		    Barrier.subresourceRange.layerCount = 1;
			Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		}
	#endif
		Attachments.Add(Texture->View.View);

		// Create a write-barrier
		WriteBarriers.AddZeroed();
		VkImageMemoryBarrier& Barrier = WriteBarriers.Last();
		Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		//Barrier.pNext = NULL;
		Barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		Barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		Barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		Barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		Barrier.image = Texture->Surface.Image;
		Barrier.subresourceRange.aspectMask = Texture->Surface.GetAspectMask();
		//Barrier.subresourceRange.baseMipLevel = 0;
		Barrier.subresourceRange.levelCount = 1;
		//Barrier.subresourceRange.baseArrayLayer = 0;
		Barrier.subresourceRange.layerCount = 1;

		if (Texture->Surface.GetViewType() == VK_IMAGE_VIEW_TYPE_2D)
		{
			FVulkanTexture2D* Texture2D = (FVulkanTexture2D*)Texture;
			if (Texture2D->IsBackBuffer())
			{
				check(BackBuffer == nullptr);
				BackBuffer = (FVulkanBackBuffer*)Texture2D;
			}
		}

		NumColorAttachments++;
	}

	if (RTLayout.GetHasDepthStencil())
	{
		FVulkanTextureBase* Texture = FVulkanTextureBase::Cast(InRTInfo.DepthStencilRenderTarget.Texture);

		//#todo-rco: Check this got fixed with a new driver/OS
		if (PLATFORM_ANDROID)
		{
			//@HACK: Re-create the ImageView for the depth buffer, because the original view doesn't work for some unknown reason (it's a bug in the device system software)
			Texture->View.Destroy(Device);
			Texture->View.Create(Device, Texture->Surface,
				Texture->Surface.GetViewType(),
				Texture->Surface.InternalFormat,
				Texture->Surface.GetNumMips());
		}

		bool bHasStencil = (Texture->Surface.Format == PF_DepthStencil);

		// Create a write-barrier
		WriteBarriers.AddZeroed();
		VkImageMemoryBarrier& Barrier = WriteBarriers.Last();
		Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		//Barrier.pNext = NULL;
		Barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		Barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		Barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		Barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		Barrier.image = Texture->Surface.Image;

		Barrier.subresourceRange.aspectMask = Texture->Surface.GetAspectMask();
		//Barrier.subresourceRange.baseMipLevel = 0;
		Barrier.subresourceRange.levelCount = 1;
		//Barrier.subresourceRange.baseArrayLayer = 0;
		Barrier.subresourceRange.layerCount = 1;

		Attachments.Add(Texture->View.View);
	}

	VkFramebufferCreateInfo Info;
	FMemory::Memzero(Info);
	Info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	Info.pNext = nullptr;
	Info.renderPass = RenderPass.GetHandle();
	Info.attachmentCount = Attachments.Num();
	Info.pAttachments = Attachments.GetData();
	Info.width  = RTLayout.GetExtent3D().width;
	Info.height = RTLayout.GetExtent3D().height;
	Info.layers = 1;
	VERIFYVULKANRESULT_EXPANDED(vkCreateFramebuffer(Device.GetInstanceHandle(), &Info, nullptr, &Framebuffer));

	RTInfo = InRTInfo;
}

void FVulkanFramebuffer::Destroy(FVulkanDevice& Device)
{
	vkDestroyFramebuffer(Device.GetInstanceHandle(), Framebuffer, nullptr);
}

bool FVulkanFramebuffer::Matches(const FRHISetRenderTargetsInfo& InRTInfo) const
{
	if (RTInfo.NumColorRenderTargets != InRTInfo.NumColorRenderTargets)
	{
		return false;
	}
	if (RTInfo.bClearColor != InRTInfo.bClearColor)
	{
		return false;
	}
	if (RTInfo.bClearDepth != InRTInfo.bClearDepth)
	{
		return false;
	}
	if (RTInfo.bClearStencil != InRTInfo.bClearStencil)
	{
		return false;
	}

	{
		const FRHIDepthRenderTargetView& A = RTInfo.DepthStencilRenderTarget;
		const FRHIDepthRenderTargetView& B = InRTInfo.DepthStencilRenderTarget;
		if (!(A == B))
		{
			return false;
		}
	}

	// We dont need to compare all render-tagets, since we
	// already have compared the number of render-targets
	for (int32 Index = 0; Index < RTInfo.NumColorRenderTargets; ++Index)
	{
		const FRHIRenderTargetView& A = RTInfo.ColorRenderTarget[Index];
		const FRHIRenderTargetView& B = InRTInfo.ColorRenderTarget[Index];
		if (!(A == B))
		{
			return false;
		}
	}

	return true;
}

void FVulkanViewport::AcquireBackBuffer(FVulkanCmdBuffer* CmdBuffer)
{
	FVulkanSemaphore* AcquireSemaphore = nullptr;
	CurrentBackBuffer = SwapChain->AcquireImageIndex(&AcquireSemaphore);
	check(CurrentBackBuffer != -1);
	check(!BackBuffers[CurrentBackBuffer]->AcquiredSemaphore);
	BackBuffers[CurrentBackBuffer]->AcquiredSemaphore = AcquireSemaphore;
	VulkanSetImageLayoutSimple(CmdBuffer->GetHandle(), BackBuffers[CurrentBackBuffer]->Surface.Image, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

FVulkanBackBuffer* FVulkanViewport::PrepareBackBufferForPresent(FVulkanCmdBuffer* CmdBuffer)
{
	check(CurrentBackBuffer != -1);
	VulkanSetImageLayoutSimple(CmdBuffer->GetHandle(), BackBuffers[CurrentBackBuffer]->Surface.Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	return BackBuffers[CurrentBackBuffer];
}

void FVulkanFramebuffer::InsertWriteBarriers(FVulkanCmdBuffer* CmdBuffer)
{
	if (WriteBarriers.Num() == 0)
	{
		return;
	}

	const VkPipelineStageFlags SrcStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	const VkPipelineStageFlags DestStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	check(CmdBuffer->IsOutsideRenderPass());
	vkCmdPipelineBarrier(CmdBuffer->GetHandle(), SrcStages, DestStages, 0, 0, nullptr, 0, nullptr, WriteBarriers.Num(), WriteBarriers.GetData());
}

/*=============================================================================
 *	The following RHI functions must be called from the main thread.
 *=============================================================================*/
FViewportRHIRef FVulkanDynamicRHI::RHICreateViewport(void* WindowHandle, uint32 SizeX, uint32 SizeY, bool bIsFullscreen, EPixelFormat PreferredPixelFormat)
{
	check( IsInGameThread() );
	return new FVulkanViewport(this, WindowHandle, SizeX, SizeY, bIsFullscreen, PreferredPixelFormat);
}

void FVulkanDynamicRHI::RHIResizeViewport(FViewportRHIParamRef ViewportRHI, uint32 SizeX, uint32 SizeY, bool bIsFullscreen)
{
	check( IsInGameThread() );
	FVulkanViewport* Viewport = ResourceCast(ViewportRHI);
}

void FVulkanDynamicRHI::RHITick( float DeltaTime )
{
	check( IsInGameThread() );
}

void FVulkanDynamicRHI::WriteEndFrameTimestamp(void* Data)
{
	FVulkanDynamicRHI* This = (FVulkanDynamicRHI*)Data;
	if (This && This->Device)
	{
		FVulkanTimestampQueryPool* TimestampQueryPool = This->Device->GetTimestampQueryPool(This->PresentCount % (uint32)FVulkanDevice::NumTimestampPools);
		if (TimestampQueryPool)
		{
			FVulkanCmdBuffer* CmdBuffer = This->Device->GetImmediateContext().GetCommandBufferManager()->GetActiveCmdBuffer();
			TimestampQueryPool->WriteEndFrame(CmdBuffer->GetHandle());
		}
	}

	VulkanRHI::GManager.GPUProfilingData.EndFrameBeforeSubmit();
}

void FVulkanDynamicRHI::Present()
{
	check(DrawingViewport);

	check(Device);

	FVulkanPendingState* PendingState = &Device->GetImmediateContext().GetPendingState();
	FVulkanCommandBufferManager* CmdBufferManager = Device->GetImmediateContext().GetCommandBufferManager();
	FVulkanCmdBuffer* CmdBuffer = CmdBufferManager->GetActiveCmdBuffer();
	if (PendingState->IsRenderPassActive())
	{
		PendingState->RenderPassEnd(CmdBuffer);
	}
	FVulkanBackBuffer* BackBuffer = DrawingViewport->PrepareBackBufferForPresent(CmdBuffer);
	WriteEndFrameTimestamp(this);
	CmdBuffer->End();
	Device->GetQueue()->Submit(CmdBuffer, BackBuffer->AcquiredSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, BackBuffer->RenderingDoneSemaphore);
	// No need to acquire it anymore
	BackBuffer->AcquiredSemaphore = nullptr;

	DrawingViewport->GetSwapChain()->Present(Device->GetQueue(), BackBuffer->RenderingDoneSemaphore);

	bool bNativelyPresented = true;
	if (bNativelyPresented)
	{
		static const TConsoleVariableData<int32>* CFinishFrameVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FinishCurrentFrame"));
		if (!CFinishFrameVar->GetValueOnRenderThread())
		{
			// Wait for the GPU to finish rendering the previous frame before finishing this frame.
			DrawingViewport->WaitForFrameEventCompletion();
			DrawingViewport->IssueFrameEvent();
		}
		else
		{
			// Finish current frame immediately to reduce latency
			DrawingViewport->IssueFrameEvent();
			DrawingViewport->WaitForFrameEventCompletion();
		}
	}

	// If the input latency timer has been triggered, block until the GPU is completely
	// finished displaying this frame and calculate the delta time.
	if (GInputLatencyTimer.RenderThreadTrigger)
	{
		DrawingViewport->WaitForFrameEventCompletion();
		uint32 EndTime = FPlatformTime::Cycles();
		GInputLatencyTimer.DeltaTime = EndTime - GInputLatencyTimer.StartTime;
		GInputLatencyTimer.RenderThreadTrigger = false;
	}

	//#todo-rco: This needs to go on RHIEndFrame but the CmdBuffer index is not the correct one to read the stats out!
	VulkanRHI::GManager.GPUProfilingData.EndFrame();

	Device->GetImmediateContext().GetCommandBufferManager()->PrepareForNewActiveCommandBuffer();

	//#todo-rco: Consolidate 'end of frame'
	Device->GetImmediateContext().GetTempFrameAllocationBuffer().Reset();

	DrawingViewport->CurrentBackBuffer = -1;
	DrawingViewport = nullptr;
	PendingState->Reset();

	const uint32 QueryCurrFrameIndex = PresentCount % FVulkanDevice::NumTimestampPools;
	const uint32 QueryPrevFrameIndex = (QueryCurrFrameIndex + FVulkanDevice::NumTimestampPools - 1) % FVulkanDevice::NumTimestampPools;
	const uint32 QueryNextFrameIndex = (QueryCurrFrameIndex + 1) % FVulkanDevice::NumTimestampPools;

	FVulkanTimestampQueryPool* TimestampQueryPool = Device->GetTimestampQueryPool(QueryPrevFrameIndex);
	if (TimestampQueryPool)
	{
		if(PresentCount > FVulkanDevice::NumTimestampPools)
		{
			TimestampQueryPool->CalculateFrameTime();
		}

		Device->GetTimestampQueryPool(QueryNextFrameIndex)->WriteStartFrame(CmdBufferManager->GetActiveCmdBuffer()->GetHandle());
	}

	PresentCount++;
}

FTexture2DRHIRef FVulkanDynamicRHI::RHIGetViewportBackBuffer(FViewportRHIParamRef ViewportRHI)
{
	FVulkanViewport* Viewport = ResourceCast(ViewportRHI);
	if (Viewport->CurrentBackBuffer < 0)
	{
		Viewport->AcquireBackBuffer(Device->GetImmediateContext().GetCommandBufferManager()->GetActiveCmdBuffer());
	}
	return Viewport->GetBackBuffer();
}

void FVulkanDynamicRHI::RHIAdvanceFrameForGetViewportBackBuffer()
{
	//#todo-rco: Do a pass to clear unused or expired elements in managers
}

void FVulkanCommandListContext::RHISetViewport(uint32 MinX, uint32 MinY, float MinZ, uint32 MaxX, uint32 MaxY, float MaxZ)
{
	check(Device);
	PendingState->SetViewport(MinX, MinY, MinZ, MaxX, MaxY, MaxZ);
}

void FVulkanCommandListContext::RHISetMultipleViewports(uint32 Count, const FViewportBounds* Data)
{
	VULKAN_SIGNAL_UNIMPLEMENTED();
}

void FVulkanCommandListContext::RHISetScissorRect(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY)
{
	check(Device);
	PendingState->SetScissor(bEnable, MinX, MinY, MaxX, MaxY);
}
