// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
WantedMipsAccuracyRendering.cpp: Contains definitions for rendering the viewmode.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "WantedMipsAccuracyRendering.h"

IMPLEMENT_SHADER_TYPE(,FWantedMipsAccuracyPS,TEXT("WantedMipsAccuracyPixelShader"),TEXT("Main"),SF_Pixel);

void FWantedMipsAccuracyPS::SetParameters(
	FRHICommandList& RHICmdList, 
	const FShader* OriginalVS, 
	const FShader* OriginalPS, 
	const FMaterialRenderProxy* MaterialRenderProxy,
	const FMaterial& Material,
	const FSceneView& View
	)
{
	const int32 NumEngineColors = FMath::Min<int32>(GEngine->StreamingAccuracyColors.Num(), NumStreamingAccuracyColors);
	int32 ColorIndex = 0;
	for (; ColorIndex < NumEngineColors; ++ColorIndex)
	{
		SetShaderValue(RHICmdList, FGlobalShader::GetPixelShader(), AccuracyColorsParameter, GEngine->StreamingAccuracyColors[ColorIndex], ColorIndex);
	}
	for (; ColorIndex < NumStreamingAccuracyColors; ++ColorIndex)
	{
		SetShaderValue(RHICmdList, FGlobalShader::GetPixelShader(), AccuracyColorsParameter, FLinearColor::Black, ColorIndex);
	}
}

void FWantedMipsAccuracyPS::SetMesh(
	FRHICommandList& RHICmdList, 
	const FVertexFactory* VertexFactory,
	const FSceneView& View,
	const FPrimitiveSceneProxy* Proxy,
	int32 VisualizeLODIndex,
	const FMeshBatchElement& BatchElement, 
	const FMeshDrawingRenderState& DrawRenderState
	)
{
	float CPUWantedMips = -1.f;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	const bool bUseNewMetrics = CVarStreamingUseNewMetrics.GetValueOnRenderThread() != 0;
	const FStreamingSectionBuildInfo* SectionData = Proxy ? Proxy->GetStreamingSectionData(bUseNewMetrics ? VisualizeLODIndex : INDEX_NONE, bUseNewMetrics ? BatchElement.VisualizeElementIndex : INDEX_NONE) : nullptr;
	if (SectionData)
	{
		FVector ViewToObject = SectionData->BoxOrigin - View.ViewMatrices.ViewOrigin;

		float DistSqMinusRadiusSq = 0;
		float ViewSize = View.ViewRect.Size().X;

		if (bUseNewMetrics)
		{
			ViewToObject = ViewToObject.GetAbs();
			FVector BoxViewToObject = ViewToObject.ComponentMin(SectionData->BoxExtent);
			DistSqMinusRadiusSq = FVector::DistSquared(BoxViewToObject, ViewToObject);
		}
		else
		{
			float Distance = ViewToObject.Size();
			DistSqMinusRadiusSq = FMath::Square(Distance) - SectionData->BoxExtent.SizeSquared();
		}

		DistSqMinusRadiusSq = FMath::Max(1.f, DistSqMinusRadiusSq);

		float CoordSize = SectionData->TexelFactors[0] * FMath::InvSqrt(DistSqMinusRadiusSq) * ViewSize;

		CPUWantedMips = FMath::Clamp<float>(FMath::Log2(CoordSize), 0.f, (float)MaxStreamingAccuracyMips);
	}
#endif
	// Because the streamer use FMath::FloorToFloat, here we need to use -1 to have a useful result.
	SetShaderValue(RHICmdList, FGlobalShader::GetPixelShader(), CPUWantedMipsParameter, CPUWantedMips - 1.f);
}

void FWantedMipsAccuracyPS::SetMesh(FRHICommandList& RHICmdList, const FSceneView& View)
{
	SetShaderValue(RHICmdList, FGlobalShader::GetPixelShader(), CPUWantedMipsParameter, -1.f);
}
