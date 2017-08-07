// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "OculusHMD_CustomPresent.h"
#include "OculusHMDPrivateRHI.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS_D3D11
#include "OculusHMD.h"

#ifndef WINDOWS_PLATFORM_TYPES_GUARD
#include "AllowWindowsPlatformTypes.h"
#endif

namespace OculusHMD
{

//-------------------------------------------------------------------------------------------------
// D3D11CreateTexture2DAlias
//-------------------------------------------------------------------------------------------------

static FD3D11Texture2D* D3D11CreateTexture2DAlias(
	FD3D11DynamicRHI* InD3D11RHI,
	ID3D11Texture2D* InResource,
	ID3D11ShaderResourceView* InShaderResourceView,
	uint32 InSizeX,
	uint32 InSizeY,
	uint32 InSizeZ,
	uint32 InNumMips,
	uint32 InNumSamples,
	EPixelFormat InFormat,
	uint32 InFlags)
{
	const bool bSRGB = (InFlags & TexCreate_SRGB) != 0;

	const DXGI_FORMAT PlatformResourceFormat = (DXGI_FORMAT)GPixelFormats[InFormat].PlatformFormat;
	const DXGI_FORMAT PlatformShaderResourceFormat = FindShaderResourceDXGIFormat(PlatformResourceFormat, bSRGB);
	const DXGI_FORMAT PlatformRenderTargetFormat = FindShaderResourceDXGIFormat(PlatformResourceFormat, bSRGB);
	D3D11_RTV_DIMENSION RenderTargetViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	if (InNumSamples > 1)
	{
		RenderTargetViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
	}

	TArray<TRefCountPtr<ID3D11RenderTargetView> > RenderTargetViews;

	if (InFlags & TexCreate_RenderTargetable)
	{
		// Create a render target view for each mip
		for (uint32 MipIndex = 0; MipIndex < InNumMips; MipIndex++)
		{
			check(!(InFlags & TexCreate_TargetArraySlicesIndependently)); // not supported
			D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
			FMemory::Memzero(&RTVDesc, sizeof(RTVDesc));
			RTVDesc.Format = PlatformRenderTargetFormat;
			RTVDesc.ViewDimension = RenderTargetViewDimension;
			RTVDesc.Texture2D.MipSlice = MipIndex;

			TRefCountPtr<ID3D11RenderTargetView> RenderTargetView;
			VERIFYD3D11RESULT_EX(InD3D11RHI->GetDevice()->CreateRenderTargetView(InResource, &RTVDesc, RenderTargetView.GetInitReference()), InD3D11RHI->GetDevice());
			RenderTargetViews.Add(RenderTargetView);
		}
	}

	TRefCountPtr<ID3D11ShaderResourceView> ShaderResourceView;

	// Create a shader resource view for the texture.
	if (!InShaderResourceView && (InFlags & TexCreate_ShaderResource))
	{
		D3D11_SRV_DIMENSION ShaderResourceViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = PlatformShaderResourceFormat;

		SRVDesc.ViewDimension = ShaderResourceViewDimension;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.MipLevels = InNumMips;

		VERIFYD3D11RESULT_EX(InD3D11RHI->GetDevice()->CreateShaderResourceView(InResource, &SRVDesc, ShaderResourceView.GetInitReference()), InD3D11RHI->GetDevice());

		check(IsValidRef(ShaderResourceView));
	}
	else
	{
		ShaderResourceView = InShaderResourceView;
	}

	FD3D11Texture2D* NewTexture = new FD3D11Texture2D(
		InD3D11RHI,
		InResource,
		ShaderResourceView,
		false,
		1,
		RenderTargetViews,
		/*DepthStencilViews=*/ NULL,
		InSizeX,
		InSizeY,
		InSizeZ,
		InNumMips,
		InNumSamples,
		InFormat,
		/*bInCubemap=*/ false,
		InFlags,
		/*bPooledTexture=*/ false,
		FClearValueBinding::None
	);

	if (InFlags & TexCreate_RenderTargetable)
	{
		NewTexture->SetCurrentGPUAccess(EResourceTransitionAccess::EWritable);
	}

	return NewTexture;
}


//-------------------------------------------------------------------------------------------------
// FD3D11CustomPresent
//-------------------------------------------------------------------------------------------------

class FD3D11CustomPresent : public FCustomPresent
{
public:
	FD3D11CustomPresent(FOculusHMD* InOculusHMD);

	// Implementation of FCustomPresent, called by Plugin itself
	virtual ovrpRenderAPIType GetRenderAPI() const override;
	virtual bool IsUsingCorrectDisplayAdapter() override;
	virtual void UpdateMirrorTexture_RenderThread() override;

	virtual void* GetOvrpDevice() const override;
	virtual EPixelFormat GetDefaultPixelFormat() const override;
	virtual FTextureSetProxyPtr CreateTextureSet_RenderThread(uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, uint32 InNumMips, uint32 InNumSamples, uint32 InNumSamplesTileMem, uint32 InArraySize, bool bIsCubemap, const TArray<ovrpTextureHandle>& InTextures) override;
};


FD3D11CustomPresent::FD3D11CustomPresent(FOculusHMD* InOculusHMD) :
	FCustomPresent(InOculusHMD)
{
}


ovrpRenderAPIType FD3D11CustomPresent::GetRenderAPI() const
{
	return ovrpRenderAPI_D3D11;
}


bool FD3D11CustomPresent::IsUsingCorrectDisplayAdapter()
{
	const void* luid;

	if (OVRP_SUCCESS(ovrp_GetDisplayAdapterId2(&luid)) && luid)
	{
		TRefCountPtr<ID3D11Device> D3D11Device;

		ExecuteOnRenderThread([&D3D11Device]()
		{
			D3D11Device = (ID3D11Device*) RHIGetNativeDevice();
		});

		if (D3D11Device)
		{
			TRefCountPtr<IDXGIDevice> DXGIDevice;
			TRefCountPtr<IDXGIAdapter> DXGIAdapter;
			DXGI_ADAPTER_DESC DXGIAdapterDesc;

			if (SUCCEEDED(D3D11Device->QueryInterface(__uuidof(IDXGIDevice), (void**) DXGIDevice.GetInitReference())) &&
				SUCCEEDED(DXGIDevice->GetAdapter(DXGIAdapter.GetInitReference())) &&
				SUCCEEDED(DXGIAdapter->GetDesc(&DXGIAdapterDesc)))
			{
				return !FMemory::Memcmp(luid, &DXGIAdapterDesc.AdapterLuid, sizeof(LUID));
			}
		}
	}

	// Not enough information.  Assume that we are using the correct adapter.
	return true;
}


void FD3D11CustomPresent::UpdateMirrorTexture_RenderThread()
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
			int Width = (int)MirrorWindowSize.X;
			int Height = (int)MirrorWindowSize.Y;
			ovrpTextureHandle TextureHandle;

			ExecuteOnRHIThread([&]()
			{
				ovrp_SetupMirrorTexture2(GetOvrpDevice(), Height, Width, ovrpTextureFormat_B8G8R8A8_sRGB, &TextureHandle);
			});

			UE_LOG(LogHMD, Log, TEXT("Allocated a new mirror texture (size %d x %d)"), Width, Height);

			MirrorTextureRHI = D3D11CreateTexture2DAlias(
				static_cast<FD3D11DynamicRHI*>(GDynamicRHI),
				(ID3D11Texture2D*) TextureHandle,
				nullptr,
				Width,
				Height,
				0,
				1,
				/*ActualMSAACount=*/ 1,
				(EPixelFormat)PF_B8G8R8A8,
				TexCreate_ShaderResource);
		}
	}
}


void* FD3D11CustomPresent::GetOvrpDevice() const
{
	FD3D11DynamicRHI* DynamicRHI = static_cast<FD3D11DynamicRHI*>(GDynamicRHI);
	return DynamicRHI->GetDevice();
}


EPixelFormat FD3D11CustomPresent::GetDefaultPixelFormat() const
{
	return PF_B8G8R8A8;
}


FTextureSetProxyPtr FD3D11CustomPresent::CreateTextureSet_RenderThread(uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, uint32 InNumMips, uint32 InNumSamples, uint32 InNumSamplesTileMem, uint32 InArraySize, bool bIsCubemap, const TArray<ovrpTextureHandle>& InTextures)
{
	CheckInRenderThread();

	return CreateTextureSetProxy_D3D11(InFormat, InTextures);
}


//-------------------------------------------------------------------------------------------------
// APIs
//-------------------------------------------------------------------------------------------------

FCustomPresent* CreateCustomPresent_D3D11(FOculusHMD* InOculusHMD)
{
	return new FD3D11CustomPresent(InOculusHMD);
}


} // namespace OculusHMD

#if PLATFORM_WINDOWS
#undef WINDOWS_PLATFORM_TYPES_GUARD
#endif

#endif // OCULUS_HMD_SUPPORTED_PLATFORMS_D3D11
