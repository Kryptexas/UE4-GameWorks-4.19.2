// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
WantedMipsAccuracyRendering.cpp: Contains definitions for rendering the viewmode.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"

static TAutoConsoleVariable<int32> CVarStreamingUseAABBRenderer(
	TEXT("r.Streaming.UseAABB"),
	0,
	TEXT("If non-zero, will use AABB to compute distance."),
	ECVF_Default);


IMPLEMENT_SHADER_TYPE(,FWantedMipsAccuracyPS,TEXT("WantedMipsAccuracyPixelShader"),TEXT("Main"),SF_Pixel);

void FWantedMipsAccuracyPS::SetParameters(
	FRHICommandList& RHICmdList, 
	const FShader* OriginalVS, 
	const FShader* OriginalPS, 
	const FSceneView& View
	)
{
	const int32 NumEngineColors = FMath::Min<int32>(GEngine->StreamingAccuracyColors.Num(), NumStreamingAccuracyColors);
	int32 ColorIndex = 0;
	for (; ColorIndex < NumEngineColors; ++ColorIndex)
	{
		SetShaderValue(RHICmdList, FGlobalShader::GetPixelShader(), StreamingAccuracyColorsParameter, GEngine->StreamingAccuracyColors[ColorIndex], ColorIndex);
	}
	for (; ColorIndex < NumStreamingAccuracyColors; ++ColorIndex)
	{
		SetShaderValue(RHICmdList, FGlobalShader::GetPixelShader(), StreamingAccuracyColorsParameter, FLinearColor::Black, ColorIndex);
	}
}

void FWantedMipsAccuracyPS::SetMesh(
	FRHICommandList& RHICmdList, 
	const FVertexFactory* VertexFactory,
	const FSceneView& View,
	const FPrimitiveSceneProxy* Proxy,
	const FMeshBatchElement& BatchElement, 
	const FMeshDrawingRenderState& DrawRenderState
	)
{
	// This is taken from FStreamingHandlerTextureStatic::GetWantedMips

	float CPUWantedMips = -1.f;
	if (Proxy && Proxy->GetWorldTexelFactor() > 0)
	{
		FVector ViewToObject = Proxy->GetBounds().Origin - View.ViewMatrices.ViewOrigin;

		float DistSqMinusRadiusSq = 0;
		float ViewSize = View.ViewRect.Size().X;

		if (CVarStreamingUseAABBRenderer.GetValueOnRenderThread())
		{
			ViewToObject = ViewToObject.GetAbs();
			FVector BoxViewToObject = ViewToObject.ComponentMin(Proxy->GetBounds().BoxExtent);

			DistSqMinusRadiusSq = FVector::DistSquared(BoxViewToObject, ViewToObject);
		}
		else
		{
			float Distance = ViewToObject.Size();
			DistSqMinusRadiusSq = FMath::Square(Distance) - FMath::Square(Proxy->GetBounds().SphereRadius);
		}

		DistSqMinusRadiusSq = FMath::Max(1.f, DistSqMinusRadiusSq);

		float CoordSize = Proxy->GetWorldTexelFactor() * FMath::InvSqrt(DistSqMinusRadiusSq) * ViewSize;

		CPUWantedMips = FMath::Clamp<float>(FMath::Log2(CoordSize), 0.f, (float)MaxStreamingAccuracyMips);
	}

	SetShaderValue(RHICmdList, FGlobalShader::GetPixelShader(), CPUWantedMipsParameter, FMath::FloorToFloat(CPUWantedMips));
}

void FWantedMipsAccuracyPS::SetMesh(FRHICommandList& RHICmdList, const FSceneView& View)
{
	SetShaderValue(RHICmdList, FGlobalShader::GetPixelShader(), CPUWantedMipsParameter, -1.f);
}
