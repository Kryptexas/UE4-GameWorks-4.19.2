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
#include "DistanceFieldLightingShared.h"
#include "DistanceFieldSurfaceCacheLighting.h"
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

int32 GShadowScatterTileCulling = 1;
FAutoConsoleVariableRef CVarShadowScatterTileCulling(
	TEXT("r.DFShadowScatterTileCulling"),
	GShadowScatterTileCulling,
	TEXT("Whether to use the rasterizer to scatter objects onto the tile grid for culling."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

float GShadowWorldTileSize = 200.0f;
FAutoConsoleVariableRef CVarShadowWorldTileSize(
	TEXT("r.DFShadowWorldTileSize"),
	GShadowWorldTileSize,
	TEXT("World space size of a tile used for culling for directional lights."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

/**  */
class FClearTilesCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FClearTilesCS,Global)
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldShadowing(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), GDistanceFieldAOTileSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), GDistanceFieldAOTileSizeY);
	}

	FClearTilesCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		TileHeadDataUnpacked.Bind(Initializer.ParameterMap, TEXT("TileHeadDataUnpacked"));
		NumGroups.Bind(Initializer.ParameterMap, TEXT("NumGroups"));
	}

	FClearTilesCS()
	{
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, FVector2D NumGroupsValue, FLightTileIntersectionResources* TileIntersectionResources)
	{
		FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		TileHeadDataUnpacked.SetBuffer(RHICmdList, ShaderRHI, TileIntersectionResources->TileHeadDataUnpacked);

		SetShaderValue(RHICmdList, ShaderRHI, NumGroups, NumGroupsValue);
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		TileHeadDataUnpacked.UnsetUAV(RHICmdList, GetComputeShader());
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << TileHeadDataUnpacked;
		Ar << NumGroups;
		return bShaderHasOutdatedParameters;
	}

private:

	FRWShaderParameter TileHeadDataUnpacked;
	FShaderParameter NumGroups;
};

IMPLEMENT_SHADER_TYPE(,FClearTilesCS,TEXT("DistanceFieldShadowing"),TEXT("ClearTilesCS"),SF_Compute);


/**  */
class FShadowObjectCullVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FShadowObjectCullVS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) 
	{ 
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldShadowing(Platform); 
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);
	}

	FShadowObjectCullVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		ObjectParameters.Bind(Initializer.ParameterMap);
		ConservativeRadiusScale.Bind(Initializer.ParameterMap, TEXT("ConservativeRadiusScale"));
		WorldToShadow.Bind(Initializer.ParameterMap, TEXT("WorldToShadow"));
		MinRadius.Bind(Initializer.ParameterMap, TEXT("MinRadius"));
	}

	FShadowObjectCullVS() {}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, FVector2D NumGroupsValue, const FProjectedShadowInfo* ProjectedShadowInfo, int32 NumObjects)
	{
		const FVertexShaderRHIParamRef ShaderRHI = GetVertexShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		
		ObjectParameters.Set(RHICmdList, ShaderRHI, *ProjectedShadowInfo->LightSceneInfo->DistanceFieldObjectBuffers, NumObjects);

		const int32 NumRings = StencilingGeometry::GLowPolyStencilSphereVertexBuffer.GetNumRings();
		const float RadiansPerRingSegment = PI / (float)NumRings;

		// Boost the effective radius so that the edges of the sphere approximation lie on the sphere, instead of the vertices
		const float ConservativeRadiusScaleValue = 1.0f / FMath::Cos(RadiansPerRingSegment);

		SetShaderValue(RHICmdList, ShaderRHI, ConservativeRadiusScale, ConservativeRadiusScaleValue);

		FMatrix WorldToShadowMatrixValue = FTranslationMatrix(ProjectedShadowInfo->PreShadowTranslation) * ProjectedShadowInfo->SubjectAndReceiverMatrix;
		SetShaderValue(RHICmdList, ShaderRHI, WorldToShadow, WorldToShadowMatrixValue);

		float MinRadiusValue = 2 * ProjectedShadowInfo->ShadowBounds.W / FMath::Min(NumGroupsValue.X, NumGroupsValue.Y);
		SetShaderValue(RHICmdList, ShaderRHI, MinRadius, MinRadiusValue);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << ObjectParameters;
		Ar << ConservativeRadiusScale;
		Ar << WorldToShadow;
		Ar << MinRadius;
		return bShaderHasOutdatedParameters;
	}

private:
	FDistanceFieldObjectBufferParameters ObjectParameters;
	FShaderParameter ConservativeRadiusScale;
	FShaderParameter WorldToShadow;
	FShaderParameter MinRadius;
};

IMPLEMENT_SHADER_TYPE(,FShadowObjectCullVS,TEXT("DistanceFieldShadowing"),TEXT("ShadowObjectCullVS"),SF_Vertex);

class FShadowObjectCullPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FShadowObjectCullPS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldShadowing(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);
		
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);
		OutEnvironment.SetDefine(TEXT("MAX_OBJECTS_PER_TILE"), GMaxNumObjectsPerTile);
	}

	/** Default constructor. */
	FShadowObjectCullPS() {}

	/** Initialization constructor. */
	FShadowObjectCullPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		ObjectParameters.Bind(Initializer.ParameterMap);
		TileHeadDataUnpacked.Bind(Initializer.ParameterMap, TEXT("TileHeadDataUnpacked"));
		TileArrayData.Bind(Initializer.ParameterMap, TEXT("TileArrayData"));
		NumGroups.Bind(Initializer.ParameterMap, TEXT("NumGroups"));
	}

	void SetParameters(
		FRHICommandList& RHICmdList, 
		const FSceneView& View, 
		FVector2D NumGroupsValue, 
		int32 NumObjects, 
		const FProjectedShadowInfo* ProjectedShadowInfo,
		FLightTileIntersectionResources* TileIntersectionResources)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);

		ObjectParameters.Set(RHICmdList, ShaderRHI, *ProjectedShadowInfo->LightSceneInfo->DistanceFieldObjectBuffers, NumObjects);
		SetShaderValue(RHICmdList, ShaderRHI, NumGroups, NumGroupsValue);
	}

	void GetUAVs(const FSceneView& View, FLightTileIntersectionResources* TileIntersectionResources, TArray<FUnorderedAccessViewRHIParamRef>& UAVs)
	{
		int32 MaxIndex = FMath::Max(TileHeadDataUnpacked.GetUAVIndex(), TileArrayData.GetUAVIndex());
		UAVs.AddZeroed(MaxIndex + 1);

		if (TileHeadDataUnpacked.IsBound())
		{
			UAVs[TileHeadDataUnpacked.GetUAVIndex()] = TileIntersectionResources->TileHeadDataUnpacked.UAV;
		}

		if (TileArrayData.IsBound())
		{
			UAVs[TileArrayData.GetUAVIndex()] = TileIntersectionResources->TileArrayData.UAV;
		}

		check(UAVs.Num() > 0);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << ObjectParameters;
		Ar << TileHeadDataUnpacked;
		Ar << TileArrayData;
		Ar << NumGroups;
		return bShaderHasOutdatedParameters;
	}

private:
	FDistanceFieldObjectBufferParameters ObjectParameters;
	FRWShaderParameter TileHeadDataUnpacked;
	FRWShaderParameter TileArrayData;
	FShaderParameter NumGroups;
};

IMPLEMENT_SHADER_TYPE(,FShadowObjectCullPS,TEXT("DistanceFieldShadowing"),TEXT("ShadowObjectCullPS"),SF_Pixel);



enum EDistanceFieldShadowingType
{
	DFS_DirectionalLightScatterTileCulling,
	DFS_DirectionalLightTiledCulling,
	DFS_PointLightTiledCulling
};

template<EDistanceFieldShadowingType ShadowingType>
class TDistanceFieldShadowingCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(TDistanceFieldShadowingCS, Global);
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
		OutEnvironment.SetDefine(TEXT("MAX_OBJECTS_PER_TILE"), GMaxNumObjectsPerTile);
		OutEnvironment.SetDefine(TEXT("SCATTER_TILE_CULLING"), ShadowingType == DFS_DirectionalLightScatterTileCulling ? TEXT("1") : TEXT("0"));
		OutEnvironment.SetDefine(TEXT("POINT_LIGHT"), ShadowingType == DFS_PointLightTiledCulling ? TEXT("1") : TEXT("0"));
	}

	/** Default constructor. */
	TDistanceFieldShadowingCS() {}

	/** Initialization constructor. */
	TDistanceFieldShadowingCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		ShadowFactors.Bind(Initializer.ParameterMap, TEXT("ShadowFactors"));
		NumGroups.Bind(Initializer.ParameterMap, TEXT("NumGroups"));
		LightDirection.Bind(Initializer.ParameterMap, TEXT("LightDirection"));
		LightSourceRadius.Bind(Initializer.ParameterMap, TEXT("LightSourceRadius"));
		LightPositionAndInvRadius.Bind(Initializer.ParameterMap, TEXT("LightPositionAndInvRadius"));
		TanLightAngleAndNormalThreshold.Bind(Initializer.ParameterMap, TEXT("TanLightAngleAndNormalThreshold"));
		ScissorRectMinAndSize.Bind(Initializer.ParameterMap, TEXT("ScissorRectMinAndSize"));
		ObjectParameters.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		TileHeadDataUnpacked.Bind(Initializer.ParameterMap, TEXT("TileHeadDataUnpacked"));
		TileArrayData.Bind(Initializer.ParameterMap, TEXT("TileArrayData"));
		TileListGroupSize.Bind(Initializer.ParameterMap, TEXT("TileListGroupSize"));
		WorldToShadow.Bind(Initializer.ParameterMap, TEXT("WorldToShadow"));
	}

	void SetParameters(
		FRHICommandList& RHICmdList, 
		const FSceneView& View, 
		const FProjectedShadowInfo* ProjectedShadowInfo,
		FSceneRenderTargetItem& ShadowFactorsValue, 
		int32 NumObjects, 
		FVector2D NumGroupsValue,
		const FIntRect& ScissorRect,
		FLightTileIntersectionResources* TileIntersectionResources)
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);

		ShadowFactors.SetTexture(RHICmdList, ShaderRHI, ShadowFactorsValue.ShaderResourceTexture, ShadowFactorsValue.UAV);

		ObjectParameters.Set(RHICmdList, ShaderRHI, *ProjectedShadowInfo->LightSceneInfo->DistanceFieldObjectBuffers, NumObjects);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);

		SetShaderValue(RHICmdList, ShaderRHI, NumGroups, NumGroupsValue);

		FVector4 LightPositionAndInvRadiusValue;
		FVector4 LightColorAndFalloffExponent;
		FVector NormalizedLightDirection;
		FVector2D SpotAngles;
		float LightSourceRadiusValue;
		float LightSourceLength;
		float LightMinRoughness;

		ProjectedShadowInfo->LightSceneInfo->Proxy->GetParameters(LightPositionAndInvRadiusValue, LightColorAndFalloffExponent, NormalizedLightDirection, SpotAngles, LightSourceRadiusValue, LightSourceLength, LightMinRoughness);

		SetShaderValue(RHICmdList, ShaderRHI, LightDirection, NormalizedLightDirection);
		SetShaderValue(RHICmdList, ShaderRHI, LightPositionAndInvRadius, LightPositionAndInvRadiusValue);
		// Default light source radius of 0 gives poor results
		SetShaderValue(RHICmdList, ShaderRHI, LightSourceRadius, LightSourceRadiusValue == 0 ? 20 : FMath::Clamp(LightSourceRadiusValue, .001f, 1.0f / (4 * LightPositionAndInvRadiusValue.W)));

		const float LightSourceAngle = FMath::Clamp(ProjectedShadowInfo->LightSceneInfo->Proxy->GetLightSourceAngle(), 0.001f, 5.0f) * PI / 180.0f;
		const FVector2D TanLightAngleAndNormalThresholdValue(FMath::Tan(LightSourceAngle), FMath::Cos(PI / 2 + LightSourceAngle));
		SetShaderValue(RHICmdList, ShaderRHI, TanLightAngleAndNormalThreshold, TanLightAngleAndNormalThresholdValue);

		SetShaderValue(RHICmdList, ShaderRHI, ScissorRectMinAndSize, FIntRect(ScissorRect.Min, ScissorRect.Size()));

		check(TileIntersectionResources || !TileHeadDataUnpacked.IsBound());

		if (TileIntersectionResources)
		{
			SetSRVParameter(RHICmdList, ShaderRHI, TileHeadDataUnpacked, TileIntersectionResources->TileHeadDataUnpacked.SRV);
			SetSRVParameter(RHICmdList, ShaderRHI, TileArrayData, TileIntersectionResources->TileArrayData.SRV);
			SetShaderValue(RHICmdList, ShaderRHI, TileListGroupSize, TileIntersectionResources->TileDimensions);
		}

		FMatrix WorldToShadowMatrixValue = FTranslationMatrix(ProjectedShadowInfo->PreShadowTranslation) * ProjectedShadowInfo->SubjectAndReceiverMatrix;
		SetShaderValue(RHICmdList, ShaderRHI, WorldToShadow, WorldToShadowMatrixValue);
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
		Ar << LightPositionAndInvRadius;
		Ar << LightSourceRadius;
		Ar << TanLightAngleAndNormalThreshold;
		Ar << ScissorRectMinAndSize;
		Ar << ObjectParameters;
		Ar << DeferredParameters;
		Ar << TileHeadDataUnpacked;
		Ar << TileArrayData;
		Ar << TileListGroupSize;
		Ar << WorldToShadow;
		return bShaderHasOutdatedParameters;
	}

private:

	FRWShaderParameter ShadowFactors;
	FShaderParameter NumGroups;
	FShaderParameter LightDirection;
	FShaderParameter LightPositionAndInvRadius;
	FShaderParameter LightSourceRadius;
	FShaderParameter TanLightAngleAndNormalThreshold;
	FShaderParameter ScissorRectMinAndSize;
	FDistanceFieldObjectBufferParameters ObjectParameters;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderResourceParameter TileHeadDataUnpacked;
	FShaderResourceParameter TileArrayData;
	FShaderParameter TileListGroupSize;
	FShaderParameter WorldToShadow;
};

IMPLEMENT_SHADER_TYPE(template<>,TDistanceFieldShadowingCS<DFS_DirectionalLightScatterTileCulling>,TEXT("DistanceFieldShadowing"),TEXT("DistanceFieldShadowingCS"),SF_Compute);
IMPLEMENT_SHADER_TYPE(template<>,TDistanceFieldShadowingCS<DFS_DirectionalLightTiledCulling>,TEXT("DistanceFieldShadowing"),TEXT("DistanceFieldShadowingCS"),SF_Compute);
IMPLEMENT_SHADER_TYPE(template<>,TDistanceFieldShadowingCS<DFS_PointLightTiledCulling>,TEXT("DistanceFieldShadowing"),TEXT("DistanceFieldShadowingCS"),SF_Compute);

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
		ScissorRectMinAndSize.Bind(Initializer.ParameterMap,TEXT("ScissorRectMinAndSize"));
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, const FProjectedShadowInfo* ShadowInfo, const FIntRect& ScissorRect, TRefCountPtr<IPooledRenderTarget>& ShadowFactorsTextureValue)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);

		SetTextureParameter(RHICmdList, ShaderRHI, ShadowFactorsTexture, ShadowFactorsSampler, TStaticSamplerState<SF_Bilinear>::GetRHI(), ShadowFactorsTextureValue->GetRenderTargetItem().ShaderResourceTexture);
	
		SetShaderValue(RHICmdList, ShaderRHI, ScissorRectMinAndSize, FIntRect(ScissorRect.Min, ScissorRect.Size()));
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << ShadowFactorsTexture;
		Ar << ShadowFactorsSampler;
		Ar << ScissorRectMinAndSize;
		return bShaderHasOutdatedParameters;
	}

private:

	FDeferredPixelShaderParameters DeferredParameters;
	FShaderResourceParameter ShadowFactorsTexture;
	FShaderResourceParameter ShadowFactorsSampler;
	FShaderParameter ScissorRectMinAndSize;
};

IMPLEMENT_SHADER_TYPE(,FDistanceFieldShadowingUpsamplePS,TEXT("DistanceFieldShadowing"),TEXT("DistanceFieldShadowingUpsamplePS"),SF_Pixel);

int32 FProjectedShadowInfo::UpdateShadowCastingObjectBuffers() const
{
	int32 NumObjects = 0;

	//@todo - scene rendering allocator
	TArray<FVector4> ObjectBoundsData;
	TArray<FVector4> ObjectData;
	TArray<FVector4> ObjectBoxBounds;

	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_GatherObjectData);

		const int32 NumTexelsOneDimX = GDistanceFieldVolumeTextureAtlas.GetSizeX();
		const int32 NumTexelsOneDimY = GDistanceFieldVolumeTextureAtlas.GetSizeY();
		const int32 NumTexelsOneDimZ = GDistanceFieldVolumeTextureAtlas.GetSizeZ();
		const FVector InvTextureDim(1.0f / NumTexelsOneDimX, 1.0f / NumTexelsOneDimY, 1.0f / NumTexelsOneDimZ);
		const FMatrix WorldToShadow = FTranslationMatrix(PreShadowTranslation) * SubjectAndReceiverMatrix;

		ObjectBoundsData.Empty(SubjectPrimitives.Num());
		ObjectData.Empty(FDistanceFieldObjectBuffers::ObjectDataStride * SubjectPrimitives.Num());
		ObjectBoxBounds.Empty(FDistanceFieldObjectBuffers::ObjectBoxBoundsStride * SubjectPrimitives.Num());

		for (int32 PrimitiveIndex = 0; PrimitiveIndex < SubjectPrimitives.Num() && NumObjects < MAX_uint16; PrimitiveIndex++)
		{
			const FPrimitiveSceneInfo* PrimitiveSceneInfo = SubjectPrimitives[PrimitiveIndex];

			FBox LocalVolumeBounds;
			FIntVector BlockMin;
			FIntVector BlockSize;
			bool bBuiltAsIfTwoSided;
			bool bMeshWasPlane;
			PrimitiveSceneInfo->Proxy->GetDistancefieldAtlasData(LocalVolumeBounds, BlockMin, BlockSize, bBuiltAsIfTwoSided, bMeshWasPlane);

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

				{
					FVector LocalBoundsVertices[8];
					LocalBoundsVertices[0] = FVector(LocalVolumeBounds.Min.X, LocalVolumeBounds.Min.Y, LocalVolumeBounds.Min.Z);
					LocalBoundsVertices[1] = FVector(LocalVolumeBounds.Max.X, LocalVolumeBounds.Min.Y, LocalVolumeBounds.Min.Z);
					LocalBoundsVertices[2] = FVector(LocalVolumeBounds.Min.X, LocalVolumeBounds.Max.Y, LocalVolumeBounds.Min.Z);
					LocalBoundsVertices[3] = FVector(LocalVolumeBounds.Max.X, LocalVolumeBounds.Max.Y, LocalVolumeBounds.Min.Z);
					LocalBoundsVertices[4] = FVector(LocalVolumeBounds.Min.X, LocalVolumeBounds.Min.Y, LocalVolumeBounds.Max.Z);
					LocalBoundsVertices[5] = FVector(LocalVolumeBounds.Max.X, LocalVolumeBounds.Min.Y, LocalVolumeBounds.Max.Z);
					LocalBoundsVertices[6] = FVector(LocalVolumeBounds.Min.X, LocalVolumeBounds.Max.Y, LocalVolumeBounds.Max.Z);
					LocalBoundsVertices[7] = FVector(LocalVolumeBounds.Max.X, LocalVolumeBounds.Max.Y, LocalVolumeBounds.Max.Z);

					FVector ViewSpaceBoundsVertices[8];
					FVector MinViewSpacePosition(WORLD_MAX);
					FVector MaxViewSpacePosition(-WORLD_MAX);

					for (int32 i = 0; i < ARRAY_COUNT(LocalBoundsVertices); i++)
					{
						const FVector ViewSpacePosition = WorldToShadow.TransformPosition(LocalToWorld.TransformPosition(LocalBoundsVertices[i]));
						MinViewSpacePosition = MinViewSpacePosition.ComponentMin(ViewSpacePosition);
						MaxViewSpacePosition = MaxViewSpacePosition.ComponentMax(ViewSpacePosition);
						ViewSpaceBoundsVertices[i] = ViewSpacePosition;
					}

					FVector ObjectXAxis = (ViewSpaceBoundsVertices[1] - ViewSpaceBoundsVertices[0]) / 2.0f;
					FVector ObjectYAxis = (ViewSpaceBoundsVertices[2] - ViewSpaceBoundsVertices[0]) / 2.0f;
					FVector ObjectZAxis = (ViewSpaceBoundsVertices[4] - ViewSpaceBoundsVertices[0]) / 2.0f;

					ObjectBoxBounds.Add(FVector4(MinViewSpacePosition, 0));
					ObjectBoxBounds.Add(FVector4(MaxViewSpacePosition, 0));
					ObjectBoxBounds.Add(FVector4(ObjectXAxis / ObjectXAxis.SizeSquared(), 0));
					ObjectBoxBounds.Add(FVector4(ObjectYAxis / ObjectYAxis.SizeSquared(), 0));
					ObjectBoxBounds.Add(FVector4(ObjectZAxis / ObjectZAxis.SizeSquared(), 0));
				}

				checkSlow(ObjectData.Num() % FDistanceFieldObjectBuffers::ObjectDataStride == 0);
				checkSlow(ObjectBoxBounds.Num() % FDistanceFieldObjectBuffers::ObjectBoxBoundsStride == 0);

				NumObjects++;
			}
		}
	}

	if (NumObjects > 0)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_UpdateObjectBuffers);

		if (!LightSceneInfo->DistanceFieldObjectBuffers)
		{
			LightSceneInfo->DistanceFieldObjectBuffers = new FDistanceFieldObjectBuffers;
		}

		FDistanceFieldObjectBuffers& ObjectBuffers = *LightSceneInfo->DistanceFieldObjectBuffers;

		if (NumObjects > ObjectBuffers.MaxObjects)
		{
			// Allocate with slack
			ObjectBuffers.bWantBoxBounds = true;
			ObjectBuffers.bWantVolumeToWorld = false;
			ObjectBuffers.MaxObjects = NumObjects * 5 / 4;
			ObjectBuffers.Release();
			ObjectBuffers.Initialize();
		}

		checkSlow(NumObjects < MAX_uint16);

		void* LockedBuffer = RHILockVertexBuffer(ObjectBuffers.ObjectData.Bounds, 0, ObjectBuffers.ObjectData.Bounds->GetSize(), RLM_WriteOnly);
		FPlatformMemory::Memcpy(LockedBuffer, ObjectBoundsData.GetData(), ObjectBoundsData.GetTypeSize() * ObjectBoundsData.Num());
		RHIUnlockVertexBuffer(ObjectBuffers.ObjectData.Bounds);

		LockedBuffer = RHILockVertexBuffer(ObjectBuffers.ObjectData.Data, 0, ObjectBuffers.ObjectData.Data->GetSize(), RLM_WriteOnly);
		FPlatformMemory::Memcpy(LockedBuffer, ObjectData.GetData(), ObjectData.GetTypeSize() * ObjectData.Num());
		RHIUnlockVertexBuffer(ObjectBuffers.ObjectData.Data);

		LockedBuffer = RHILockVertexBuffer(ObjectBuffers.ObjectData.BoxBounds, 0, ObjectBuffers.ObjectData.BoxBounds->GetSize(), RLM_WriteOnly);
		FPlatformMemory::Memcpy(LockedBuffer, ObjectBoxBounds.GetData(), ObjectBoxBounds.GetTypeSize() * ObjectBoxBounds.Num());
		RHIUnlockVertexBuffer(ObjectBuffers.ObjectData.BoxBounds);
	}

	return NumObjects;
}

void FProjectedShadowInfo::RenderRayTracedDistanceFieldProjection(FRHICommandListImmediate& RHICmdList, const FViewInfo& View) const
{
	if (GDistanceFieldShadowing
		&& View.GetFeatureLevel() >= ERHIFeatureLevel::SM5
		&& DoesPlatformSupportDistanceFieldShadowing(GRHIShaderPlatform))
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_RenderRayTracedDistanceFieldShadows);
		SCOPED_DRAW_EVENT(RHICmdList, RayTracedDistanceFieldShadow, DEC_SCENE_ITEMS);

		// Update the global distance field atlas
		GDistanceFieldVolumeTextureAtlas.UpdateAllocations();

		if (GDistanceFieldVolumeTextureAtlas.VolumeTextureRHI)
		{
			const int32 NumObjects = UpdateShadowCastingObjectBuffers();

			if (NumObjects > 0)
			{
				// Allocate tile resolution based on world space size
				//@todo - light space perspective shadow maps would make much better use of the resolution
				const float LightTiles = FMath::Min(ShadowBounds.W / GShadowWorldTileSize + 1, 256.0f);
				FIntPoint LightTileDimensions(LightTiles, LightTiles);

				FLightTileIntersectionResources* TileIntersectionResources = NULL;

				GSceneRenderTargets.FinishRenderingLightAttenuation(RHICmdList);
				SetRenderTarget(RHICmdList, NULL, NULL);

				if (bDirectionalLight && GShadowScatterTileCulling)
				{
					if (!LightSceneInfo->TileIntersectionResources || LightSceneInfo->TileIntersectionResources->TileDimensions != LightTileDimensions)
					{
						if (LightSceneInfo->TileIntersectionResources)
						{
							LightSceneInfo->TileIntersectionResources->Release();
						}
						else
						{
							LightSceneInfo->TileIntersectionResources = new FLightTileIntersectionResources();
						}

						LightSceneInfo->TileIntersectionResources->TileDimensions = LightTileDimensions;

						LightSceneInfo->TileIntersectionResources->Initialize();
					}

					TileIntersectionResources = LightSceneInfo->TileIntersectionResources;

					{
						SCOPED_DRAW_EVENT(RHICmdList, ClearTiles, DEC_SCENE_ITEMS);
						TShaderMapRef<FClearTilesCS> ComputeShader(View.ShaderMap);

						uint32 GroupSizeX = FMath::DivideAndRoundUp(LightTileDimensions.X, GDistanceFieldAOTileSizeX);
						uint32 GroupSizeY = FMath::DivideAndRoundUp(LightTileDimensions.Y, GDistanceFieldAOTileSizeY);

						RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
						ComputeShader->SetParameters(RHICmdList, View, FVector2D(LightTileDimensions.X, LightTileDimensions.Y), TileIntersectionResources);
						DispatchComputeShader(RHICmdList, *ComputeShader, GroupSizeX, GroupSizeY, 1);

						ComputeShader->UnsetParameters(RHICmdList);
					}

					{
						SCOPED_DRAW_EVENT(RHICmdList, CullObjects, DEC_SCENE_ITEMS);

						TShaderMapRef<FShadowObjectCullVS> VertexShader(View.ShaderMap);
						TShaderMapRef<FShadowObjectCullPS> PixelShader(View.ShaderMap);

						TArray<FUnorderedAccessViewRHIParamRef> UAVs;
						PixelShader->GetUAVs(View, TileIntersectionResources, UAVs);
						RHICmdList.SetRenderTargets(0, (const FRHIRenderTargetView *)NULL, NULL, UAVs.Num(), UAVs.GetData());

						RHICmdList.SetViewport(0, 0, 0.0f, LightTileDimensions.X, LightTileDimensions.Y, 1.0f);

						// Render backfaces since camera may intersect
						RHICmdList.SetRasterizerState(View.bReverseCulling ? TStaticRasterizerState<FM_Solid, CM_CW>::GetRHI() : TStaticRasterizerState<FM_Solid, CM_CCW>::GetRHI());
						RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
						RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());

						static FGlobalBoundShaderState BoundShaderState;

						SetGlobalBoundShaderState(RHICmdList, View.GetFeatureLevel(), BoundShaderState, GetVertexDeclarationFVector4(), *VertexShader, *PixelShader);

						VertexShader->SetParameters(RHICmdList, View, FVector2D(LightTileDimensions.X, LightTileDimensions.Y), this, NumObjects);
						PixelShader->SetParameters(RHICmdList, View, FVector2D(LightTileDimensions.X, LightTileDimensions.Y), NumObjects, this, TileIntersectionResources);

						RHICmdList.SetStreamSource(0, StencilingGeometry::GLowPolyStencilSphereVertexBuffer.VertexBufferRHI, sizeof(FVector4), 0);

						RHICmdList.DrawIndexedPrimitive(
							StencilingGeometry::GLowPolyStencilSphereIndexBuffer.IndexBufferRHI, 
							PT_TriangleList, 
							0,
							0, 
							StencilingGeometry::GLowPolyStencilSphereVertexBuffer.GetVertexCount(), 
							0, 
							StencilingGeometry::GLowPolyStencilSphereIndexBuffer.GetIndexCount() / 3, 
							NumObjects);
					}
				}

				TRefCountPtr<IPooledRenderTarget> RayTracedShadowsRT;

				{
					const FIntPoint ExpandedBufferSize = GetBufferSizeForAO();
					FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(ExpandedBufferSize / GAODownsampleFactor, PF_G16R16F, TexCreate_None, TexCreate_RenderTargetable | TexCreate_UAV, false));
					GRenderTargetPool.FindFreeElement(Desc, RayTracedShadowsRT, TEXT("RayTracedShadows"));
				}

				FIntRect ScissorRect;

				{
					if (!LightSceneInfo->Proxy->GetScissorRect(ScissorRect, View))
					{
						ScissorRect = View.ViewRect;
					}

					uint32 GroupSizeX = FMath::DivideAndRoundUp(ScissorRect.Size().X / GAODownsampleFactor, GDistanceFieldAOTileSizeX);
					uint32 GroupSizeY = FMath::DivideAndRoundUp(ScissorRect.Size().Y / GAODownsampleFactor, GDistanceFieldAOTileSizeY);

					{
						SCOPED_DRAW_EVENT(RHICmdList, RayTraceShadows, DEC_SCENE_ITEMS);
						SetRenderTarget(RHICmdList, NULL, NULL);

						if (bDirectionalLight && GShadowScatterTileCulling)
						{
							TShaderMapRef<TDistanceFieldShadowingCS<DFS_DirectionalLightScatterTileCulling> > ComputeShader(View.ShaderMap);
							RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
							ComputeShader->SetParameters(RHICmdList, View, this, RayTracedShadowsRT->GetRenderTargetItem(), NumObjects, FVector2D(GroupSizeX, GroupSizeY), ScissorRect, TileIntersectionResources);
							DispatchComputeShader(RHICmdList, *ComputeShader, GroupSizeX, GroupSizeY, 1);
							ComputeShader->UnsetParameters(RHICmdList);
						}
						else if (bDirectionalLight)
						{
							TShaderMapRef<TDistanceFieldShadowingCS<DFS_DirectionalLightTiledCulling> > ComputeShader(View.ShaderMap);
							RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
							ComputeShader->SetParameters(RHICmdList, View, this, RayTracedShadowsRT->GetRenderTargetItem(), NumObjects, FVector2D(GroupSizeX, GroupSizeY), ScissorRect, TileIntersectionResources);
							DispatchComputeShader(RHICmdList, *ComputeShader, GroupSizeX, GroupSizeY, 1);
							ComputeShader->UnsetParameters(RHICmdList);
						}
						else
						{
							TShaderMapRef<TDistanceFieldShadowingCS<DFS_PointLightTiledCulling> > ComputeShader(View.ShaderMap);
							RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
							ComputeShader->SetParameters(RHICmdList, View, this, RayTracedShadowsRT->GetRenderTargetItem(), NumObjects, FVector2D(GroupSizeX, GroupSizeY), ScissorRect, TileIntersectionResources);
							DispatchComputeShader(RHICmdList, *ComputeShader, GroupSizeX, GroupSizeY, 1);
							ComputeShader->UnsetParameters(RHICmdList);
						}
					}
				}

				{
					GSceneRenderTargets.BeginRenderingLightAttenuation(RHICmdList);

					SCOPED_DRAW_EVENT(RHICmdList, Upsample, DEC_SCENE_ITEMS);

					RHICmdList.SetViewport(ScissorRect.Min.X, ScissorRect.Min.Y, 0.0f, ScissorRect.Max.X, ScissorRect.Max.Y, 1.0f);
					RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
					RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
					
					if (bDirectionalLight)
					{
						check(CascadeSettings.FadePlaneLength == 0);
						// first cascade rendered or old method doesn't require fading (CO_Min is needed to combine multiple shadow passes)
						// The ray traced cascade should always be first
						RHICmdList.SetBlendState(TStaticBlendState<CW_RG, BO_Min, BF_One, BF_One>::GetRHI());
					}
					else
					{
						// use B and A in Light Attenuation
						// (CO_Min is needed to combine multiple shadow passes)
						RHICmdList.SetBlendState(TStaticBlendState<CW_BA, BO_Min, BF_One, BF_One, BO_Min, BF_One, BF_One>::GetRHI());
					}

					TShaderMapRef<FPostProcessVS> VertexShader(View.ShaderMap);
					TShaderMapRef<FDistanceFieldShadowingUpsamplePS> PixelShader(View.ShaderMap);

					static FGlobalBoundShaderState BoundShaderState;

					SetGlobalBoundShaderState(RHICmdList, View.GetFeatureLevel(), BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

					VertexShader->SetParameters(RHICmdList, View);
					PixelShader->SetParameters(RHICmdList, View, this, ScissorRect, RayTracedShadowsRT);

					DrawRectangle( 
						RHICmdList,
						0, 0, 
						ScissorRect.Width(), ScissorRect.Height(),
						ScissorRect.Min.X / GAODownsampleFactor, ScissorRect.Min.Y / GAODownsampleFactor, 
						ScissorRect.Width() / GAODownsampleFactor, ScissorRect.Height() / GAODownsampleFactor,
						FIntPoint(ScissorRect.Width(), ScissorRect.Height()),
						GetBufferSizeForAO() / GAODownsampleFactor,
						*VertexShader);
				}
			}
		}
	}
}
