// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "OculusHMD_TextureSetProxy.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS
//#include "OculusHMDLayers.h"
//#include "MediaTexture.h"
//#include "ScreenRendering.h"
//#include "ScenePrivate.h"
//#include "PostProcess/SceneFilterRendering.h"


namespace OculusHMD
{

//-------------------------------------------------------------------------------------------------
// FTextureSetProxy
//-------------------------------------------------------------------------------------------------

FTextureSetProxy::FTextureSetProxy(FTextureRHIParamRef InTexture, uint32 InSwapChainLength) :
	RHITexture(InTexture), SwapChainLength(InSwapChainLength), SwapChainIndex_RHIThread(0)
{
}


void FTextureSetProxy::GenerateMips_RenderThread(FRHICommandListImmediate& RHICmdList)
{
	CheckInRenderThread();

	if (RHITexture->GetNumMips() > 1)
	{
#if PLATFORM_WINDOWS
		RHICmdList.GenerateMips(RHITexture);
#endif
	}
}


void FTextureSetProxy::IncrementSwapChainIndex_RHIThread()
{
	CheckInRHIThread();

	SwapChainIndex_RHIThread = (SwapChainIndex_RHIThread + 1) % SwapChainLength;

	AliasResources_RHIThread();
}


} // namespace OculusHMD

#endif //OCULUS_HMD_SUPPORTED_PLATFORMS
