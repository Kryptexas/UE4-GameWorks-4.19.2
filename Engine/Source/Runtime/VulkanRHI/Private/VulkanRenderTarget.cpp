// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanRenderTarget.cpp: Vulkan render target implementation.
=============================================================================*/

#include "VulkanRHIPrivate.h"
#include "ScreenRendering.h"
#include "VulkanPendingState.h"
#include "VulkanContext.h"

static int32 GSubmitOnCopyToResolve = 0;
static FAutoConsoleVariableRef CVarVulkanSubmitOnCopyToResolve(
	TEXT("r.Vulkan.SubmitOnCopyToResolve"),
	GSubmitOnCopyToResolve,
	TEXT("Submits the Queue to the GPU on every RHICopyToResolveTarget call.\n")
	TEXT(" 0: Do not submit (default)\n")
	TEXT(" 1: Submit"),
	ECVF_Default
	);

#if VULKAN_USE_NEW_RENDERPASSES
void FVulkanCommandListContext::FRenderPassState::BeginRenderPass(FVulkanCommandListContext& Context, FVulkanDevice& InDevice, FVulkanCmdBuffer* CmdBuffer, const FRHISetRenderTargetsInfo& RenderTargetsInfo)
{
	check(!CurrentRenderPass);
	VkClearValue ClearValues[MaxSimultaneousRenderTargets + 1];
	FMemory::Memzero(ClearValues);
	FVulkanRenderTargetLayout RTLayout(RenderTargetsInfo);
	//#todo-rco
	FVulkanRenderPass* RenderPass = new FVulkanRenderPass(InDevice, RTLayout);//GetOrCreateRenderPass(RTLayout);
	//#todo-rco
	FVulkanFramebuffer* Framebuffer = new FVulkanFramebuffer(InDevice, RenderTargetsInfo, RTLayout, *RenderPass);//GetOrCreateFramebuffer(RenderTargetsInfo);

	for (int32 Index = 0; Index < RenderTargetsInfo.NumColorRenderTargets; ++Index)
	{
		FTextureRHIParamRef Texture = RenderTargetsInfo.ColorRenderTarget[Index].Texture;
		if (Texture)
		{
			VkImage Image = FVulkanTextureBase::Cast(Texture)->Surface.Image;
			VkImageLayout* Found = CurrentLayout.Find(Image);
			if (!Found)
			{
				Found = &CurrentLayout.Add(Image, VK_IMAGE_LAYOUT_UNDEFINED);
			}

			if (*Found != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			{
				Context.RHITransitionResources(EResourceTransitionAccess::EWritable, &Texture, 1);
			}

			const FLinearColor& ClearColor = Texture->HasClearValue() ? Texture->GetClearColor() : FLinearColor::Black;
			ClearValues[Index].color.float32[0] = ClearColor.R;
			ClearValues[Index].color.float32[1] = ClearColor.G;
			ClearValues[Index].color.float32[2] = ClearColor.B;
			ClearValues[Index].color.float32[3] = ClearColor.A;

			*Found = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
	}

	if (RenderTargetsInfo.DepthStencilRenderTarget.Texture)
	{
		FTextureRHIParamRef DSTexture = RenderTargetsInfo.DepthStencilRenderTarget.Texture;
		VkImageLayout& DSLayout = CurrentLayout.FindOrAdd(FVulkanTextureBase::Cast(DSTexture)->Surface.Image);
		if (DSLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL || DSLayout == VK_IMAGE_LAYOUT_UNDEFINED)
		{
			Context.RHITransitionResources(EResourceTransitionAccess::EWritable, &DSTexture, 1);
			DSLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}
		else
		{
			ensure(DSLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		}
		if (DSTexture->HasClearValue())
		{
			float Depth = 0;
			uint32 Stencil = 0;
			DSTexture->GetDepthStencilClearValue(Depth, Stencil);
			ClearValues[RenderTargetsInfo.NumColorRenderTargets].depthStencil.depth = Depth;
			ClearValues[RenderTargetsInfo.NumColorRenderTargets].depthStencil.stencil = Stencil;
		}
	}

	CmdBuffer->BeginRenderPass(RenderPass->GetLayout(), RenderPass->GetHandle(), Framebuffer->GetHandle(), ClearValues);

	CurrentFramebuffer = Framebuffer;
	CurrentRenderPass = RenderPass;
}

void FVulkanCommandListContext::FRenderPassState::EndRenderPass(FVulkanCmdBuffer* CmdBuffer)
{
	check(CurrentRenderPass);
	CmdBuffer->EndRenderPass();
	CurrentRenderPass = nullptr;
}
#endif

void FVulkanCommandListContext::RHISetRenderTargets(uint32 NumSimultaneousRenderTargets, const FRHIRenderTargetView* NewRenderTargets, const FRHIDepthRenderTargetView* NewDepthStencilTarget, uint32 NumUAVs, const FUnorderedAccessViewRHIParamRef* UAVs)
{
	//FRCLog::Printf(FString::Printf(TEXT("RHISetRenderTargets")));

	FRHIDepthRenderTargetView DepthView;
	if (NewDepthStencilTarget)
	{
		DepthView = *NewDepthStencilTarget;
	}
	else
	{
#if VULKAN_USE_NEW_RENDERPASSES
		DepthView = FRHIDepthRenderTargetView(FTextureRHIParamRef(), ERenderTargetLoadAction::ENoAction, ERenderTargetStoreAction::ENoAction, ERenderTargetLoadAction::ENoAction, ERenderTargetStoreAction::ENoAction);
#else
		DepthView = FRHIDepthRenderTargetView(FTextureRHIParamRef(), ERenderTargetLoadAction::EClear, ERenderTargetStoreAction::ENoAction);
#endif
	}

#if VULKAN_USE_NEW_RENDERPASSES
	ensure(NumUAVs == 0);

	if (NumSimultaneousRenderTargets == 1 && (!NewRenderTargets || !NewRenderTargets->Texture))
	{
		--NumSimultaneousRenderTargets;
	}
#else
	checkf(NumUAVs == 0, TEXT("Calling SetRenderTargets with UAVs is not supported in Vulkan yet"));
#endif

	FRHISetRenderTargetsInfo Info(NumSimultaneousRenderTargets, NewRenderTargets, DepthView);
	RHISetRenderTargetsAndClear(Info);
}

void FVulkanCommandListContext::RHISetRenderTargetsAndClear(const FRHISetRenderTargetsInfo& RenderTargetsInfo)
{
	//FRCLog::Printf(FString::Printf(TEXT("RHISetRenderTargetsAndClear")));

#if VULKAN_USE_NEW_RENDERPASSES
	FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
	if (CmdBuffer->IsInsideRenderPass())
	{
		RenderPassState.EndRenderPass(CmdBuffer);
	}

	if (RenderTargetsInfo.DepthStencilRenderTarget.Texture ||
		RenderTargetsInfo.NumColorRenderTargets > 1 ||
		RenderTargetsInfo.NumColorRenderTargets == 1 && RenderTargetsInfo.ColorRenderTarget[0].Texture)
	{
		RenderPassState.BeginRenderPass(*this, *Device, CmdBuffer, RenderTargetsInfo);
	}
#else
	PendingState->SetRenderTargetsInfo(RenderTargetsInfo);
#endif


#if 0//VULKAN_USE_NEW_RESOURCE_MANAGEMENT
	FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
	if (CmdBuffer->IsInsideRenderPass())
	{
		if (
			(RenderTargetsInfo.NumColorRenderTargets == 0 || (RenderTargetsInfo.NumColorRenderTargets == 1 && !RenderTargetsInfo.ColorRenderTarget[0].Texture)) &&
			!RenderTargetsInfo.DepthStencilRenderTarget.Texture)
		{
			PendingState->RenderPassEnd(CmdBuffer);
			PendingState->PrevRenderTargetsInfo = FRHISetRenderTargetsInfo();
		}
	}
#endif
}

void FVulkanCommandListContext::RHICopyToResolveTarget(FTextureRHIParamRef SourceTextureRHI, FTextureRHIParamRef DestTextureRHI, bool bKeepOriginalSurface, const FResolveParams& ResolveParams)
{
	//FRCLog::Printf(FString::Printf(TEXT("RHICopyToResolveTarget")));
#if VULKAN_USE_NEW_RENDERPASSES
	if (!SourceTextureRHI || !DestTextureRHI)
	{
		// no need to do anything (silently ignored)
		return;
	}

	RHITransitionResources(EResourceTransitionAccess::EReadable, &SourceTextureRHI, 1);

	auto CopyImage = [](FRenderPassState& RenderPassState, FVulkanCmdBuffer* InCmdBuffer, FVulkanSurface& SrcSurface, FVulkanSurface& DstSurface, uint32 NumLayers)
	{
		VkImageLayout* SrcLayoutPtr = RenderPassState.CurrentLayout.Find(SrcSurface.Image);
		check(SrcLayoutPtr);
		VkImageLayout SrcLayout = *SrcLayoutPtr;
		VkImageLayout* DstLayoutPtr = RenderPassState.CurrentLayout.Find(DstSurface.Image);
		VkImageLayout DstLayout = (DstSurface.GetFullAspectMask() & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0 ?
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		if (!DstLayoutPtr)
		{
			RenderPassState.CurrentLayout.Add(DstSurface.Image, DstLayout);
		}

		check(InCmdBuffer->IsOutsideRenderPass());
		VkCommandBuffer CmdBuffer = InCmdBuffer->GetHandle();
		VkImageSubresourceRange SrcRange ={SrcSurface.GetFullAspectMask(), 0, 1, 0, 6};
		VkImageSubresourceRange DstRange ={DstSurface.GetFullAspectMask(), 0, 1, 0, 6};
		VulkanSetImageLayout(CmdBuffer, SrcSurface.Image, SrcLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, SrcRange);
		VulkanSetImageLayout(CmdBuffer, DstSurface.Image, DstLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, DstRange);

		VkImageCopy Region;
		FMemory::Memzero(Region);
		Region.extent.width = FMath::Min(SrcSurface.Width, DstSurface.Width);
		Region.extent.height = FMath::Min(SrcSurface.Height, DstSurface.Height);
		Region.extent.depth = 1;
		Region.srcSubresource.aspectMask = SrcSurface.GetFullAspectMask();
		Region.srcSubresource.layerCount = NumLayers;
		Region.dstSubresource.aspectMask = DstSurface.GetFullAspectMask();
		Region.dstSubresource.layerCount = NumLayers;
		VulkanRHI::vkCmdCopyImage(CmdBuffer,
			SrcSurface.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			DstSurface.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &Region);

		VulkanSetImageLayout(CmdBuffer, SrcSurface.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, SrcLayout, SrcRange);
		VulkanSetImageLayout(CmdBuffer, DstSurface.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, DstLayout, DstRange);
	};
	FRHITexture2D* SourceTextureRHI2D = SourceTextureRHI->GetTexture2D();
	if (SourceTextureRHI2D)
	{
		FRHITexture2D* DestTextureRHI2D = DestTextureRHI->GetTexture2D();
		FVulkanTexture2D* VulkanSrcTexture = (FVulkanTexture2D*)SourceTextureRHI2D;
		FVulkanTexture2D* VulkanDstTexture = (FVulkanTexture2D*)DestTextureRHI2D;
		if (VulkanSrcTexture->Surface.Image != VulkanDstTexture->Surface.Image)
		{
			FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
			CopyImage(RenderPassState, CmdBuffer, VulkanSrcTexture->Surface, VulkanDstTexture->Surface, 1);
		}
	}
	else
	{
		FRHITexture3D* SourceTextureRHI3D = SourceTextureRHI->GetTexture3D();
		if (SourceTextureRHI3D)
		{
			FRHITexture3D* DestTextureRHI3D = DestTextureRHI->GetTexture3D();
			FVulkanTexture3D* VulkanSrcTexture = (FVulkanTexture3D*)SourceTextureRHI3D;
			FVulkanTexture3D* VulkanDstTexture = (FVulkanTexture3D*)DestTextureRHI3D;
			if (VulkanSrcTexture->Surface.Image != VulkanDstTexture->Surface.Image)
			{
				FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
				CopyImage(RenderPassState, CmdBuffer, VulkanSrcTexture->Surface, VulkanDstTexture->Surface, 1);
			}
		}
		else
		{
			FRHITextureCube* SourceTextureRHICube = SourceTextureRHI->GetTextureCube();
			check(SourceTextureRHICube);
			FRHITextureCube* DestTextureRHICube = DestTextureRHI->GetTextureCube();
			FVulkanTextureCube* VulkanSrcTexture = (FVulkanTextureCube*)SourceTextureRHICube;
			FVulkanTextureCube* VulkanDstTexture = (FVulkanTextureCube*)DestTextureRHICube;
			if (VulkanSrcTexture->Surface.Image != VulkanDstTexture->Surface.Image)
			{
				FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
				CopyImage(RenderPassState, CmdBuffer, VulkanSrcTexture->Surface, VulkanDstTexture->Surface, 6);
			}
		}
	}
#else
#if 1//VULKAN_USE_NEW_COMMAND_BUFFERS
	FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
	// Verify if we need to do some work (for the case of SetRT(), CopyToResolve() with no draw calls in between)
	PendingState->UpdateRenderPass(CmdBuffer);

	const bool bRenderPassIsActive = PendingState->IsRenderPassActive();

	if (bRenderPassIsActive)
	{
		PendingState->RenderPassEnd(CmdBuffer);
	}

	check(!SourceTextureRHI || SourceTextureRHI->GetNumSamples() < 2);

	FVulkanFramebuffer* Framebuffer = PendingState->GetFrameBuffer();
	if (Framebuffer)
	{
		Framebuffer->InsertWriteBarriers(GetCommandBufferManager()->GetActiveCmdBuffer());
	}

	if (GSubmitOnCopyToResolve)
	{
		check(0);
#if 0
		CmdBuffer->End();

		FVulkanSemaphore* BackBufferAcquiredSemaphore = Framebuffer->GetBackBuffer() ? Framebuffer->GetBackBuffer()->AcquiredSemaphore : nullptr;

		Device->GetQueue()->Submit(CmdBuffer, BackBufferAcquiredSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, nullptr);
		// No need to acquire it anymore
		Framebuffer->GetBackBuffer()->AcquiredSemaphore = nullptr;
		CommandBufferManager->PrepareForNewActiveCommandBuffer();
#endif
	}
#else
	// If we're using the pResolveAttachments property of the subpass, so we don't need manual command buffer resolves and this function is a NoOp.
#if !VULKAN_USE_MSAA_RESOLVE_ATTACHMENTS
	if (SourceTextureRHI->GetNumSamples() < 2)
	{
		return;
	}

	const bool bRenderPassIsActive = PendingState->IsRenderPassActive();

	if (bRenderPassIsActive)
	{
		PendingState->RenderPassEnd();
	}

	VulkanResolveImage(PendingState->GetCommandBuffer(), SourceTextureRHI, DestTextureRHI);

	if (bRenderPassIsActive)
	{
		PendingState->RenderPassBegin();
	}
#endif
#endif
#endif
}

void FVulkanDynamicRHI::RHIReadSurfaceData(FTextureRHIParamRef TextureRHI, FIntRect Rect, TArray<FColor>& OutData, FReadSurfaceDataFlags InFlags)
{
	FRHITexture2D* TextureRHI2D = TextureRHI->GetTexture2D();
	check(TextureRHI2D);
	FVulkanTexture2D* Texture2D = (FVulkanTexture2D*)TextureRHI2D;
	uint32 NumPixels = TextureRHI2D->GetSizeX() * TextureRHI2D->GetSizeY();
	FVulkanCmdBuffer* CmdBuffer = Device->GetImmediateContext().GetCommandBufferManager()->GetUploadCmdBuffer();

	ensure(Texture2D->Surface.InternalFormat == VK_FORMAT_R8G8B8A8_UNORM || Texture2D->Surface.InternalFormat == VK_FORMAT_B8G8R8A8_UNORM);
	const uint32 Size = NumPixels * sizeof(FColor);
	VulkanRHI::FStagingBuffer* StagingBuffer = Device->GetStagingManager().AcquireBuffer(Size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);
	VkBufferImageCopy CopyRegion;
	FMemory::Memzero(CopyRegion);
	//Region.bufferOffset = 0;
	CopyRegion.bufferRowLength = TextureRHI2D->GetSizeX();
	CopyRegion.bufferImageHeight = TextureRHI2D->GetSizeY();
	CopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	//Region.imageSubresource.mipLevel = 0;
	//Region.imageSubresource.baseArrayLayer = 0;
	CopyRegion.imageSubresource.layerCount = 1;
	CopyRegion.imageExtent.width = TextureRHI2D->GetSizeX();
	CopyRegion.imageExtent.height = TextureRHI2D->GetSizeY();
	CopyRegion.imageExtent.depth = 1;
	VulkanRHI::vkCmdCopyImageToBuffer(CmdBuffer->GetHandle(), Texture2D->Surface.Image, VK_IMAGE_LAYOUT_GENERAL, StagingBuffer->GetHandle(), 1, &CopyRegion);

	VkBufferMemoryBarrier Barrier;
	VulkanRHI::SetupAndZeroBufferBarrier(Barrier, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, StagingBuffer->GetHandle(), StagingBuffer->GetOffset(), Size);
	VulkanRHI::vkCmdPipelineBarrier(CmdBuffer->GetHandle(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &Barrier, 0, nullptr);

	// Force upload
	Device->GetImmediateContext().GetCommandBufferManager()->SubmitUploadCmdBuffer(true);
	Device->WaitUntilIdle();

	VkMappedMemoryRange MappedRange;
	FMemory::Memzero(MappedRange);
	MappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	MappedRange.memory = StagingBuffer->GetDeviceMemoryHandle();
	MappedRange.offset = StagingBuffer->GetOffset();
	MappedRange.size = Size;
	VulkanRHI::vkInvalidateMappedMemoryRanges(Device->GetInstanceHandle(), 1, &MappedRange);
	VulkanRHI::vkFlushMappedMemoryRanges(Device->GetInstanceHandle(), 1, &MappedRange);

	OutData.SetNum(NumPixels);
	FColor* Dest = OutData.GetData();
	for (int32 Row = Rect.Min.Y; Row < Rect.Max.Y; ++Row)
	{
		FColor* Src = (FColor*)StagingBuffer->GetMappedPointer() + Row * TextureRHI2D->GetSizeX() + Rect.Min.X;
		for (int32 Col = Rect.Min.X; Col < Rect.Max.X; ++Col)
		{
			*Dest++ = *Src++;
		}
	}
	Device->GetStagingManager().ReleaseBuffer(CmdBuffer, StagingBuffer);
}

void FVulkanDynamicRHI::RHIMapStagingSurface(FTextureRHIParamRef TextureRHI,void*& OutData,int32& OutWidth,int32& OutHeight)
{
	VULKAN_SIGNAL_UNIMPLEMENTED();
}

void FVulkanDynamicRHI::RHIUnmapStagingSurface(FTextureRHIParamRef TextureRHI)
{
	VULKAN_SIGNAL_UNIMPLEMENTED();
}

void FVulkanDynamicRHI::RHIReadSurfaceFloatData(FTextureRHIParamRef TextureRHI, FIntRect Rect, TArray<FFloat16Color>& OutData, ECubeFace CubeFace,int32 ArrayIndex,int32 MipIndex)
{
	auto DoCopyFloat = [](FVulkanDevice* InDevice, FVulkanCmdBuffer* CmdBuffer, const FVulkanSurface& Surface, uint32 MipIndex, uint32 SrcBaseArrayLayer, FIntRect Rect, TArray<FFloat16Color>& OutData)
	{
		ensure(Surface.InternalFormat == VK_FORMAT_R16G16B16A16_SFLOAT);

		uint32 NumPixels = (Surface.Width >> MipIndex) * (Surface.Height >> MipIndex);
		const uint32 Size = NumPixels * sizeof(FLinearColor);
		VulkanRHI::FStagingBuffer* StagingBuffer = InDevice->GetStagingManager().AcquireBuffer(Size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);
		VkBufferImageCopy CopyRegion;
		FMemory::Memzero(CopyRegion);
		//Region.bufferOffset = 0;
		CopyRegion.bufferRowLength = Surface.Width >> MipIndex;
		CopyRegion.bufferImageHeight = Surface.Height >> MipIndex;
		CopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		CopyRegion.imageSubresource.mipLevel = MipIndex;
		CopyRegion.imageSubresource.baseArrayLayer = SrcBaseArrayLayer;
		CopyRegion.imageSubresource.layerCount = 1;
		CopyRegion.imageExtent.width = Surface.Width >> MipIndex;
		CopyRegion.imageExtent.height = Surface.Height >> MipIndex;
		CopyRegion.imageExtent.depth = 1;
		VulkanRHI::vkCmdCopyImageToBuffer(CmdBuffer->GetHandle(), Surface.Image, VK_IMAGE_LAYOUT_GENERAL, StagingBuffer->GetHandle(), 1, &CopyRegion);

		VkBufferMemoryBarrier Barrier;
		VulkanRHI::SetupAndZeroBufferBarrier(Barrier, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, StagingBuffer->GetHandle(), StagingBuffer->GetOffset(), Size);
		VulkanRHI::vkCmdPipelineBarrier(CmdBuffer->GetHandle(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &Barrier, 0, nullptr);

		// Force upload
		InDevice->GetImmediateContext().GetCommandBufferManager()->SubmitUploadCmdBuffer(true);
		InDevice->WaitUntilIdle();

		VkMappedMemoryRange MappedRange;
		FMemory::Memzero(MappedRange);
		MappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		MappedRange.memory = StagingBuffer->GetDeviceMemoryHandle();
		MappedRange.offset = StagingBuffer->GetOffset();
		MappedRange.size = Size;
		VulkanRHI::vkInvalidateMappedMemoryRanges(InDevice->GetInstanceHandle(), 1, &MappedRange);
		VulkanRHI::vkFlushMappedMemoryRanges(InDevice->GetInstanceHandle(), 1, &MappedRange);

		OutData.SetNum(NumPixels);
		FFloat16Color* Dest = OutData.GetData();
		for (int32 Row = Rect.Min.Y; Row < Rect.Max.Y; ++Row)
		{
			FLinearColor* Src = (FLinearColor*)StagingBuffer->GetMappedPointer() + Row * (Surface.Width >> MipIndex) + Rect.Min.X;
			for (int32 Col = Rect.Min.X; Col < Rect.Max.X; ++Col)
			{
				*Dest++ = FFloat16Color(*Src++);
			}
		}
		InDevice->GetStagingManager().ReleaseBuffer(CmdBuffer, StagingBuffer);
	};

	FVulkanCmdBuffer* CmdBuffer = Device->GetImmediateContext().GetCommandBufferManager()->GetUploadCmdBuffer();
	if (TextureRHI->GetTextureCube())
	{
		FRHITextureCube* TextureRHICube = TextureRHI->GetTextureCube();
		FVulkanTextureCube* TextureCube = (FVulkanTextureCube*)TextureRHICube;
		DoCopyFloat(Device, CmdBuffer, TextureCube->Surface, MipIndex, CubeFace, Rect, OutData);
	}
	else
	{
		FRHITexture2D* TextureRHI2D = TextureRHI->GetTexture2D();
		check(TextureRHI2D);
		FVulkanTexture2D* Texture2D = (FVulkanTexture2D*)TextureRHI2D;
		DoCopyFloat(Device, CmdBuffer, Texture2D->Surface, MipIndex, 1, Rect, OutData);
	}
}

void FVulkanDynamicRHI::RHIRead3DSurfaceFloatData(FTextureRHIParamRef TextureRHI,FIntRect InRect,FIntPoint ZMinMax,TArray<FFloat16Color>& OutData)
{
	VULKAN_SIGNAL_UNIMPLEMENTED();
}

void FVulkanCommandListContext::RHITransitionResources(EResourceTransitionAccess TransitionType, EResourceTransitionPipeline TransitionPipeline, FUnorderedAccessViewRHIParamRef* InUAVs, int32 NumUAVs, FComputeFenceRHIParamRef WriteComputeFence)
{
	IRHICommandContext::RHITransitionResources(TransitionType, TransitionPipeline, InUAVs, NumUAVs, WriteComputeFence);
	ensure(NumUAVs == 0);
}

void FVulkanCommandListContext::RHITransitionResources(EResourceTransitionAccess TransitionType, FTextureRHIParamRef* InTextures, int32 NumTextures)
{
#if VULKAN_USE_NEW_RENDERPASSES
	static IConsoleVariable* CVarShowTransitions = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ProfileGPU.ShowTransitions"));
	bool bShowTransitionEvents = CVarShowTransitions->GetInt() != 0;

	SCOPED_RHI_CONDITIONAL_DRAW_EVENTF(*this, RHITransitionResources, bShowTransitionEvents, TEXT("TransitionTo: %s: %i Textures"), *FResourceTransitionUtility::ResourceTransitionAccessStrings[(int32)TransitionType], NumTextures);

	//FRCLog::Printf(FString::Printf(TEXT("RHITransitionResources")));
	if (NumTextures == 0)
	{
		return;
	}

	if (TransitionType == EResourceTransitionAccess::EReadable)
	{
		TArray<VkImageMemoryBarrier> ReadBarriers;
		ReadBarriers.AddZeroed(NumTextures);
		uint32 NumBarriers = 0;

		FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();

		if (RenderPassState.CurrentRenderPass)
		{
			bool bEndedRenderPass = false;
			// If any of the textures are in the current render pass, we need to end it
			for (int32 Index = 0; Index < NumTextures; ++Index)
			{
				SCOPED_RHI_CONDITIONAL_DRAW_EVENTF(*this, RHITransitionResourcesLoop, bShowTransitionEvents, TEXT("To:%i - %s"), Index, *InTextures[Index]->GetName().ToString());

				FVulkanTextureBase* VulkanTexture = FVulkanTextureBase::Cast(InTextures[Index]);
				bool bIsDepthStencil = (VulkanTexture->Surface.GetFullAspectMask() & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0;
				VkImageLayout SrcLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				VkImageLayout DstLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				if (RenderPassState.CurrentFramebuffer->ContainsRenderTarget(VulkanTexture->Surface.Image))
				{
					if (!bEndedRenderPass)
					{
						// If at least one of the textures is inside the render pass, need to end it
						RenderPassState.EndRenderPass(CmdBuffer);
						bEndedRenderPass = true;
					}

					SrcLayout = bIsDepthStencil ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				}
				else
				{
					VkImageLayout* FoundLayout = RenderPassState.CurrentLayout.Find(VulkanTexture->Surface.Image);
					if (FoundLayout)
					{
						SrcLayout = *FoundLayout;
					}
					else
					{
						ensure(0);
					}
				}

				DstLayout = bIsDepthStencil ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				if (SrcLayout == DstLayout)
				{
					// Ignore redundant layouts
				}
				else
				{
					VkAccessFlags SrcMask = VulkanRHI::GetAccessMask(SrcLayout);
					VkAccessFlags DstMask = VulkanRHI::GetAccessMask(DstLayout);
					VulkanRHI::SetupImageBarrier(ReadBarriers[NumBarriers], VulkanTexture->Surface, SrcMask, SrcLayout, DstMask, DstLayout);
					RenderPassState.CurrentLayout.FindOrAdd(VulkanTexture->Surface.Image) = DstLayout;
					++NumBarriers;
				}
			}
		}
		else
		{
			for (int32 Index = 0; Index < NumTextures; ++Index)
			{
				SCOPED_RHI_CONDITIONAL_DRAW_EVENTF(*this, RHITransitionResourcesLoop, bShowTransitionEvents, TEXT("To:%i - %s"), Index, *InTextures[Index]->GetName().ToString());

				FVulkanTextureBase* VulkanTexture = FVulkanTextureBase::Cast(InTextures[Index]);
				VkImageLayout SrcLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				VkImageLayout DstLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				bool bIsDepthStencil = (VulkanTexture->Surface.GetFullAspectMask() & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0;
				VkImageLayout* FoundLayout = RenderPassState.CurrentLayout.Find(VulkanTexture->Surface.Image);
				if (FoundLayout)
				{
					SrcLayout = *FoundLayout;
				}

				DstLayout = bIsDepthStencil ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				if (SrcLayout == DstLayout)
				{
					// Ignore redundant layouts
				}
				else
				{
					VkAccessFlags SrcMask = VulkanRHI::GetAccessMask(SrcLayout);
					VkAccessFlags DstMask = VulkanRHI::GetAccessMask(DstLayout);
					if (InTextures[Index]->GetTextureCube())
					{
						VulkanRHI::SetupImageBarrier(ReadBarriers[NumBarriers], VulkanTexture->Surface, SrcMask, SrcLayout, DstMask, DstLayout, 6);
					}
					else
					{
						ensure(InTextures[Index]->GetTexture2D() || InTextures[Index]->GetTexture3D());
						VulkanRHI::SetupImageBarrier(ReadBarriers[NumBarriers], VulkanTexture->Surface, SrcMask, SrcLayout, DstMask, DstLayout, 1);
					}
					RenderPassState.CurrentLayout.FindOrAdd(VulkanTexture->Surface.Image) = DstLayout;
					++NumBarriers;
				}
			}
		}

		if (NumBarriers > 0)
		{
			VkPipelineStageFlags SourceStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			VkPipelineStageFlags DestStages = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
			VulkanRHI::vkCmdPipelineBarrier(CmdBuffer->GetHandle(), SourceStages, DestStages, 0, 0, nullptr, 0, nullptr, NumBarriers, ReadBarriers.GetData());
		}
	}
	else if (TransitionType == EResourceTransitionAccess::EWritable)
	{
		FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
		check(CmdBuffer->HasBegun());

		auto SetImageLayout = [](FRenderPassState& RenderPassState, VkCommandBuffer CmdBuffer, FVulkanSurface& Surface, uint32 NumArraySlices)
		{
			if (RenderPassState.CurrentRenderPass)
			{
				ensure(RenderPassState.CurrentFramebuffer);
				if (RenderPassState.CurrentFramebuffer->ContainsRenderTarget(Surface.Image))
				{
#if DO_CHECK
					VkImageLayout* Found = RenderPassState.CurrentLayout.Find(Surface.Image);
					ensure(Found && (*Found == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL || *Found == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));
#endif
					// Redundant transition, so skip
					return;
				}
			}

			const VkImageAspectFlags AspectMask = Surface.GetFullAspectMask();
			VkImageSubresourceRange SubresourceRange ={AspectMask, 0, Surface.GetNumMips(), 0, NumArraySlices};
			if ((AspectMask & VK_IMAGE_ASPECT_COLOR_BIT) != 0)
			{
				VulkanSetImageLayout(CmdBuffer, Surface.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, SubresourceRange);
				RenderPassState.CurrentLayout.FindOrAdd(Surface.Image) = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}
			else
			{
				check((AspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != 0);
				VulkanSetImageLayout(CmdBuffer, Surface.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, SubresourceRange);
				RenderPassState.CurrentLayout.FindOrAdd(Surface.Image) = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
		};

		for (int32 i = 0; i < NumTextures; ++i)
		{
			SCOPED_RHI_CONDITIONAL_DRAW_EVENTF(*this, RHITransitionResourcesLoop, bShowTransitionEvents, TEXT("To:%i - %s"), i, *InTextures[i]->GetName().ToString());
			FRHITexture* RHITexture = InTextures[i];
		
			FRHITexture2D* RHITexture2D = RHITexture->GetTexture2D();
			if (RHITexture2D)
			{
				FVulkanTexture2D* Texture2D = (FVulkanTexture2D*)RHITexture2D;
				SetImageLayout(RenderPassState, CmdBuffer->GetHandle(), Texture2D->Surface, 1);
			}
			else
			{
				FRHITextureCube* RHITextureCube = RHITexture->GetTextureCube();
				if (RHITextureCube)
				{
					FVulkanTextureCube* TextureCube = (FVulkanTextureCube*)RHITextureCube;
					SetImageLayout(RenderPassState, CmdBuffer->GetHandle(), TextureCube->Surface, 6);
				}
				else
				{
					FRHITexture3D* RHITexture3D = RHITexture->GetTexture3D();
					if (RHITexture3D)
					{
						FVulkanTexture3D* Texture3D = (FVulkanTexture3D*)RHITexture3D;
						SetImageLayout(RenderPassState, CmdBuffer->GetHandle(), Texture3D->Surface, 1);
					}
					else
					{
						ensure(0);
					}
				}
			}
		}
	}
	else if (TransitionType == EResourceTransitionAccess::ERWSubResBarrier)
	{
	}
	else
	{
		ensure(0);
	}
#else
	if (TransitionType == EResourceTransitionAccess::EReadable)
	{
		const FResolveParams ResolveParams;
		for (int32 i = 0; i < NumTextures; ++i)
		{
			RHICopyToResolveTarget(InTextures[i], InTextures[i], true, ResolveParams);
		}
	}
	else if (TransitionType == EResourceTransitionAccess::EWritable)
	{
		for (int32 i = 0; i < NumTextures; ++i)
		{
			FRHITexture* RHITexture = InTextures[i];
			FRHITexture2D* RHITexture2D = RHITexture->GetTexture2D();
			if (RHITexture2D)
			{
				//FVulkanTexture2D* Texture = (FVulkanTexture2D*)RHITexture2D;
				//check(Texture->Surface.GetAspectMask() == VK_IMAGE_ASPECT_COLOR_BIT);
				//VulkanSetImageLayoutSimple(PendingState->GetCommandBuffer(), Texture->Surface.Image, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			}
			else
			{
				FRHITextureCube* RHITextureCube = RHITexture->GetTextureCube();
				if (RHITextureCube)
				{
				}
				else
				{
					FRHITexture3D* RHITexture3D = RHITexture->GetTexture3D();
					if (RHITexture3D)
					{
					}
					else
					{
						ensure(0);
					}
				}
			}
		}
	}
	else if (TransitionType == EResourceTransitionAccess::ERWSubResBarrier)
	{
	}
	else
	{
		ensure(0);
	}
#endif
}

// Need a separate struct so we can memzero/remove dependencies on reference counts
struct FRenderTargetLayoutHashableStruct
{
	// Depth goes in the last slot
	FRHITexture* Texture[MaxSimultaneousRenderTargets + 1];
	uint32 MipIndex[MaxSimultaneousRenderTargets];
	uint32 ArraySliceIndex[MaxSimultaneousRenderTargets];
	ERenderTargetLoadAction LoadAction[MaxSimultaneousRenderTargets];
	ERenderTargetStoreAction StoreAction[MaxSimultaneousRenderTargets];

	ERenderTargetLoadAction		DepthLoadAction;
	ERenderTargetStoreAction	DepthStoreAction;
	ERenderTargetLoadAction		StencilLoadAction;
	ERenderTargetStoreAction	StencilStoreAction;
	FExclusiveDepthStencil		DepthStencilAccess;

	// Fill this outside Set() function
	VkExtent2D					Extents;

	bool bClearDepth;
	bool bClearStencil;
	bool bClearColor;

	void Set(const FRHISetRenderTargetsInfo& RTInfo)
	{
		FMemory::Memzero(*this);
		for (int32 Index = 0; Index < RTInfo.NumColorRenderTargets; ++Index)
		{
			Texture[Index] = RTInfo.ColorRenderTarget[Index].Texture;
			MipIndex[Index] = RTInfo.ColorRenderTarget[Index].MipIndex;
			ArraySliceIndex[Index] = RTInfo.ColorRenderTarget[Index].ArraySliceIndex;
			LoadAction[Index] = RTInfo.ColorRenderTarget[Index].LoadAction;
			StoreAction[Index] = RTInfo.ColorRenderTarget[Index].StoreAction;
		}

		Texture[MaxSimultaneousRenderTargets] = RTInfo.DepthStencilRenderTarget.Texture;
		DepthLoadAction = RTInfo.DepthStencilRenderTarget.DepthLoadAction;
		DepthStoreAction = RTInfo.DepthStencilRenderTarget.DepthStoreAction;
		StencilLoadAction = RTInfo.DepthStencilRenderTarget.StencilLoadAction;
		StencilStoreAction = RTInfo.DepthStencilRenderTarget.GetStencilStoreAction();
		DepthStencilAccess = RTInfo.DepthStencilRenderTarget.GetDepthStencilAccess();

		bClearDepth = RTInfo.bClearDepth;
		bClearStencil = RTInfo.bClearStencil;
		bClearColor = RTInfo.bClearColor;
	}
};

FVulkanRenderTargetLayout::FVulkanRenderTargetLayout(const FRHISetRenderTargetsInfo& RTInfo)
	: NumAttachments(0)
	, NumColorAttachments(0)
	, bHasDepthStencil(false)
	, bHasResolveAttachments(false)
	, Hash(0)
{
	FMemory::Memzero(ColorReferences);
	FMemory::Memzero(ResolveReferences);
	FMemory::Memzero(DepthStencilReference);
	FMemory::Memzero(Desc);
	FMemory::Memzero(Extent);

	bool bSetExtent = false;
		
	for (int32 Index = 0; Index < RTInfo.NumColorRenderTargets; ++Index)
	{
		const FRHIRenderTargetView& RTView = RTInfo.ColorRenderTarget[Index];
		if (RTView.Texture)
		{
		    FVulkanTextureBase* Texture = FVulkanTextureBase::Cast(RTView.Texture);
		    check(Texture);
    
		    if (!bSetExtent)
		    {
			    bSetExtent = true;
			    Extent.Extent3D.width = Texture->Surface.Width >> RTView.MipIndex;
			    Extent.Extent3D.height = Texture->Surface.Height >> RTView.MipIndex;
#if VULKAN_USE_NEW_RENDERPASSES
			    Extent.Extent3D.depth = Texture->Surface.Depth;
#else
			    Extent.Extent3D.depth = 1;
#endif
		    }
    
		    uint32 NumSamples = RTView.Texture->GetNumSamples();
	    
		    VkAttachmentDescription& CurrDesc = Desc[NumAttachments];
		    
		    //@TODO: Check this, it should be a power-of-two. Might be a VulkanConvert helper function.
		    CurrDesc.samples = static_cast<VkSampleCountFlagBits>(NumSamples);
		    CurrDesc.format = UEToVkFormat(RTView.Texture->GetFormat(), (Texture->Surface.UEFlags & TexCreate_SRGB) == TexCreate_SRGB);
		    CurrDesc.loadOp = RenderTargetLoadActionToVulkan(RTView.LoadAction);
		    CurrDesc.storeOp = RenderTargetStoreActionToVulkan(RTView.StoreAction);
		    CurrDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
#if VULKAN_USE_NEW_RENDERPASSES || !VULKAN_USE_MSAA_RESOLVE_ATTACHMENTS
		    CurrDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
#endif
#if VULKAN_USE_NEW_RENDERPASSES
		    CurrDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		    CurrDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
#else
		    CurrDesc.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
		    CurrDesc.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
#endif
		    ColorReferences[NumColorAttachments].attachment = NumAttachments;
		    ColorReferences[NumColorAttachments].layout = VK_IMAGE_LAYOUT_GENERAL;
		    NumAttachments++;
#if !VULKAN_USE_NEW_RENDERPASSES
#if VULKAN_USE_MSAA_RESOLVE_ATTACHMENTS
			if (NumSamples == 1)
			{
			    CurrDesc.storeOp = RenderTargetStoreActionToVulkan(RTView.StoreAction);
    
			    ResolveReferences[NumColorAttachments].attachment = VK_ATTACHMENT_UNUSED;
			    ResolveReferences[NumColorAttachments].layout = VK_IMAGE_LAYOUT_UNDEFINED;
		    }
			else
		    {
			    // discard MSAA color target after resolve to 1x surface
			    CurrDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    
			    // Resolve attachment
			    VkAttachmentDescription& ResolveDesc = Desc[NumAttachments];
			    ResolveDesc.samples = VK_SAMPLE_COUNT_1_BIT;
			    ResolveDesc.format = (VkFormat)GPixelFormats[RTView.Texture->GetFormat()].PlatformFormat;
			    ResolveDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			    ResolveDesc.storeOp = RenderTargetStoreActionToVulkan(RTView.StoreAction);
			    ResolveDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			    ResolveDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			    ResolveDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			    ResolveDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
			    ResolveReferences[NumColorAttachments].attachment = NumAttachments;
			    ResolveReferences[NumColorAttachments].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			    NumAttachments++;
			    bHasResolveAttachments = true;
		    }
#endif
#endif
		    NumColorAttachments++;
		}
	}

	if (RTInfo.DepthStencilRenderTarget.Texture)
	{
		VkAttachmentDescription& CurrDesc = Desc[NumAttachments];
		FMemory::Memzero(CurrDesc);
		FVulkanTextureBase* Texture = FVulkanTextureBase::Cast(RTInfo.DepthStencilRenderTarget.Texture);
		check(Texture);

		//@TODO: Check this, it should be a power-of-two. Might be a VulkanConvert helper function.
		CurrDesc.samples = static_cast<VkSampleCountFlagBits>(RTInfo.DepthStencilRenderTarget.Texture->GetNumSamples());
		CurrDesc.format = UEToVkFormat(RTInfo.DepthStencilRenderTarget.Texture->GetFormat(), false);
		CurrDesc.loadOp = RenderTargetLoadActionToVulkan(RTInfo.DepthStencilRenderTarget.DepthLoadAction);
		CurrDesc.stencilLoadOp = RenderTargetLoadActionToVulkan(RTInfo.DepthStencilRenderTarget.StencilLoadAction);
		if (CurrDesc.samples == VK_SAMPLE_COUNT_1_BIT)
		{
			CurrDesc.storeOp = RenderTargetStoreActionToVulkan(RTInfo.DepthStencilRenderTarget.DepthStoreAction);
			CurrDesc.stencilStoreOp = RenderTargetStoreActionToVulkan(RTInfo.DepthStencilRenderTarget.GetStencilStoreAction());
		}
		else
		{
			// Never want to store MSAA depth/stencil
			CurrDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			CurrDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}
		CurrDesc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		CurrDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		
		DepthStencilReference.attachment = NumAttachments;
		DepthStencilReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		bHasDepthStencil = true;
		NumAttachments++;

		if (!bSetExtent)
		{
			bSetExtent = true;
			Extent.Extent3D.width = Texture->Surface.Width;
			Extent.Extent3D.height = Texture->Surface.Height;
			Extent.Extent3D.depth = 1;
		}
	}

	check(bSetExtent);

	FRenderTargetLayoutHashableStruct RTHash;
	FMemory::Memzero(RTHash);
	RTHash.Set(RTInfo);
	RTHash.Extents = Extent.Extent2D;
	Hash = FCrc::MemCrc32(&RTHash, sizeof(RTHash));
}
