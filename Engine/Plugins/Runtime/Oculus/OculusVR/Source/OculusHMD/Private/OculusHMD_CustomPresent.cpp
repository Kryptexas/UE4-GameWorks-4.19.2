// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "OculusHMD_CustomPresent.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS
#include "OculusHMD.h"
#include "ScreenRendering.h"
#include "PipelineStateCache.h"
#include "ClearQuad.h"
#include "OculusShaders.h"

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "Android/AndroidEGL.h"
#include "AndroidApplication.h"
#endif

namespace OculusHMD
{

//-------------------------------------------------------------------------------------------------
// FCustomPresent
//-------------------------------------------------------------------------------------------------

FCustomPresent::FCustomPresent(FOculusHMD* InOculusHMD)
	: FRHICustomPresent(nullptr)
	, OculusHMD(InOculusHMD)
{
	// grab a pointer to the renderer module for displaying our mirror window
	static const FName RendererModuleName("Renderer");
	RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);
}


void FCustomPresent::ReleaseResources_RHIThread()
{
	CheckInRHIThread();

	if (MirrorTextureRHI.IsValid())
	{
		ovrp_DestroyMirrorTexture2();
		MirrorTextureRHI = nullptr;
	}
}


void FCustomPresent::Shutdown()
{
	CheckInGameThread();

	// OculusHMD is going away, but this object can live on until viewport is destroyed
	ExecuteOnRenderThread([this]()
	{
		ExecuteOnRHIThread([this]()
		{
			OculusHMD = nullptr;
		});
	});
}

void FCustomPresent::UpdateViewport(FRHIViewport* InViewportRHI)
{
	CheckInGameThread();

	ViewportRHI = InViewportRHI;
	ViewportRHI->SetCustomPresent(this);
}

void FCustomPresent::OnBackBufferResize()
{
	// if we are in the middle of rendering: prevent from calling EndFrame
	ExecuteOnRenderThread_DoNotWait([this]()
	{
		ExecuteOnRHIThread_DoNotWait([this]()
		{
			if (OculusHMD)
			{
				FGameFrame* Frame_RHIThread = OculusHMD->GetFrame_RHIThread();

				if (Frame_RHIThread)
				{
					Frame_RHIThread->ShowFlags.Rendering = 0;
				}
			}
		});
	});
}

bool FCustomPresent::Present(int32& SyncInterval)
{
	CheckInRHIThread();

	bool bHostPresent;

	if (OculusHMD && OculusHMD->GetFrame_RHIThread())
	{
#if PLATFORM_ANDROID
		bHostPresent = false;
#else
		bHostPresent = OculusHMD->IsSpectatorScreenActive();
#endif
		SyncInterval = 0; // VSync off
		FinishRendering_RHIThread();
	}
	else
	{
		bHostPresent = true; // use regular Present; this frame is not ready yet
	}

	return bHostPresent;
}


void FCustomPresent::FinishRendering_RHIThread()
{
	SCOPE_CYCLE_COUNTER(STAT_FinishRendering);
	CheckInRHIThread();

#if STATS
	if (OculusHMD->GetFrame_RHIThread()->ShowFlags.Rendering)
	{
		ovrpAppLatencyTimings AppLatencyTimings;
		if(OVRP_SUCCESS(ovrp_GetAppLatencyTimings2(&AppLatencyTimings)))
		{
			SET_FLOAT_STAT(STAT_LatencyRender, AppLatencyTimings.LatencyRender * 1000.0f);
			SET_FLOAT_STAT(STAT_LatencyTimewarp, AppLatencyTimings.LatencyTimewarp * 1000.0f);
			SET_FLOAT_STAT(STAT_LatencyPostPresent, AppLatencyTimings.LatencyPostPresent * 1000.0f);
			SET_FLOAT_STAT(STAT_ErrorRender, AppLatencyTimings.ErrorRender * 1000.0f);
			SET_FLOAT_STAT(STAT_ErrorTimewarp, AppLatencyTimings.ErrorTimewarp * 1000.0f);
		}
	}
#endif

	OculusHMD->FinishRHIFrame_RHIThread();
}


EPixelFormat FCustomPresent::GetPixelFormat(EPixelFormat Format) const
{
	switch (Format)
	{
	case PF_B8G8R8A8:
	case PF_FloatRGBA:
	case PF_FloatR11G11B10:
	case PF_R8G8B8A8:
		return Format;
	}

	return GetDefaultPixelFormat();
}


EPixelFormat FCustomPresent::GetPixelFormat(ovrpTextureFormat Format) const
{
	switch(Format)
	{
		case ovrpTextureFormat_R8G8B8A8_sRGB:
		case ovrpTextureFormat_R8G8B8A8:
			return PF_R8G8B8A8;
		case ovrpTextureFormat_R16G16B16A16_FP:
			return PF_FloatRGBA;
		case ovrpTextureFormat_R11G11B10_FP:
			return PF_FloatR11G11B10;
		case ovrpTextureFormat_B8G8R8A8_sRGB:
		case ovrpTextureFormat_B8G8R8A8:
			return PF_B8G8R8A8;
	}

	return GetDefaultPixelFormat();
}


ovrpTextureFormat FCustomPresent::GetOvrpTextureFormat(EPixelFormat Format, bool bSRGB) const
{
	switch (Format)
	{
	case PF_B8G8R8A8:
		return bSRGB ? ovrpTextureFormat_B8G8R8A8_sRGB : ovrpTextureFormat_B8G8R8A8;
	case PF_FloatRGBA:
		return ovrpTextureFormat_R16G16B16A16_FP;
	case PF_FloatR11G11B10:
		return ovrpTextureFormat_R11G11B10_FP;
	case PF_R8G8B8A8:
		return bSRGB ? ovrpTextureFormat_R8G8B8A8_sRGB : ovrpTextureFormat_R8G8B8A8;
	}

	return GetOvrpTextureFormat(GetDefaultPixelFormat(), bSRGB);
}


void FCustomPresent::CopyTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRHIParamRef DstTexture, FTextureRHIParamRef SrcTexture,
	FIntRect DstRect, FIntRect SrcRect, bool bAlphaPremultiply, bool bNoAlphaWrite) const
{
	CheckInRenderThread();

	FTexture2DRHIParamRef DstTexture2D = DstTexture->GetTexture2D();
	FTextureCubeRHIParamRef DstTextureCube = DstTexture->GetTextureCube();
	FTexture2DRHIParamRef SrcTexture2D = SrcTexture->GetTexture2D();
	FTextureCubeRHIParamRef SrcTextureCube = SrcTexture->GetTextureCube();

	FIntPoint DstSize;
	FIntPoint SrcSize;

	if (DstTexture2D && SrcTexture2D)
	{
		DstSize = FIntPoint(DstTexture2D->GetSizeX(), DstTexture2D->GetSizeY());
		SrcSize = FIntPoint(SrcTexture2D->GetSizeX(), SrcTexture2D->GetSizeY());
	}
	else if(DstTextureCube && SrcTextureCube)
	{
		DstSize = FIntPoint(DstTextureCube->GetSize(), DstTextureCube->GetSize());
		SrcSize = FIntPoint(SrcTextureCube->GetSize(), SrcTextureCube->GetSize());
	}
	else
	{
		return;
	}

	if (DstRect.IsEmpty())
	{
		DstRect = FIntRect(FIntPoint::ZeroValue, DstSize);
	}

	if (SrcRect.IsEmpty())
	{
		SrcRect = FIntRect(FIntPoint::ZeroValue, SrcSize);
	}

	const uint32 ViewportWidth = DstRect.Width();
	const uint32 ViewportHeight = DstRect.Height();
	const FIntPoint TargetSize(ViewportWidth, ViewportHeight);
	float U = SrcRect.Min.X / (float) SrcSize.X;
	float V = SrcRect.Min.Y / (float) SrcSize.Y;
	float USize = SrcRect.Width() / (float) SrcSize.X;
	float VSize = SrcRect.Height() / (float) SrcSize.Y;

#if PLATFORM_ANDROID // on android, top-left isn't 0/0 but 1/0.
	V = 1.0f - V;
	VSize = -VSize;
#endif

	FRHITexture* SrcTextureRHI = SrcTexture;
	RHICmdList.TransitionResources(EResourceTransitionAccess::EReadable, &SrcTextureRHI, 1);
	FGraphicsPipelineStateInitializer GraphicsPSOInit;

	if (bAlphaPremultiply)
	{
		if (bNoAlphaWrite)
		{
			// for quads, write RGB, RGB = src.rgb * 1 + dst.rgb * 0
			GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGB, BO_Add, BF_One, BF_Zero, BO_Add, BF_One, BF_Zero>::GetRHI();
		}
		else
		{
			// for quads, write RGBA, RGB = src.rgb * src.a + dst.rgb * 0, A = src.a + dst.a * 0
			GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_Zero, BO_Add, BF_One, BF_Zero>::GetRHI();
		}
	}
	else
	{
		if (bNoAlphaWrite)
		{
			GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGB>::GetRHI();
		}
		else
		{
			// for mirror window
			GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		}
	}

	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	const auto FeatureLevel = GMaxRHIFeatureLevel;
	auto ShaderMap = GetGlobalShaderMap(FeatureLevel);
	TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = RendererModule->GetFilterVertexDeclaration().VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);

	if (DstTexture2D)
	{
		SetRenderTarget(RHICmdList, DstTexture, FTextureRHIRef());

		if (bNoAlphaWrite)
		{
			DrawClearQuad(RHICmdList, bAlphaPremultiply ? FLinearColor::Black : FLinearColor::White);
		}

		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		
		TShaderMapRef<FScreenPS> PixelShader(ShaderMap);
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
		FSamplerStateRHIParamRef SamplerState = DstRect.Size() == SrcRect.Size() ? TStaticSamplerState<SF_Point>::GetRHI() : TStaticSamplerState<SF_Bilinear>::GetRHI();
		PixelShader->SetParameters(RHICmdList, SamplerState, SrcTextureRHI);

		RHICmdList.SetViewport(DstRect.Min.X, DstRect.Min.Y, 0.0f, DstRect.Max.X, DstRect.Max.Y, 1.0f);

		RendererModule->DrawRectangle(
			RHICmdList,
			0, 0, ViewportWidth, ViewportHeight,
			U, V, USize, VSize,
			TargetSize,
			FIntPoint(1, 1),
			*VertexShader,
			EDRF_Default);
	}
	else
	{
		for (int FaceIndex = 0; FaceIndex < 6; FaceIndex++)
		{
			SetRenderTarget(RHICmdList, DstTexture, 0, FaceIndex, FTextureRHIRef());

			if (bNoAlphaWrite)
			{
				DrawClearQuad(RHICmdList, bAlphaPremultiply ? FLinearColor::Black : FLinearColor::White);
			}

			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
			
			TShaderMapRef<FOculusCubemapPS> PixelShader(ShaderMap);
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
			FSamplerStateRHIParamRef SamplerState = DstRect.Size() == SrcRect.Size() ? TStaticSamplerState<SF_Point>::GetRHI() : TStaticSamplerState<SF_Bilinear>::GetRHI();
			PixelShader->SetParameters(RHICmdList, SamplerState, SrcTextureRHI, FaceIndex);
			
			RHICmdList.SetViewport(DstRect.Min.X, DstRect.Min.Y, 0.0f, DstRect.Max.X, DstRect.Max.Y, 1.0f);
			
			RendererModule->DrawRectangle(
				RHICmdList,
				0, 0, ViewportWidth, ViewportHeight,
				U, V, USize, VSize,
				TargetSize,
				FIntPoint(1, 1),
				*VertexShader,
				EDRF_Default);
		}
	}
}

} // namespace OculusHMD

#endif //OCULUS_HMD_SUPPORTED_PLATFORMS
