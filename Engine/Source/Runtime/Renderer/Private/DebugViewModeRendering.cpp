// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
DebugViewModeRendering.cpp: Contains definitions for rendering debug viewmodes.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "SceneUtils.h"
#include "PostProcessing.h"

IMPLEMENT_MATERIAL_SHADER_TYPE(,FDebugViewModeVS,TEXT("DebugViewModeVertexShader"),TEXT("Main"),SF_Vertex);	
IMPLEMENT_MATERIAL_SHADER_TYPE(,FDebugViewModeHS,TEXT("DebugViewModeVertexShader"),TEXT("MainHull"),SF_Hull);	
IMPLEMENT_MATERIAL_SHADER_TYPE(,FDebugViewModeDS,TEXT("DebugviewModeVertexShader"),TEXT("MainDomain"),SF_Domain);


void FDebugViewMode::GetDebugMaterial(const FMaterialRenderProxy** MaterialRenderProxy, const FMaterial** Material, ERHIFeatureLevel::Type FeatureLevel)
{
	if (!(*Material)->HasVertexPositionOffsetConnected() && (*Material)->GetTessellationMode() == MTM_NoTessellation)
	{
		if (MaterialRenderProxy)
		{
			*MaterialRenderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(false);
		}
		*Material = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(false)->GetMaterial(FeatureLevel);
	}
}

FDebugViewModePS* FDebugViewMode::GetPixelShader(TShaderMap<FGlobalShaderType>* ShaderMap, EDebugViewShaderMode DebugViewShaderMode)
{
	switch (DebugViewShaderMode)
	{
	case DVSM_QuadComplexity:
	case DVSM_ShaderComplexityBleedingQuadOverhead:
		return *TShaderMapRef<TQuadComplexityAccumulatePS>(ShaderMap);
	case DVSM_ShaderComplexity:
	case DVSM_ShaderComplexityContainedQuadOverhead:
		return *TShaderMapRef<TShaderComplexityAccumulatePS>(ShaderMap);
	case DVSM_WantedMipsAccuracy:
		return *TShaderMapRef<FWantedMipsAccuracyPS>(ShaderMap);
	case DVSM_TexelFactorAccuracy:
		return *TShaderMapRef<FTexelFactorAccuracyPS>(ShaderMap);
	default:
		check(false); // Going here will probably generate a null access
		return nullptr;
	}
}

void FDebugViewMode::PatchBoundShaderState(
	FBoundShaderStateInput& BoundShaderStateInput,
	const FMaterial* Material,
	const FVertexFactory* VertexFactory,
	ERHIFeatureLevel::Type FeatureLevel, 
	EDebugViewShaderMode DebugViewShaderMode
	)
{
	GetDebugMaterial(nullptr, &Material, FeatureLevel);

	if (!Material->HasVertexPositionOffsetConnected() && Material->GetTessellationMode() == MTM_NoTessellation)
	{
		Material = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(false)->GetMaterial(FeatureLevel);
	}

	FVertexFactoryType* VertexFactoryType = VertexFactory->GetType();

	BoundShaderStateInput.VertexShaderRHI = Material->GetShader<FDebugViewModeVS>(VertexFactoryType)->GetVertexShader();

	if (BoundShaderStateInput.HullShaderRHI)
	{
		BoundShaderStateInput.HullShaderRHI = Material->GetShader<FDebugViewModeHS>(VertexFactoryType)->GetHullShader();
	}
	if (BoundShaderStateInput.DomainShaderRHI)
	{
		BoundShaderStateInput.DomainShaderRHI = Material->GetShader<FDebugViewModeDS>(VertexFactoryType)->GetDomainShader();
	}

	BoundShaderStateInput.PixelShaderRHI = GetPixelShader(GetGlobalShaderMap(FeatureLevel), DebugViewShaderMode)->GetPixelShader();
}

void FDebugViewMode::SetParametersVSHSDS(
	FRHICommandList& RHICmdList, 
	const FMaterialRenderProxy* MaterialRenderProxy, 
	const FMaterial* Material, 
	const FSceneView& View,
	const FVertexFactory* VertexFactory,
	bool bHasHullAndDomainShader
	)
{
	VertexFactory->Set(RHICmdList);

	GetDebugMaterial(&MaterialRenderProxy, &Material, View.GetFeatureLevel());

	FVertexFactoryType* VertexFactoryType = VertexFactory->GetType();

	Material->GetShader<FDebugViewModeVS>(VertexFactoryType)->SetParameters(RHICmdList, MaterialRenderProxy, *Material, View);

	if (bHasHullAndDomainShader)
	{
		Material->GetShader<FDebugViewModeHS>(VertexFactoryType)->SetParameters(RHICmdList, MaterialRenderProxy, View);
		Material->GetShader<FDebugViewModeDS>(VertexFactoryType)->SetParameters(RHICmdList, MaterialRenderProxy, View);
	}
}

void FDebugViewMode::SetMeshVSHSDS(
	FRHICommandList& RHICmdList, 
	const FVertexFactory* VertexFactory,
	const FSceneView& View,
	const FPrimitiveSceneProxy* Proxy,
	const FMeshBatchElement& BatchElement, 
	const FMeshDrawingRenderState& DrawRenderState,
	const FMaterial* Material, 
	bool bHasHullAndDomainShader
	)
{
	GetDebugMaterial(nullptr, &Material, View.GetFeatureLevel());

	FVertexFactoryType* VertexFactoryType = VertexFactory->GetType();

	Material->GetShader<FDebugViewModeVS>(VertexFactoryType)->SetMesh(RHICmdList, VertexFactory, View, Proxy, BatchElement, DrawRenderState);

	if (bHasHullAndDomainShader)
	{
		Material->GetShader<FDebugViewModeHS>(VertexFactoryType)->SetMesh(RHICmdList, VertexFactory, View, Proxy, BatchElement, DrawRenderState);
		Material->GetShader<FDebugViewModeDS>(VertexFactoryType)->SetMesh(RHICmdList, VertexFactory, View, Proxy, BatchElement, DrawRenderState);
	}
}
