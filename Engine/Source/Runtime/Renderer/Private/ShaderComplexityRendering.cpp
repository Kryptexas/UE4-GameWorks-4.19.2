// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
ShaderComplexityRendering.cpp: Contains definitions for rendering the shader complexity viewmode.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessVisualizeComplexity.h"
#include "SceneUtils.h"
#include "PostProcessing.h"

IMPLEMENT_MATERIAL_SHADER_TYPE(,FQuadOverdrawVS,TEXT("QuadOverdrawVertexShader"),TEXT("Main"),SF_Vertex);	
IMPLEMENT_MATERIAL_SHADER_TYPE(,FQuadOverdrawHS,TEXT("QuadOverdrawVertexShader"),TEXT("MainHull"),SF_Hull);	
IMPLEMENT_MATERIAL_SHADER_TYPE(,FQuadOverdrawDS,TEXT("QuadOverdrawVertexShader"),TEXT("MainDomain"),SF_Domain);

void FShaderComplexityAccumulatePS::SetParameters(
	FRHICommandList& RHICmdList, 
	uint32 NumVertexInstructions, 
	uint32 NumPixelInstructions,
	EQuadOverdrawMode QuadOverdrawMode,
	ERHIFeatureLevel::Type InFeatureLevel)
{
	float NormalizeMul = 1.0f / GetMaxShaderComplexityCount(InFeatureLevel);

	// normalize the complexity so we can fit it in a low precision scene color which is necessary on some platforms
	// late value is for overdraw which can be problmatic with a low precision float format, at some point the precision isn't there any more and it doesn't accumulate
	FVector Value = QuadOverdrawMode == QOM_QuadComplexity ? FVector(NormalizedQuadComplexityValue) : FVector(NumPixelInstructions * NormalizeMul, NumVertexInstructions * NormalizeMul, 1/32.0f);

	// Disable UAVs if something is wrong
	if (QuadOverdrawMode != QOM_None)
	{
		const FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);
		if (QuadBufferUAV.IsBound() && SceneContext.GetQuadOverdrawIndex() != QuadBufferUAV.GetBaseIndex())
		{
			QuadOverdrawMode = QOM_None;
		}
	}

	SetShaderValue(RHICmdList, FGlobalShader::GetPixelShader(), NormalizedComplexity, Value);
	SetShaderValue(RHICmdList, FGlobalShader::GetPixelShader(), ShowQuadOverdraw, QuadOverdrawMode != QOM_None);
}

template <bool bQuadComplexity>
void TComplexityAccumulatePS<bQuadComplexity>::SetParameters(
		TShaderMap<FGlobalShaderType>* ShaderMap,
		FRHICommandList& RHICmdList, 
		uint32 NumVertexInstructions, 
		uint32 NumPixelInstructions,
		EQuadOverdrawMode QuadOverdrawMode,
		ERHIFeatureLevel::Type InFeatureLevel)
{
	TShaderMapRef< TComplexityAccumulatePS<bQuadComplexity> > ShaderRef(ShaderMap);
	ShaderRef->FShaderComplexityAccumulatePS::SetParameters(RHICmdList, NumVertexInstructions, NumPixelInstructions, QuadOverdrawMode, InFeatureLevel);
}

template <bool bQuadComplexity>
FShader* TComplexityAccumulatePS<bQuadComplexity>::GetPixelShader(TShaderMap<FGlobalShaderType>* ShaderMap)
{
	TShaderMapRef< TComplexityAccumulatePS<bQuadComplexity> > ShaderRef(ShaderMap);
	return *ShaderRef;
}

void FShaderComplexityAccumulatePS::SetParameters(
		TShaderMap<FGlobalShaderType>* ShaderMap,
		FRHICommandList& RHICmdList, 
		uint32 NumVertexInstructions, 
		uint32 NumPixelInstructions,
		EQuadOverdrawMode QuadOverdrawMode,
		ERHIFeatureLevel::Type InFeatureLevel)
{
	if (QuadOverdrawMode == QOM_QuadComplexity || QuadOverdrawMode == QOM_ShaderComplexityBleeding)
	{
		TQuadComplexityAccumulatePS::SetParameters(ShaderMap, RHICmdList, NumVertexInstructions, NumPixelInstructions, QuadOverdrawMode, InFeatureLevel);
	}
	else
	{
		TShaderComplexityAccumulatePS::SetParameters(ShaderMap, RHICmdList, NumVertexInstructions, NumPixelInstructions, QuadOverdrawMode, InFeatureLevel);
	}
}

FShader* FShaderComplexityAccumulatePS::GetPixelShader(TShaderMap<FGlobalShaderType>* ShaderMap, EQuadOverdrawMode QuadOverdrawMode)
{
	if (QuadOverdrawMode == QOM_QuadComplexity || QuadOverdrawMode == QOM_ShaderComplexityBleeding)
	{
		return TQuadComplexityAccumulatePS::GetPixelShader(ShaderMap);
	}
	else
	{
		return TShaderComplexityAccumulatePS::GetPixelShader(ShaderMap);
	}
}

IMPLEMENT_SHADER_TYPE(template<>,TShaderComplexityAccumulatePS,TEXT("ShaderComplexityAccumulatePixelShader"),TEXT("Main"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>,TQuadComplexityAccumulatePS,TEXT("QuadComplexityAccumulatePixelShader"),TEXT("Main"),SF_Pixel);

void PatchBoundShaderStateInputForQuadOverdraw(
		FBoundShaderStateInput& BoundShaderStateInput,
		const FMaterial* MaterialResource,
		const FVertexFactory* VertexFactory,
		ERHIFeatureLevel::Type FeatureLevel, 
		EQuadOverdrawMode QuadOverdrawMode
		)
{
	TShaderMap<FGlobalShaderType>* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
	FVertexFactoryType* VertexFactoryType = VertexFactory->GetType();

	if (!MaterialResource->HasVertexPositionOffsetConnected() && MaterialResource->GetTessellationMode() == MTM_NoTessellation)
	{
		MaterialResource = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(false)->GetMaterial(FeatureLevel);
	}

	BoundShaderStateInput.VertexShaderRHI = MaterialResource->GetShader<FQuadOverdrawVS>(VertexFactoryType)->GetVertexShader();

	if (BoundShaderStateInput.HullShaderRHI)
	{
		BoundShaderStateInput.HullShaderRHI = MaterialResource->GetShader<FQuadOverdrawHS>(VertexFactoryType)->GetHullShader();
	}
	if (BoundShaderStateInput.DomainShaderRHI)
	{
		BoundShaderStateInput.DomainShaderRHI = MaterialResource->GetShader<FQuadOverdrawDS>(VertexFactoryType)->GetDomainShader();
	}

	if (QuadOverdrawMode == QOM_QuadComplexity || QuadOverdrawMode == QOM_ShaderComplexityBleeding)
	{
		BoundShaderStateInput.PixelShaderRHI = TQuadComplexityAccumulatePS::GetPixelShader(GlobalShaderMap)->GetPixelShader();
	}
	else
	{
		BoundShaderStateInput.PixelShaderRHI = TShaderComplexityAccumulatePS::GetPixelShader(GlobalShaderMap)->GetPixelShader();
	}
}

void SetNonPSParametersForQuadOverdraw(
	FRHICommandList& RHICmdList, 
	const FMaterialRenderProxy* MaterialRenderProxy, 
	const FMaterial* MaterialResource, 
	const FSceneView& View,
	const FVertexFactory* VertexFactory,
	bool bHasHullAndDomainShader
	)
{
	VertexFactory->Set(RHICmdList);

	FVertexFactoryType* VertexFactoryType = VertexFactory->GetType();
	
	if (!MaterialResource->HasVertexPositionOffsetConnected() && MaterialResource->GetTessellationMode() == MTM_NoTessellation)
	{
		MaterialRenderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(false);
		MaterialResource = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(false)->GetMaterial(View.GetFeatureLevel());
	}

	MaterialResource->GetShader<FQuadOverdrawVS>(VertexFactoryType)->SetParameters(RHICmdList, MaterialRenderProxy, *MaterialResource, View);

	if (bHasHullAndDomainShader)
	{
		MaterialResource->GetShader<FQuadOverdrawHS>(VertexFactoryType)->SetParameters(RHICmdList, MaterialRenderProxy, View);
		MaterialResource->GetShader<FQuadOverdrawDS>(VertexFactoryType)->SetParameters(RHICmdList, MaterialRenderProxy, View);
	}
}

void SetMeshForQuadOverdraw(
	FRHICommandList& RHICmdList, 
	const FMaterial* MaterialResource, 
	const FSceneView& View,
	const FVertexFactory* VertexFactory,
	bool bHasHullAndDomainShader,
	const FPrimitiveSceneProxy* Proxy,
	const FMeshBatchElement& BatchElement, 
	const FMeshDrawingRenderState& DrawRenderState
	)
{
	FVertexFactoryType* VertexFactoryType = VertexFactory->GetType();
	
	if (!MaterialResource->HasVertexPositionOffsetConnected() && MaterialResource->GetTessellationMode() == MTM_NoTessellation)
	{
		MaterialResource = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(false)->GetMaterial(View.GetFeatureLevel());
	}

	MaterialResource->GetShader<FQuadOverdrawVS>(VertexFactoryType)->SetMesh(RHICmdList, VertexFactory, View, Proxy, BatchElement, DrawRenderState);

	if (bHasHullAndDomainShader)
	{
		MaterialResource->GetShader<FQuadOverdrawHS>(VertexFactoryType)->SetMesh(RHICmdList, VertexFactory, View, Proxy, BatchElement, DrawRenderState);
		MaterialResource->GetShader<FQuadOverdrawDS>(VertexFactoryType)->SetMesh(RHICmdList, VertexFactory, View, Proxy, BatchElement, DrawRenderState);
	}
}


