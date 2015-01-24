// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	BasePassRendering.inl: Base pass rendering implementations.
		(Due to forward declaration issues)
=============================================================================*/

#pragma once

template<typename LightMapPolicyType>
inline void TBasePassVertexShaderBaseType<LightMapPolicyType>::SetMesh(FRHICommandList& RHICmdList, const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy, const FMeshBatch& Mesh, const FMeshBatchElement& BatchElement)
{
	FVertexShaderRHIParamRef VertexShaderRHI = GetVertexShader();
	FMeshMaterialShader::SetMesh(RHICmdList, VertexShaderRHI, VertexFactory, View, Proxy, BatchElement);

	if (PreviousLocalToWorldParameter.IsBound() && Proxy)
	{
		check(SkipOutputVelocityParameter.IsBound());

		FMatrix PreviousLocalToWorld;
		bool bHasPreviousLocalToWorld = false;
		const auto& ViewInfo = (const FViewInfo&)View;
		if (FVelocityDrawingPolicy::HasVelocityOnBasePass(ViewInfo, Proxy, Proxy->GetPrimitiveSceneInfo(), Mesh,
			bHasPreviousLocalToWorld, PreviousLocalToWorld))
		{
			const FMatrix& Transform = bHasPreviousLocalToWorld ? PreviousLocalToWorld : Proxy->GetLocalToWorld();
			SetShaderValue(RHICmdList, VertexShaderRHI, PreviousLocalToWorldParameter, Transform.ConcatTranslation(ViewInfo.PrevViewMatrices.PreViewTranslation));

			SetShaderValue(RHICmdList, VertexShaderRHI, SkipOutputVelocityParameter, 0.0f);
		}
		else
		{
			SetShaderValue(RHICmdList, VertexShaderRHI, SkipOutputVelocityParameter, 1.0f);
		}
	}
}

template<typename LightMapPolicyType>
void TBasePassPixelShaderBaseType<LightMapPolicyType>::SetMesh(FRHICommandList& RHICmdList, const FVertexFactory* VertexFactory, const FSceneView& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatchElement& BatchElement, EBlendMode BlendMode)
{
	if (View.GetFeatureLevel() >= ERHIFeatureLevel::SM4
		&& IsTranslucentBlendMode(BlendMode))
	{
		TranslucentLightingParameters.SetMesh(RHICmdList, this, Proxy, View.GetFeatureLevel());
	}

	FMeshMaterialShader::SetMesh(RHICmdList, GetPixelShader(), VertexFactory, View, Proxy, BatchElement);
}
