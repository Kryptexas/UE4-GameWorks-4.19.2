// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DistanceFieldShadowing.cpp
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "UniformBuffer.h"
#include "ShaderParameters.h"
#include "PostProcessing.h"
#include "SceneFilterRendering.h"
#include "ScreenRendering.h"
#include "DistanceFieldSurfaceCacheLighting.h"
#include "DistanceFieldLightingShared.h"
#include "RHICommandList.h"
#include "SceneUtils.h"
#include "DistanceFieldAtlas.h"

int32 GDistanceFieldShadowing = 1;
FAutoConsoleVariableRef CVarDistanceFieldShadowing(
	TEXT("r.DistanceFieldShadowing"),
	GDistanceFieldShadowing,
	TEXT("Whether the distance field shadowing feature is allowed."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

//@todo - per light, breaks with multiple directional lights
TGlobalResource<FDistanceFieldObjectBuffers> GDirectionalLightObjectBuffers;

class FDistanceFieldShadowingCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDistanceFieldShadowingCS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldShadowing(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), GDistanceFieldAOTileSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), GDistanceFieldAOTileSizeY);
	}

	/** Default constructor. */
	FDistanceFieldShadowingCS() {}

	/** Initialization constructor. */
	FDistanceFieldShadowingCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		ShadowFactors.Bind(Initializer.ParameterMap, TEXT("ShadowFactors"));
		NumGroups.Bind(Initializer.ParameterMap, TEXT("NumGroups"));
		LightDirection.Bind(Initializer.ParameterMap, TEXT("LightDirection"));
		ObjectParameters.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
	}

	void SetParameters(
		FRHICommandList& RHICmdList, 
		const FSceneView& View, 
		const FProjectedShadowInfo* ProjectedShadowInfo,
		FSceneRenderTargetItem& ShadowFactorsValue, 
		int32 NumObjects, 
		FVector2D NumGroupsValue)
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);

		ShadowFactors.SetTexture(RHICmdList, ShaderRHI, ShadowFactorsValue.ShaderResourceTexture, ShadowFactorsValue.UAV);

		ObjectParameters.Set(RHICmdList, ShaderRHI, GDirectionalLightObjectBuffers, NumObjects);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);

		SetShaderValue(RHICmdList, ShaderRHI, NumGroups, NumGroupsValue);
		SetShaderValue(RHICmdList, ShaderRHI, LightDirection, ProjectedShadowInfo->LightSceneInfo->Proxy->GetDirection());
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		ShadowFactors.UnsetUAV(RHICmdList, GetComputeShader());
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << ShadowFactors;
		Ar << NumGroups;
		Ar << LightDirection;
		Ar << ObjectParameters;
		Ar << DeferredParameters;
		return bShaderHasOutdatedParameters;
	}

private:

	FRWShaderParameter ShadowFactors;
	FShaderParameter NumGroups;
	FShaderParameter LightDirection;
	FDistanceFieldObjectBufferParameters ObjectParameters;
	FDeferredPixelShaderParameters DeferredParameters;
};

IMPLEMENT_SHADER_TYPE(,FDistanceFieldShadowingCS,TEXT("DistanceFieldShadowing"),TEXT("DistanceFieldShadowingCS"),SF_Compute);

class FDistanceFieldShadowingUpsamplePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDistanceFieldShadowingUpsamplePS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldShadowing(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);
	}

	/** Default constructor. */
	FDistanceFieldShadowingUpsamplePS() {}

	/** Initialization constructor. */
	FDistanceFieldShadowingUpsamplePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		ShadowFactorsTexture.Bind(Initializer.ParameterMap,TEXT("ShadowFactorsTexture"));
		ShadowFactorsSampler.Bind(Initializer.ParameterMap,TEXT("ShadowFactorsSampler"));
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, const FProjectedShadowInfo* ShadowInfo, TRefCountPtr<IPooledRenderTarget>& ShadowFactorsTextureValue)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);

		SetTextureParameter(RHICmdList, ShaderRHI, ShadowFactorsTexture, ShadowFactorsSampler, TStaticSamplerState<SF_Bilinear>::GetRHI(), ShadowFactorsTextureValue->GetRenderTargetItem().ShaderResourceTexture);
	}
	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << ShadowFactorsTexture;
		Ar << ShadowFactorsSampler;
		return bShaderHasOutdatedParameters;
	}

private:

	FDeferredPixelShaderParameters DeferredParameters;
	FShaderResourceParameter ShadowFactorsTexture;
	FShaderResourceParameter ShadowFactorsSampler;
};

IMPLEMENT_SHADER_TYPE(,FDistanceFieldShadowingUpsamplePS,TEXT("DistanceFieldShadowing"),TEXT("DistanceFieldShadowingUpsamplePS"),SF_Pixel);

int32 FProjectedShadowInfo::UpdateShadowCastingObjectBuffers() const
{
	int32 NumObjects = 0;

	//@todo - scene rendering allocator
	TArray<FVector4> ObjectBoundsData;
	TArray<FVector4> ObjectData;
	TArray<FVector4> ObjectData2;

	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_GatherObjectData);

		const int32 NumTexelsOneDimX = GDistanceFieldVolumeTextureAtlas.GetSizeX();
		const int32 NumTexelsOneDimY = GDistanceFieldVolumeTextureAtlas.GetSizeY();
		const int32 NumTexelsOneDimZ = GDistanceFieldVolumeTextureAtlas.GetSizeZ();
		const FVector InvTextureDim(1.0f / NumTexelsOneDimX, 1.0f / NumTexelsOneDimY, 1.0f / NumTexelsOneDimZ);

		ObjectBoundsData.Empty(SubjectPrimitives.Num());
		ObjectData.Empty(FDistanceFieldObjectBuffers::ObjectDataStride * SubjectPrimitives.Num());
		ObjectData2.Empty(FDistanceFieldObjectBuffers::ObjectData2Stride * SubjectPrimitives.Num());

		for (int32 PrimitiveIndex = 0; PrimitiveIndex < SubjectPrimitives.Num() && NumObjects < MAX_uint16; PrimitiveIndex++)
		{
			const FPrimitiveSceneInfo* PrimitiveSceneInfo = SubjectPrimitives[PrimitiveIndex];

			FBox LocalVolumeBounds;
			FIntVector BlockMin;
			FIntVector BlockSize;
			bool bBuiltAsIfTwoSided;
			PrimitiveSceneInfo->Proxy->GetDistancefieldAtlasData(LocalVolumeBounds, BlockMin, BlockSize, bBuiltAsIfTwoSided);

			if (BlockMin.X >= 0 && BlockMin.Y >= 0 && BlockMin.Z >= 0)
			{
				const FMatrix LocalToWorld = PrimitiveSceneInfo->Proxy->GetLocalToWorld();

				const FMatrix VolumeToWorld = FScaleMatrix(LocalVolumeBounds.GetExtent()) 
					* FTranslationMatrix(LocalVolumeBounds.GetCenter())
					* LocalToWorld;

				const FVector4 ObjectBoundingSphere(VolumeToWorld.GetOrigin(), VolumeToWorld.GetScaleVector().Size());

				ObjectBoundsData.Add(ObjectBoundingSphere);

				const float MaxExtent = LocalVolumeBounds.GetExtent().GetMax();

				const FMatrix UniformScaleVolumeToWorld = FScaleMatrix(MaxExtent) 
					* FTranslationMatrix(LocalVolumeBounds.GetCenter())
					* LocalToWorld;

				const FVector InvBlockSize(1.0f / BlockSize.X, 1.0f / BlockSize.Y, 1.0f / BlockSize.Z);

				//float3 VolumeUV = (VolumePosition / LocalPositionExtent * .5f * UVScale + .5f * UVScale + UVAdd;
				const FVector LocalPositionExtent = LocalVolumeBounds.GetExtent() / FVector(MaxExtent);
				const FVector UVScale = FVector(BlockSize) * InvTextureDim;
				const float VolumeScale = UniformScaleVolumeToWorld.GetMaximumAxisScale();
				ObjectData2.Add(*(FVector4*)&UniformScaleVolumeToWorld.M[0]);
				ObjectData2.Add(*(FVector4*)&UniformScaleVolumeToWorld.M[1]);
				ObjectData2.Add(*(FVector4*)&UniformScaleVolumeToWorld.M[2]);
				ObjectData2.Add(*(FVector4*)&UniformScaleVolumeToWorld.M[3]);

				const FMatrix WorldToVolume = UniformScaleVolumeToWorld.InverseFast();
				// WorldToVolume
				ObjectData.Add(*(FVector4*)&WorldToVolume.M[0]);
				ObjectData.Add(*(FVector4*)&WorldToVolume.M[1]);
				ObjectData.Add(*(FVector4*)&WorldToVolume.M[2]);
				ObjectData.Add(*(FVector4*)&WorldToVolume.M[3]);

				// Clamp to texel center by subtracting a half texel in the [-1,1] position space
				// LocalPositionExtent
				ObjectData.Add(FVector4(LocalPositionExtent - InvBlockSize, bBuiltAsIfTwoSided ? 1.0f : 0.0f));

				// UVScale
				ObjectData.Add(FVector4(FVector(BlockSize) * InvTextureDim * .5f / LocalPositionExtent, VolumeScale));

				// UVAdd
				ObjectData.Add(FVector4(FVector(BlockMin) * InvTextureDim + .5f * UVScale, 0));

				// Padding
				ObjectData.Add(FVector4(0, 0, 0, 0));

				checkSlow(ObjectData.Num() % FDistanceFieldObjectBuffers::ObjectDataStride == 0);
				checkSlow(ObjectData2.Num() % FDistanceFieldObjectBuffers::ObjectData2Stride == 0);

				NumObjects++;
			}
		}
	}

	if (NumObjects > 0)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_UpdateObjectBuffers);

		if (NumObjects > GDirectionalLightObjectBuffers.MaxObjects)
		{
			// Allocate with slack
			GDirectionalLightObjectBuffers.MaxObjects = NumObjects * 5 / 4;
			GDirectionalLightObjectBuffers.UpdateRHI();
		}

		checkSlow(NumObjects < MAX_uint16);

		void* LockedBuffer = RHILockVertexBuffer(GDirectionalLightObjectBuffers.ObjectData.Bounds, 0, GDirectionalLightObjectBuffers.ObjectData.Bounds->GetSize(), RLM_WriteOnly);
		FPlatformMemory::Memcpy(LockedBuffer, ObjectBoundsData.GetData(), ObjectBoundsData.GetTypeSize() * ObjectBoundsData.Num());
		RHIUnlockVertexBuffer(GDirectionalLightObjectBuffers.ObjectData.Bounds);

		LockedBuffer = RHILockVertexBuffer(GDirectionalLightObjectBuffers.ObjectData.Data, 0, GDirectionalLightObjectBuffers.ObjectData.Data->GetSize(), RLM_WriteOnly);
		FPlatformMemory::Memcpy(LockedBuffer, ObjectData.GetData(), ObjectData.GetTypeSize() * ObjectData.Num());
		RHIUnlockVertexBuffer(GDirectionalLightObjectBuffers.ObjectData.Data);

		LockedBuffer = RHILockVertexBuffer(GDirectionalLightObjectBuffers.ObjectData.Data2, 0, GDirectionalLightObjectBuffers.ObjectData.Data2->GetSize(), RLM_WriteOnly);
		FPlatformMemory::Memcpy(LockedBuffer, ObjectData2.GetData(), ObjectData2.GetTypeSize() * ObjectData2.Num());
		RHIUnlockVertexBuffer(GDirectionalLightObjectBuffers.ObjectData.Data2);
	}

	return NumObjects;
}

void FProjectedShadowInfo::RenderRayTracedDistanceFieldProjection(FRHICommandListImmediate& RHICmdList, int32 ViewIndex, const FViewInfo& View) const
{
	if (GDistanceFieldShadowing
		&& View.GetFeatureLevel() >= ERHIFeatureLevel::SM5
		&& DoesPlatformSupportDistanceFieldShadowing(GRHIShaderPlatform))
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_RenderRayTracedDistanceFieldShadows);
		SCOPED_DRAW_EVENT(RayTracedDistanceFieldShadow, DEC_SCENE_ITEMS);

		// Update the global distance field atlas
		GDistanceFieldVolumeTextureAtlas.UpdateAllocations();

		if (GDistanceFieldVolumeTextureAtlas.VolumeTextureRHI)
		{
			const int32 NumObjects = UpdateShadowCastingObjectBuffers();

			if (NumObjects > 0)
			{
				TRefCountPtr<IPooledRenderTarget> RayTracedShadowsRT;

				{
					const FIntPoint ExpandedBufferSize = GetBufferSizeForAO();
					FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(ExpandedBufferSize / GAODownsampleFactor, PF_G16R16F, TexCreate_None, TexCreate_RenderTargetable | TexCreate_UAV, false));
					GRenderTargetPool.FindFreeElement(Desc, RayTracedShadowsRT, TEXT("RayTracedShadows"));
				}

				{
					GSceneRenderTargets.FinishRenderingLightAttenuation(RHICmdList);
					SetRenderTarget(RHICmdList, NULL, NULL);

					uint32 GroupSizeX = FMath::DivideAndRoundUp(View.ViewRect.Size().X / GAODownsampleFactor, GDistanceFieldAOTileSizeX);
					uint32 GroupSizeY = FMath::DivideAndRoundUp(View.ViewRect.Size().Y / GAODownsampleFactor, GDistanceFieldAOTileSizeY);

					{
						SCOPED_DRAW_EVENT(RayTraceShadows, DEC_SCENE_ITEMS);
						TShaderMapRef<FDistanceFieldShadowingCS> ComputeShader(View.ShaderMap);

						RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
						ComputeShader->SetParameters(RHICmdList, View, this, RayTracedShadowsRT->GetRenderTargetItem(), NumObjects, FVector2D(GroupSizeX, GroupSizeY));
						DispatchComputeShader(RHICmdList, *ComputeShader, GroupSizeX, GroupSizeY, 1);

						ComputeShader->UnsetParameters(RHICmdList);
					}
				}

				{
					GSceneRenderTargets.BeginRenderingLightAttenuation(RHICmdList);

					SCOPED_DRAW_EVENT(Upsample, DEC_SCENE_ITEMS);

					RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);
					RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
					RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
					
					check(CascadeSettings.FadePlaneLength == 0);
					// first cascade rendered or old method doesn't require fading (CO_Min is needed to combine multiple shadow passes)
					// The ray traced cascade should always be first
					RHICmdList.SetBlendState(TStaticBlendState<CW_RG, BO_Min, BF_One, BF_One>::GetRHI());

					TShaderMapRef<FPostProcessVS> VertexShader( View.ShaderMap );
					TShaderMapRef<FDistanceFieldShadowingUpsamplePS> PixelShader( View.ShaderMap );

					static FGlobalBoundShaderState BoundShaderState;

					SetGlobalBoundShaderState(RHICmdList, View.GetFeatureLevel(), BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

					PixelShader->SetParameters(RHICmdList, View, this, RayTracedShadowsRT);

					DrawRectangle( 
						RHICmdList,
						0, 0, 
						View.ViewRect.Width(), View.ViewRect.Height(),
						View.ViewRect.Min.X / GAODownsampleFactor, View.ViewRect.Min.Y / GAODownsampleFactor, 
						View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor,
						FIntPoint(View.ViewRect.Width(), View.ViewRect.Height()),
						GetBufferSizeForAO() / GAODownsampleFactor,
						*VertexShader);
				}
			}
		}
	}
}
