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

void FVulkanCommandListContext::RHISetRenderTargets(uint32 NumSimultaneousRenderTargets, const FRHIRenderTargetView* NewRenderTargets, const FRHIDepthRenderTargetView* NewDepthStencilTarget, uint32 NumUAVs, const FUnorderedAccessViewRHIParamRef* UAVs)
{
	//@TODO: call SetRenderTargetsAndClear?
	FRHIDepthRenderTargetView DepthView;
	if (NewDepthStencilTarget)
	{
		DepthView = *NewDepthStencilTarget;
	}
	else
	{
		DepthView = FRHIDepthRenderTargetView(FTextureRHIParamRef(), ERenderTargetLoadAction::EClear, ERenderTargetStoreAction::ENoAction);
	}

	FRHISetRenderTargetsInfo Info(NumSimultaneousRenderTargets, NewRenderTargets, DepthView);
	RHISetRenderTargetsAndClear(Info);

	checkf(NumUAVs == 0, TEXT("Calling SetRenderTargets with UAVs is not supported in Vulkan yet"));
}

void FVulkanDynamicRHI::RHIDiscardRenderTargets(bool Depth, bool Stencil, uint32 ColorBitMask)
{
}

void FVulkanCommandListContext::RHISetRenderTargetsAndClear(const FRHISetRenderTargetsInfo& RenderTargetsInfo)
{
	PendingState->SetRenderTargetsInfo(RenderTargetsInfo);

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
		CmdBuffer->End();

		FVulkanSemaphore* BackBufferAcquiredSemaphore = Framebuffer->GetBackBuffer() ? Framebuffer->GetBackBuffer()->AcquiredSemaphore : nullptr;

		Device->GetQueue()->Submit(CmdBuffer, BackBufferAcquiredSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, nullptr);
		// No need to acquire it anymore
		Framebuffer->GetBackBuffer()->AcquiredSemaphore = nullptr;
		CommandBufferManager->PrepareForNewActiveCommandBuffer();
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
}

void FVulkanDynamicRHI::RHIReadSurfaceData(FTextureRHIParamRef TextureRHI, FIntRect Rect, TArray<FColor>& OutData, FReadSurfaceDataFlags InFlags)
{
	if (!ensure(TextureRHI))
	{
		OutData.Empty();
		OutData.AddZeroed(Rect.Width() * Rect.Height());
		return;
	}

	VULKAN_SIGNAL_UNIMPLEMENTED();
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
	FVulkanTextureBase* Texture = FVulkanTextureBase::Cast(TextureRHI);
	FVulkanSurface& Surface = Texture->Surface;

	// By pass for now
	#if 1
		for(uint32 Index=0; Index<Surface.Width*Surface.Height; Index++)
		{
			OutData.Add(FFloat16Color(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f)));
		}
	#endif
}

void FVulkanDynamicRHI::RHIRead3DSurfaceFloatData(FTextureRHIParamRef TextureRHI,FIntRect InRect,FIntPoint ZMinMax,TArray<FFloat16Color>& OutData)
{
	VULKAN_SIGNAL_UNIMPLEMENTED();
}


void FVulkanCommandListContext::RHITransitionResources(EResourceTransitionAccess TransitionType, FTextureRHIParamRef* InTextures, int32 NumTextures)
{
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
				FVulkanTexture2D* Texture = (FVulkanTexture2D*)RHITexture2D;
				if (Texture->Surface.ImageLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
				{
#if 1//VULKAN_USE_NEW_COMMAND_BUFFERS
					check(0);
#else
					check(Texture->Surface.GetAspectMask() == VK_IMAGE_ASPECT_COLOR_BIT);
					VulkanSetImageLayoutSimple(PendingState->GetCommandBuffer(), Texture->Surface.Image, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
#endif
				}
			}
			else
			{
				FRHITextureCube* RHITextureCube = RHITexture->GetTextureCube();
				if (RHITextureCube)
				{
				}
				else
				{
					ensure(0);
				}
			}
		}
	}
	else
	{
		ensure(0);
	}
}
