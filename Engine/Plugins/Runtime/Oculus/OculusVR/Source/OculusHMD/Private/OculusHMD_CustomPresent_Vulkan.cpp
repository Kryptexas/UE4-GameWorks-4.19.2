// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OculusHMD_CustomPresent.h"
#include "OculusHMDPrivateRHI.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS_VULKAN
#include "OculusHMD.h"

#if PLATFORM_WINDOWS
#ifndef WINDOWS_PLATFORM_TYPES_GUARD
#include "AllowWindowsPlatformTypes.h"
#endif
#endif

namespace OculusHMD
{

//-------------------------------------------------------------------------------------------------
// FCustomPresentVulkan
//-------------------------------------------------------------------------------------------------

class FVulkanCustomPresent : public FCustomPresent
{
public:
	FVulkanCustomPresent(FOculusHMD* InOculusHMD);

	// Implementation of FCustomPresent, called by Plugin itself
	virtual ovrpRenderAPIType GetRenderAPI() const override;
	virtual bool IsUsingCorrectDisplayAdapter() override;
	virtual void UpdateMirrorTexture_RenderThread() override;

	virtual void* GetOvrpDevice() const override;
	virtual EPixelFormat GetDefaultPixelFormat() const override;
	virtual FTextureSetProxyPtr CreateTextureSet_RenderThread(uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, uint32 InNumMips, uint32 InNumSamples, uint32 InNumSamplesTileMem, uint32 InArraySize, bool bIsCubemap, const TArray<ovrpTextureHandle>& InTextures) override;
};


FVulkanCustomPresent::FVulkanCustomPresent(FOculusHMD* InOculusHMD) :
	FCustomPresent(InOculusHMD)
{
	CheckInGameThread();

#ifdef DISABLE_RHI_THREAD
	if (GRHISupportsRHIThread && GIsThreadedRendering && GUseRHIThread)
	{
		FSuspendRenderingThread SuspendRenderingThread(true);
		GUseRHIThread = false;
	}
#endif
}


ovrpRenderAPIType FVulkanCustomPresent::GetRenderAPI() const
{
	return ovrpRenderAPI_Vulkan;
}


bool FVulkanCustomPresent::IsUsingCorrectDisplayAdapter()
{
#if PLATFORM_WINDOWS
	// UNDONE
#endif
	return true;
}

void FVulkanCustomPresent::UpdateMirrorTexture_RenderThread()
{
	SCOPE_CYCLE_COUNTER(STAT_BeginRendering);

	CheckInRenderThread();

	const ESpectatorScreenMode MirrorWindowMode = OculusHMD->GetSpectatorScreenMode();
	const FVector2D MirrorWindowSize = OculusHMD->GetFrame_RenderThread()->WindowSize;

	if (ovrp_GetInitialized())
	{
		// Need to destroy mirror texture?
		if (MirrorTextureRHI.IsValid() && (MirrorWindowMode != ESpectatorScreenMode::Distorted ||
			MirrorWindowSize != FVector2D(MirrorTextureRHI->GetSizeX(), MirrorTextureRHI->GetSizeY())))
		{
			ExecuteOnRHIThread([]()
			{
				ovrp_DestroyMirrorTexture2();
			});

			MirrorTextureRHI = nullptr;
		}

		// Need to create mirror texture?
		if (!MirrorTextureRHI.IsValid() &&
			MirrorWindowMode == ESpectatorScreenMode::Distorted &&
			MirrorWindowSize.X != 0 && MirrorWindowSize.Y != 0)
		{
			FVulkanDynamicRHI* DynamicRHI = static_cast<FVulkanDynamicRHI*>(GDynamicRHI);
			int Width = (int)MirrorWindowSize.X;
			int Height = (int)MirrorWindowSize.Y;
			ovrpTextureHandle TextureHandle;

			ExecuteOnRHIThread([&]()
			{
				ovrp_SetupMirrorTexture2(GetOvrpDevice(), Height, Width, ovrpTextureFormat_B8G8R8A8_sRGB, &TextureHandle);
			});

			UE_LOG(LogHMD, Log, TEXT("Allocated a new mirror texture (size %d x %d)"), Width, Height);

			MirrorTextureRHI = DynamicRHI->RHICreateTexture2DFromVkImage(
				PF_R8G8B8A8,
				Width,
				Height,
				(VkImage) TextureHandle,
				TexCreate_ShaderResource);
		}
	}
}


void* FVulkanCustomPresent::GetOvrpDevice() const
{
	return RHIGetNativeDevice();
}


EPixelFormat FVulkanCustomPresent::GetDefaultPixelFormat() const
{
	return PF_R8G8B8A8;
}


FTextureSetProxyPtr FVulkanCustomPresent::CreateTextureSet_RenderThread(uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, uint32 InNumMips, uint32 InNumSamples, uint32 InNumSamplesTileMem, uint32 InArraySize, bool bIsCubemap, const TArray<ovrpTextureHandle>& InTextures)
{
	CheckInRenderThread();

	FVulkanDynamicRHI* DynamicRHI = static_cast<FVulkanDynamicRHI*>(GDynamicRHI);

	FTexture2DRHIRef RHITexture;
	{
		RHITexture = DynamicRHI->RHICreateTexture2DFromVkImage(InFormat, InSizeX, InSizeY, (VkImage) InTextures[0], TexCreate_ShaderResource);
	}

	TArray<FTexture2DRHIRef> RHITextureSwapChain;
	{
		for (int32 TextureIndex = 0; TextureIndex < InTextures.Num(); ++TextureIndex)
		{
			RHITextureSwapChain.Add(DynamicRHI->RHICreateTexture2DFromVkImage(InFormat, InSizeX, InSizeY, (VkImage) InTextures[TextureIndex], TexCreate_ShaderResource));
		}
	}

	return CreateTextureSetProxy_Vulkan(RHITexture, RHITextureSwapChain);
}


//-------------------------------------------------------------------------------------------------
// APIs
//-------------------------------------------------------------------------------------------------

FCustomPresent* CreateCustomPresent_Vulkan(FOculusHMD* InOculusHMD)
{
	return new FVulkanCustomPresent(InOculusHMD);
}


} // namespace OculusHMD

#if PLATFORM_WINDOWS
#undef WINDOWS_PLATFORM_TYPES_GUARD
#endif

#endif // OCULUS_HMD_SUPPORTED_PLATFORMS_VULKAN
