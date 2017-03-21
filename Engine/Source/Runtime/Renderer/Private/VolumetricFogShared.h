// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VolumetricFogShared.h
=============================================================================*/

#pragma once

#include "RHIDefinitions.h"
#include "SceneView.h"
#include "SceneRendering.h"

extern FVector VolumetricFogTemporalRandom(uint32 FrameNumber);

/**  */
class FVolumetricFogIntegrationParameters
{
public:

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		
	}

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		VBufferA.Bind(ParameterMap, TEXT("VBufferA"));
		VBufferB.Bind(ParameterMap, TEXT("VBufferB"));
		LightScattering.Bind(ParameterMap, TEXT("LightScattering"));
		IntegratedLightScattering.Bind(ParameterMap, TEXT("IntegratedLightScattering"));
		IntegratedLightScatteringSampler.Bind(ParameterMap, TEXT("IntegratedLightScatteringSampler"));
		VolumetricFogData.Bind(ParameterMap, TEXT("VolumetricFog"));
		UnjitteredClipToTranslatedWorld.Bind(ParameterMap, TEXT("UnjitteredClipToTranslatedWorld")); 
		UnjitteredPrevWorldToClip.Bind(ParameterMap, TEXT("UnjitteredPrevWorldToClip")); 
		FrameJitterOffset.Bind(ParameterMap, TEXT("FrameJitterOffset")); 
	}

	template<typename ShaderRHIParamRef>
	void Set(
		FRHICommandList& RHICmdList, 
		const ShaderRHIParamRef& ShaderRHI, 
		const FViewInfo& View, 
		IPooledRenderTarget* VBufferARenderTarget, 
		IPooledRenderTarget* VBufferBRenderTarget, 
		IPooledRenderTarget* LightScatteringRenderTarget) const
	{
		if (VBufferA.IsBound())
		{
			const FSceneRenderTargetItem& VBufferAItem = VBufferARenderTarget->GetRenderTargetItem();
			VBufferA.SetTexture(RHICmdList, ShaderRHI, VBufferAItem.ShaderResourceTexture, VBufferAItem.UAV);
		}

		if (VBufferB.IsBound())
		{
			const FSceneRenderTargetItem& VBufferBItem = VBufferBRenderTarget->GetRenderTargetItem();
			VBufferB.SetTexture(RHICmdList, ShaderRHI, VBufferBItem.ShaderResourceTexture, VBufferBItem.UAV);
		}

		if (LightScattering.IsBound())
		{
			const FSceneRenderTargetItem& LightScatteringItem = LightScatteringRenderTarget->GetRenderTargetItem();
			LightScattering.SetTexture(RHICmdList, ShaderRHI, LightScatteringItem.ShaderResourceTexture, LightScatteringItem.UAV);
		}

		if (IntegratedLightScattering.IsBound())
		{
			const FSceneRenderTargetItem& IntegratedLightScatteringItem = View.VolumetricFogResources.IntegratedLightScattering->GetRenderTargetItem();

			IntegratedLightScattering.SetTexture(RHICmdList, ShaderRHI, IntegratedLightScatteringItem.ShaderResourceTexture, IntegratedLightScatteringItem.UAV);
		}

		SetSamplerParameter(RHICmdList, ShaderRHI, IntegratedLightScatteringSampler, TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());
		
		if (VolumetricFogData.IsBound())
		{
			SetUniformBufferParameter(RHICmdList, ShaderRHI, VolumetricFogData, View.VolumetricFogResources.VolumetricFogGlobalData);
		}

		if (UnjitteredClipToTranslatedWorld.IsBound())
		{
			FMatrix UnjitteredInvTranslatedViewProjectionMatrix = View.ViewMatrices.ComputeInvProjectionNoAAMatrix() * View.ViewMatrices.GetTranslatedViewMatrix().GetTransposed();
			SetShaderValue(RHICmdList, ShaderRHI, UnjitteredClipToTranslatedWorld, UnjitteredInvTranslatedViewProjectionMatrix);
		}

		if (UnjitteredPrevWorldToClip.IsBound())
		{
			FMatrix UnjitteredViewProjectionMatrix = View.PrevViewMatrices.GetViewMatrix() * View.PrevViewMatrices.ComputeProjectionNoAAMatrix();
			SetShaderValue(RHICmdList, ShaderRHI, UnjitteredPrevWorldToClip, UnjitteredViewProjectionMatrix);
		}

		if (FrameJitterOffset.IsBound())
		{
			const FVector FrameJitterOffsetValue = VolumetricFogTemporalRandom(View.Family->FrameNumber);
			SetShaderValue(RHICmdList, ShaderRHI, FrameJitterOffset, FrameJitterOffsetValue);
		}
	}

	template<typename ShaderRHIParamRef>
	void UnsetParameters(
		FRHICommandList& RHICmdList, 
		const ShaderRHIParamRef& ShaderRHI, 
		const FViewInfo& View, 
		IPooledRenderTarget* VBufferARenderTarget, 
		IPooledRenderTarget* VBufferBRenderTarget, 
		IPooledRenderTarget* LightScatteringRenderTarget, 
		bool bTransitionToReadable)
	{
		VBufferA.UnsetUAV(RHICmdList, ShaderRHI);
		VBufferB.UnsetUAV(RHICmdList, ShaderRHI);
		LightScattering.UnsetUAV(RHICmdList, ShaderRHI);
		IntegratedLightScattering.UnsetUAV(RHICmdList, ShaderRHI);

		const FSceneRenderTargetItem& IntegratedLightScatteringItem = View.VolumetricFogResources.IntegratedLightScattering->GetRenderTargetItem();

		TArray<FUnorderedAccessViewRHIParamRef, TInlineAllocator<4>> OutUAVs;

		if (VBufferA.IsUAVBound())
		{
			OutUAVs.Add(VBufferARenderTarget->GetRenderTargetItem().UAV);
		}

		if (VBufferB.IsUAVBound())
		{
			OutUAVs.Add(VBufferBRenderTarget->GetRenderTargetItem().UAV);
		}

		if (LightScattering.IsUAVBound())
		{
			OutUAVs.Add(LightScatteringRenderTarget->GetRenderTargetItem().UAV);
		}

		if (IntegratedLightScattering.IsUAVBound())
		{
			OutUAVs.Add(IntegratedLightScatteringItem.UAV);
		}

		if (OutUAVs.Num() > 0 && bTransitionToReadable)
		{
			RHICmdList.TransitionResources(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToGfx, OutUAVs.GetData(), OutUAVs.Num());
		}
	}

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,FVolumetricFogIntegrationParameters& P)
	{
		Ar << P.VBufferA;
		Ar << P.VBufferB;
		Ar << P.LightScattering;
		Ar << P.IntegratedLightScattering;
		Ar << P.IntegratedLightScatteringSampler;
		Ar << P.VolumetricFogData;
		Ar << P.UnjitteredClipToTranslatedWorld;
		Ar << P.UnjitteredPrevWorldToClip;
		Ar << P.FrameJitterOffset;
		return Ar;
	}

private:

	FRWShaderParameter VBufferA;
	FRWShaderParameter VBufferB;
	FRWShaderParameter LightScattering;
	FRWShaderParameter IntegratedLightScattering;
	FShaderResourceParameter IntegratedLightScatteringSampler;
	FShaderUniformBufferParameter VolumetricFogData;
	FShaderParameter UnjitteredClipToTranslatedWorld;
	FShaderParameter UnjitteredPrevWorldToClip;
	FShaderParameter FrameJitterOffset;
};

inline int32 ComputeZSliceFromDepth(float SceneDepth, FVector GridZParams)
{
	return FMath::TruncToInt(FMath::Log2(SceneDepth * GridZParams.X + GridZParams.Y) * GridZParams.Z);
}

extern FVector GetVolumetricFogGridZParams(float NearPlane, float FarPlane, int32 GridSizeZ);
