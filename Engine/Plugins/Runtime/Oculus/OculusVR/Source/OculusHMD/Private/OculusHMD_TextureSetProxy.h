// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OculusHMDPrivate.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS

namespace OculusHMD
{

//-------------------------------------------------------------------------------------------------
// FTextureSetProxy
//-------------------------------------------------------------------------------------------------

class FTextureSetProxy : public TSharedFromThis<FTextureSetProxy, ESPMode::ThreadSafe>
{
public:
	FTextureSetProxy(FTextureRHIParamRef InTexture, uint32 InSwapChainLength);
	virtual ~FTextureSetProxy() {}

	FRHITexture* GetTexture() const { return RHITexture.GetReference(); }
	FRHITexture2D* GetTexture2D() const { return RHITexture->GetTexture2D(); }
	FRHITextureCube* GetTextureCube() const { return RHITexture->GetTextureCube(); }
	uint32 GetSwapChainLength() const { return SwapChainLength; }

	void GenerateMips_RenderThread(FRHICommandListImmediate& RHICmdList);
	uint32 GetSwapChainIndex_RHIThread() { return SwapChainIndex_RHIThread; }
	void IncrementSwapChainIndex_RHIThread();

protected:
	virtual void AliasResources_RHIThread() = 0;

	FTextureRHIRef RHITexture;
	uint32 SwapChainLength;
	uint32 SwapChainIndex_RHIThread;
};

typedef TSharedPtr<FTextureSetProxy, ESPMode::ThreadSafe> FTextureSetProxyPtr;


//-------------------------------------------------------------------------------------------------
// APIs
//-------------------------------------------------------------------------------------------------

#if OCULUS_HMD_SUPPORTED_PLATFORMS_D3D11
FTextureSetProxyPtr CreateTextureSetProxy_D3D11(EPixelFormat InFormat, const TArray<ovrpTextureHandle>& InTextures);
#endif
#if OCULUS_HMD_SUPPORTED_PLATFORMS_D3D12
FTextureSetProxyPtr CreateTextureSetProxy_D3D12(FTexture2DRHIParamRef InRHITexture, const TArray<FTexture2DRHIRef>& InRHITextureSwapChain);
#endif
#if OCULUS_HMD_SUPPORTED_PLATFORMS_OPENGL
FTextureSetProxyPtr CreateTextureSetProxy_OpenGL(uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, uint32 InNumMips, uint32 InNumSamples, uint32 InNumSamplesTileMem, uint32 InArraySize, bool bIsCubemap, const TArray<ovrpTextureHandle>& InTextures);
#endif
#if OCULUS_HMD_SUPPORTED_PLATFORMS_VULKAN
FTextureSetProxyPtr CreateTextureSetProxy_Vulkan(FTexture2DRHIParamRef InRHITexture, const TArray<FTexture2DRHIRef>& InRHITextureSwapChain);
#endif


} // namespace OculusHMD

#endif //OCULUS_HMD_SUPPORTED_PLATFORMS
