// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OculusHMD_CustomPresent.h"
#include "OculusHMDPrivateRHI.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS_D3D12
#include "OculusHMD.h"

#ifndef WINDOWS_PLATFORM_TYPES_GUARD
#include "AllowWindowsPlatformTypes.h"
#endif

namespace OculusHMD
{

//-------------------------------------------------------------------------------------------------
// FCustomPresentD3D12
//-------------------------------------------------------------------------------------------------

class FD3D12CustomPresent : public FCustomPresent
{
public:
	FD3D12CustomPresent(FOculusHMD* InOculusHMD);

	// Implementation of FCustomPresent, called by Plugin itself
	virtual ovrpRenderAPIType GetRenderAPI() const override;
	virtual bool IsUsingCorrectDisplayAdapter() override;
	virtual void UpdateMirrorTexture_RenderThread() override;

	virtual void* GetOvrpDevice() const override;
	virtual EPixelFormat GetDefaultPixelFormat() const override;
	virtual FTextureSetProxyPtr CreateTextureSet_RenderThread(uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, uint32 InNumMips, uint32 InNumSamples, uint32 InNumSamplesTileMem, uint32 InArraySize, bool bIsCubemap, const TArray<ovrpTextureHandle>& InTextures) override;
};


FD3D12CustomPresent::FD3D12CustomPresent(FOculusHMD* InOculusHMD) :
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


ovrpRenderAPIType FD3D12CustomPresent::GetRenderAPI() const
{
	return ovrpRenderAPI_D3D12;
}


bool FD3D12CustomPresent::IsUsingCorrectDisplayAdapter()
{
	const void* luid;

	if (OVRP_SUCCESS(ovrp_GetDisplayAdapterId2(&luid)) && luid)
	{
		TRefCountPtr<ID3D12Device> D3DDevice;

		ExecuteOnRenderThread([&D3DDevice]()
		{
			D3DDevice = (ID3D12Device*) RHIGetNativeDevice();
		});

		if (D3DDevice)
		{
			LUID AdapterLuid = D3DDevice->GetAdapterLuid();
			return !FMemory::Memcmp(luid, &AdapterLuid, sizeof(LUID));
		}
	}

	// Not enough information.  Assume that we are using the correct adapter.
	return true;
}

void FD3D12CustomPresent::UpdateMirrorTexture_RenderThread()
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
			FD3D12DynamicRHI* DynamicRHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
			int Width = (int)MirrorWindowSize.X;
			int Height = (int)MirrorWindowSize.Y;
			ovrpTextureHandle TextureHandle;

			ExecuteOnRHIThread([&]()
			{
				ovrp_SetupMirrorTexture2(GetOvrpDevice(), Height, Width, ovrpTextureFormat_B8G8R8A8_sRGB, &TextureHandle);
			});

			UE_LOG(LogHMD, Log, TEXT("Allocated a new mirror texture (size %d x %d)"), Width, Height);

			MirrorTextureRHI = DynamicRHI->RHICreateTexture2DFromD3D12Resource(
				PF_B8G8R8A8,
				TexCreate_ShaderResource,
				FClearValueBinding::None,
				(ID3D12Resource*) TextureHandle);
		}
	}
}


void* FD3D12CustomPresent::GetOvrpDevice() const
{
	FD3D12DynamicRHI* DynamicRHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
	return DynamicRHI->RHIGetD3DCommandQueue();
}


EPixelFormat FD3D12CustomPresent::GetDefaultPixelFormat() const
{
	return PF_B8G8R8A8;
}


FTextureSetProxyPtr FD3D12CustomPresent::CreateTextureSet_RenderThread(uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, uint32 InNumMips, uint32 InNumSamples, uint32 InNumSamplesTileMem, uint32 InArraySize, bool bIsCubemap, const TArray<ovrpTextureHandle>& InTextures)
{
	CheckInRenderThread();

	FD3D12DynamicRHI* DynamicRHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);

	FTexture2DRHIRef RHITexture;
	{
		RHITexture = DynamicRHI->RHICreateTexture2DFromD3D12Resource(InFormat, TexCreate_ShaderResource, FClearValueBinding::None, (ID3D12Resource*) InTextures[0]);
	}

	TArray<FTexture2DRHIRef> RHITextureSwapChain;
	{
		for (int32 TextureIndex = 0; TextureIndex < InTextures.Num(); ++TextureIndex)
		{
			RHITextureSwapChain.Add(DynamicRHI->RHICreateTexture2DFromD3D12Resource(InFormat, TexCreate_ShaderResource, FClearValueBinding::None, (ID3D12Resource*) InTextures[TextureIndex]));
		}
	}

	return CreateTextureSetProxy_D3D12(RHITexture, RHITextureSwapChain);
}


//-------------------------------------------------------------------------------------------------
// APIs
//-------------------------------------------------------------------------------------------------

FCustomPresent* CreateCustomPresent_D3D12(FOculusHMD* InOculusHMD)
{
	return new FD3D12CustomPresent(InOculusHMD);
}


} // namespace OculusHMD

#if PLATFORM_WINDOWS
#undef WINDOWS_PLATFORM_TYPES_GUARD
#endif

#endif // OCULUS_HMD_SUPPORTED_PLATFORMS_D3D12
