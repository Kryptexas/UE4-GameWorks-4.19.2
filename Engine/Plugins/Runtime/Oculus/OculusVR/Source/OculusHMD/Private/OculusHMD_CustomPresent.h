// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OculusHMDPrivate.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS
#include "OculusHMD_Settings.h"
#include "OculusHMD_GameFrame.h"
#include "OculusHMD_TextureSetProxy.h"
#include "RHI.h"
#include "RendererInterface.h"
#include "IStereoLayers.h"

#if PLATFORM_WINDOWS
#include "WindowsHWrapper.h"
#endif

DECLARE_STATS_GROUP(TEXT("OculusHMD"), STATGROUP_OculusHMD, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("BeginRendering"), STAT_BeginRendering, STATGROUP_OculusHMD);
DECLARE_CYCLE_STAT(TEXT("FinishRendering"), STAT_FinishRendering, STATGROUP_OculusHMD);
DECLARE_FLOAT_COUNTER_STAT(TEXT("LatencyRender"), STAT_LatencyRender, STATGROUP_OculusHMD);
DECLARE_FLOAT_COUNTER_STAT(TEXT("LatencyTimewarp"), STAT_LatencyTimewarp, STATGROUP_OculusHMD);
DECLARE_FLOAT_COUNTER_STAT(TEXT("LatencyPostPresent"), STAT_LatencyPostPresent, STATGROUP_OculusHMD);
DECLARE_FLOAT_COUNTER_STAT(TEXT("ErrorRender"), STAT_ErrorRender, STATGROUP_OculusHMD);
DECLARE_FLOAT_COUNTER_STAT(TEXT("ErrorTimewarp"), STAT_ErrorTimewarp, STATGROUP_OculusHMD);

namespace OculusHMD
{

//-------------------------------------------------------------------------------------------------
// FCustomPresent
//-------------------------------------------------------------------------------------------------

class FCustomPresent : public FRHICustomPresent
{
public:
	FCustomPresent(class FOculusHMD* InOculusHMD);

	// FRHICustomPresent
	virtual void OnBackBufferResize() override;
	virtual bool Present(int32& SyncInterval) override;
	// virtual void PostPresent() override;

	virtual ovrpRenderAPIType GetRenderAPI() const = 0;
	virtual bool IsUsingCorrectDisplayAdapter() = 0;

	virtual void UpdateMirrorTexture_RenderThread() = 0;
	void FinishRendering_RHIThread();
	void ReleaseResources_RHIThread();
	void Shutdown();

	void UpdateViewport(FRHIViewport* InViewportRHI);

	FTexture2DRHIRef GetMirrorTexture() { return MirrorTextureRHI; }

	virtual void* GetOvrpDevice() const = 0;
	virtual EPixelFormat GetDefaultPixelFormat() const = 0;
	EPixelFormat GetPixelFormat(EPixelFormat InFormat) const;
	EPixelFormat GetPixelFormat(ovrpTextureFormat InFormat) const;
	ovrpTextureFormat GetOvrpTextureFormat(EPixelFormat InFormat, bool InSRGB) const;

	// Create texture set
	virtual FTextureSetProxyPtr CreateTextureSet_RenderThread(uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, uint32 InNumMips, uint32 InNumSamples, uint32 InNumSamplesTileMem, uint32 InArraySize, bool bIsCubemap, const TArray<ovrpTextureHandle>& InTextures) = 0;

	// Copies one texture to another
	void CopyTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRHIParamRef DstTexture, FTextureRHIParamRef SrcTexture, FIntRect DstRect = FIntRect(), FIntRect SrcRect = FIntRect(), bool bAlphaPremultiply = false, bool bNoAlphaWrite = false) const;

protected:
	FOculusHMD* OculusHMD;
	IRendererModule* RendererModule;
	FTexture2DRHIRef MirrorTextureRHI;
};


//-------------------------------------------------------------------------------------------------
// APIs
//-------------------------------------------------------------------------------------------------

#if OCULUS_HMD_SUPPORTED_PLATFORMS_D3D11
FCustomPresent* CreateCustomPresent_D3D11(FOculusHMD* InOculusHMD);
#endif
#if OCULUS_HMD_SUPPORTED_PLATFORMS_D3D12
FCustomPresent* CreateCustomPresent_D3D12(FOculusHMD* InOculusHMD);
#endif
#if OCULUS_HMD_SUPPORTED_PLATFORMS_OPENGL
FCustomPresent* CreateCustomPresent_OpenGL(FOculusHMD* InOculusHMD);
#endif
#if OCULUS_HMD_SUPPORTED_PLATFORMS_VULKAN
FCustomPresent* CreateCustomPresent_Vulkan(FOculusHMD* InOculusHMD);
#endif


} // namespace OculusHMD

#endif //OCULUS_HMD_SUPPORTED_PLATFORMS
