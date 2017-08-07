// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OculusHMD_TextureSetProxy.h"
#include "OculusHMDPrivateRHI.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS_D3D12
#include "OculusHMD_CustomPresent.h"

#ifndef WINDOWS_PLATFORM_TYPES_GUARD
#include "AllowWindowsPlatformTypes.h"
#endif

namespace OculusHMD
{

//-------------------------------------------------------------------------------------------------
// FD3D12TextureSetProxy
//-------------------------------------------------------------------------------------------------

class FD3D12TextureSetProxy : public FTextureSetProxy
{
public:
	FD3D12TextureSetProxy(FTexture2DRHIParamRef InRHITexture, const TArray<FTexture2DRHIRef>& InRHITextureSwapChain) :
		FTextureSetProxy(InRHITexture, InRHITextureSwapChain.Num()),
		RHITextureSwapChain(InRHITextureSwapChain) {}

	virtual ~FD3D12TextureSetProxy()
	{
		if (InRenderThread())
		{
			ExecuteOnRHIThread([this]()
			{
				ReleaseResources_RHIThread();
			});
		}
		else
		{
			ReleaseResources_RHIThread();
		}
	}

protected:
	virtual void AliasResources_RHIThread() override
	{
		CheckInRHIThread();

		FD3D12DynamicRHI* DynamicRHI = static_cast<FD3D12DynamicRHI*>(GDynamicRHI);
		DynamicRHI->RHIAliasTexture2DResources(RHITexture->GetTexture2D(), RHITextureSwapChain[SwapChainIndex_RHIThread]);
	}

	void ReleaseResources_RHIThread()
	{
		CheckInRHIThread();

		RHITexture = nullptr;
		RHITextureSwapChain.Empty();
	}

protected:
	TArray<FTexture2DRHIRef> RHITextureSwapChain;
};


//-------------------------------------------------------------------------------------------------
// APIs
//-------------------------------------------------------------------------------------------------

FTextureSetProxyPtr CreateTextureSetProxy_D3D12(FTexture2DRHIParamRef InRHITexture, const TArray<FTexture2DRHIRef>& InRHITextureSwapChain)
{
	return MakeShareable(new FD3D12TextureSetProxy(InRHITexture, InRHITextureSwapChain));
}


} // namespace OculusHMD

#if PLATFORM_WINDOWS
#undef WINDOWS_PLATFORM_TYPES_GUARD
#endif

#endif // OCULUS_HMD_SUPPORTED_PLATFORMS_D3D12
