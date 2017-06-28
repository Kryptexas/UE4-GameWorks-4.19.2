// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "OculusHMD_TextureSetProxy.h"
#include "OculusHMDPrivateRHI.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS_D3D11
#include "OculusHMD_CustomPresent.h"

#ifndef WINDOWS_PLATFORM_TYPES_GUARD
#include "AllowWindowsPlatformTypes.h"
#endif

namespace OculusHMD
{

//-------------------------------------------------------------------------------------------------
// FD3D11Texture2DSet
//-------------------------------------------------------------------------------------------------

class FD3D11Texture2DSet : public FD3D11Texture2D
{
protected:
	FD3D11Texture2DSet(FD3D11DynamicRHI* InD3DRHI, ID3D11Texture2D* InResource, ID3D11ShaderResourceView* InShaderResourceView,
		bool bInCreatedRTVsPerSlice, int32 InRTVArraySize, const TArray<TRefCountPtr<ID3D11RenderTargetView> >& InRenderTargetViews,
		TRefCountPtr<ID3D11DepthStencilView>* InDepthStencilViews, uint32 InSizeX, uint32 InSizeY, uint32 InSizeZ,
		uint32 InNumMips, uint32 InNumSamples, EPixelFormat InFormat, bool bInCubemap, uint32 InFlags, bool bInPooled);

	void AddTexture(ID3D11Texture2D* InTexture, ID3D11ShaderResourceView* InSRV, TArray<TRefCountPtr<ID3D11RenderTargetView> >* InRTVs = nullptr);

public:
	static FD3D11Texture2DSet* CreateTextureSet_RenderThread(EPixelFormat InFormat, const TArray<ovrpTextureHandle>& InTextures);

	void AliasResources_RHIThread(uint32 SwapChainIndex);
	void ReleaseResources_RHIThread();

protected:
	struct SwapChainElement
	{
		TRefCountPtr<ID3D11Texture2D> Texture;
		TRefCountPtr<ID3D11ShaderResourceView> SRV;
		TArray<TRefCountPtr<ID3D11RenderTargetView> > RTVs;
	};

	TArray<SwapChainElement> SwapChainElements;
};


FD3D11Texture2DSet::FD3D11Texture2DSet(FD3D11DynamicRHI* InD3DRHI, ID3D11Texture2D* InResource, ID3D11ShaderResourceView* InShaderResourceView,
	bool bInCreatedRTVsPerSlice, int32 InRTVArraySize, const TArray<TRefCountPtr<ID3D11RenderTargetView> >& InRenderTargetViews,
	TRefCountPtr<ID3D11DepthStencilView>* InDepthStencilViews, uint32 InSizeX, uint32 InSizeY, uint32 InSizeZ,
	uint32 InNumMips, uint32 InNumSamples, EPixelFormat InFormat, bool bInCubemap, uint32 InFlags, bool bInPooled)

	: FD3D11Texture2D(InD3DRHI, InResource, InShaderResourceView, bInCreatedRTVsPerSlice, InRTVArraySize, InRenderTargetViews, InDepthStencilViews,
		InSizeX, InSizeY, InSizeZ, InNumMips, InNumSamples, InFormat, bInCubemap, InFlags, bInPooled, FClearValueBinding::None)
{
}


void FD3D11Texture2DSet::AddTexture(ID3D11Texture2D* InTexture, ID3D11ShaderResourceView* InSRV, TArray<TRefCountPtr<ID3D11RenderTargetView> >* InRTVs)
{
	SwapChainElement element;
	element.Texture = InTexture;
	element.SRV = InSRV;
	if (InRTVs)
	{
		element.RTVs.Empty(InRTVs->Num());
		for (int32 i = 0; i < InRTVs->Num(); ++i)
		{
			element.RTVs.Add((*InRTVs)[i]);
		}
	}
	SwapChainElements.Push(element);
}


FD3D11Texture2DSet* FD3D11Texture2DSet::CreateTextureSet_RenderThread(EPixelFormat InFormat, const TArray<ovrpTextureHandle>& InTextures)
{
	CheckInRenderThread();

	if (!InTextures.Num() || !InTextures[0])
	{
		return nullptr;
	}

	D3D11_TEXTURE2D_DESC TextureDesc;
	((ID3D11Texture2D*) InTextures[0])->GetDesc(&TextureDesc);

	uint32 TexCreateFlags = TexCreate_ShaderResource | TexCreate_RenderTargetable;

	FD3D11DynamicRHI* D3D11RHI = (FD3D11DynamicRHI*) GDynamicRHI;


	TArray<TRefCountPtr<ID3D11RenderTargetView> > TextureSetRenderTargetViews;
	FD3D11Texture2DSet* NewTextureSet = new FD3D11Texture2DSet(
		D3D11RHI,
		nullptr,
		nullptr,
		false,
		1,
		TextureSetRenderTargetViews,
		/*DepthStencilViews=*/ NULL,
		TextureDesc.Width,
		TextureDesc.Height,
		0,
		TextureDesc.MipLevels,
		TextureDesc.SampleDesc.Count,
		InFormat,
		/*bInCubemap=*/ false,
		TexCreateFlags,
		/*bPooledTexture=*/ false
	);

	int32 TextureCount = InTextures.Num();
	const bool bSRGB = (TexCreateFlags & TexCreate_SRGB) != 0;

	const DXGI_FORMAT PlatformResourceFormat = (DXGI_FORMAT)GPixelFormats[InFormat].PlatformFormat;
	const DXGI_FORMAT PlatformShaderResourceFormat = FindShaderResourceDXGIFormat(PlatformResourceFormat, bSRGB);
	const DXGI_FORMAT PlatformRenderTargetFormat = FindShaderResourceDXGIFormat(PlatformResourceFormat, bSRGB);
	D3D11_RTV_DIMENSION RenderTargetViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	if (TextureDesc.SampleDesc.Count > 1)
	{
		RenderTargetViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
	}

	for (int32 TextureIndex = 0; TextureIndex < TextureCount; ++TextureIndex)
	{
		ID3D11Texture2D* pD3DTexture = (ID3D11Texture2D*) InTextures[TextureIndex];

		TArray<TRefCountPtr<ID3D11RenderTargetView> > RenderTargetViews;
		if (TexCreateFlags & TexCreate_RenderTargetable)
		{
			// Create a render target view for each mip
			for (uint32 MipIndex = 0; MipIndex < TextureDesc.MipLevels; MipIndex++)
			{
				check(!(TexCreateFlags & TexCreate_TargetArraySlicesIndependently)); // not supported
				D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
				FMemory::Memzero(&RTVDesc, sizeof(RTVDesc));
				RTVDesc.Format = PlatformRenderTargetFormat;
				RTVDesc.ViewDimension = RenderTargetViewDimension;
				RTVDesc.Texture2D.MipSlice = MipIndex;

				TRefCountPtr<ID3D11RenderTargetView> RenderTargetView;
				VERIFYD3D11RESULT_EX(D3D11RHI->GetDevice()->CreateRenderTargetView(pD3DTexture, &RTVDesc, RenderTargetView.GetInitReference()), D3D11RHI->GetDevice());
				RenderTargetViews.Add(RenderTargetView);
			}
		}

		TRefCountPtr<ID3D11ShaderResourceView> ShaderResourceView;

		// Create a shader resource view for the texture.
		if (TexCreateFlags & TexCreate_ShaderResource)
		{
			D3D11_SRV_DIMENSION ShaderResourceViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
			SRVDesc.Format = PlatformShaderResourceFormat;

			SRVDesc.ViewDimension = ShaderResourceViewDimension;
			SRVDesc.Texture2D.MostDetailedMip = 0;
			SRVDesc.Texture2D.MipLevels = TextureDesc.MipLevels;

			VERIFYD3D11RESULT_EX(D3D11RHI->GetDevice()->CreateShaderResourceView(pD3DTexture, &SRVDesc, ShaderResourceView.GetInitReference()), D3D11RHI->GetDevice());

			check(IsValidRef(ShaderResourceView));
		}

		NewTextureSet->AddTexture(pD3DTexture, ShaderResourceView, &RenderTargetViews);
	}

	if (TexCreateFlags & TexCreate_RenderTargetable)
	{
		NewTextureSet->SetCurrentGPUAccess(EResourceTransitionAccess::EWritable);
	}

	ExecuteOnRHIThread([&]()
	{
		NewTextureSet->AliasResources_RHIThread(0);
	});

	return NewTextureSet;
}


void FD3D11Texture2DSet::AliasResources_RHIThread(uint32 SwapChainIndex)
{
	CheckInRHIThread();

	Resource = SwapChainElements[SwapChainIndex].Texture;
	ShaderResourceView = SwapChainElements[SwapChainIndex].SRV;

	RenderTargetViews.Empty(SwapChainElements[SwapChainIndex].RTVs.Num());
	for (int32 i = 0; i < SwapChainElements[SwapChainIndex].RTVs.Num(); ++i)
	{
		RenderTargetViews.Add(SwapChainElements[SwapChainIndex].RTVs[i]);
	}
}


void FD3D11Texture2DSet::ReleaseResources_RHIThread()
{
	CheckInRHIThread();

	SwapChainElements.Empty(0);
}


//-------------------------------------------------------------------------------------------------
// FD3D11TextureSetProxy
//-------------------------------------------------------------------------------------------------

class FD3D11TextureSetProxy : public FTextureSetProxy
{
public:
	FD3D11TextureSetProxy(FTextureRHIRef InTexture, uint32 InSwapChainLength)
		: FTextureSetProxy(InTexture, InSwapChainLength) {}

	virtual ~FD3D11TextureSetProxy()
	{
		FD3D11Texture2DSet* D3D11TS = static_cast<FD3D11Texture2DSet*>(RHITexture->GetTexture2D());

		if (InRenderThread())
		{
			ExecuteOnRHIThread([&]()
			{
				D3D11TS->ReleaseResources_RHIThread();
			});
		}
		else
		{
			D3D11TS->ReleaseResources_RHIThread();
		}

		RHITexture = nullptr;
	}

protected:
	virtual void AliasResources_RHIThread() override
	{
		auto D3D11TS = static_cast<FD3D11Texture2DSet*>(RHITexture->GetTexture2D());
		D3D11TS->AliasResources_RHIThread(SwapChainIndex_RHIThread);
	}
};


//-------------------------------------------------------------------------------------------------
// APIs
//-------------------------------------------------------------------------------------------------

FTextureSetProxyPtr CreateTextureSetProxy_D3D11(EPixelFormat InFormat, const TArray<ovrpTextureHandle>& InTextures)
{
	TRefCountPtr<FD3D11Texture2DSet> TextureSet = FD3D11Texture2DSet::CreateTextureSet_RenderThread(InFormat, InTextures);

	if (TextureSet)
	{
		return MakeShareable(new FD3D11TextureSetProxy(TextureSet->GetTexture2D(), InTextures.Num()));
	}

	return nullptr;
}


} // namespace OculusHMD

#if PLATFORM_WINDOWS
#undef WINDOWS_PLATFORM_TYPES_GUARD
#endif

#endif // OCULUS_HMD_SUPPORTED_PLATFORMS_D3D11
