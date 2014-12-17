// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DistanceFieldGlobalIllumination.cpp
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "UniformBuffer.h"
#include "ShaderParameters.h"
#include "PostProcessing.h"
#include "SceneFilterRendering.h"
#include "DistanceFieldLightingShared.h"
#include "DistanceFieldSurfaceCacheLighting.h"
#include "DistanceFieldGlobalIllumination.h"
#include "RHICommandList.h"
#include "SceneUtils.h"
#include "DistanceFieldAtlas.h"

int32 GDistanceFieldGI = 0;
FAutoConsoleVariableRef CVarDistanceFieldGI(
	TEXT("r.DistanceFieldGI"),
	GDistanceFieldGI,
	TEXT(""),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

int32 GVPLGridDimension = 128;
FAutoConsoleVariableRef CVarVPLGridDimension(
	TEXT("r.VPLGridDimension"),
	GVPLGridDimension,
	TEXT(""),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

float GVPLDirectionalLightTraceDistance = 100000;
FAutoConsoleVariableRef CVarVPLDirectionalLightTraceDistance(
	TEXT("r.VPLDirectionalLightTraceDistance"),
	GVPLDirectionalLightTraceDistance,
	TEXT(""),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

float GVPLPlacementCameraRadius = 4000;
FAutoConsoleVariableRef CVarVPLPlacementCameraRadius(
	TEXT("r.VPLPlacementCameraRadius"),
	GVPLPlacementCameraRadius,
	TEXT(""),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

int32 GVPLViewCulling = 1;
FAutoConsoleVariableRef CVarVPLViewCulling(
	TEXT("r.VPLViewCulling"),
	GVPLViewCulling,
	TEXT(""),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

int32 GAOUseConesForGI = 1;
FAutoConsoleVariableRef CVarAOUseConesForGI(
	TEXT("r.AOUseConesForGI"),
	GAOUseConesForGI,
	TEXT(""),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

TGlobalResource<FVPLResources> GVPLResources;
TGlobalResource<FVPLResources> GCulledVPLResources;

class FVPLPlacementCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FVPLPlacementCS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), GDistanceFieldAOTileSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), GDistanceFieldAOTileSizeY);
	}

	/** Default constructor. */
	FVPLPlacementCS() {}

	/** Initialization constructor. */
	FVPLPlacementCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		VPLParameterBuffer.Bind(Initializer.ParameterMap, TEXT("VPLParameterBuffer"));
		VPLClusterData.Bind(Initializer.ParameterMap, TEXT("VPLClusterData"));
		VPLData.Bind(Initializer.ParameterMap, TEXT("VPLData"));
		InvPlacementGridSize.Bind(Initializer.ParameterMap, TEXT("InvPlacementGridSize"));
		WorldToShadow.Bind(Initializer.ParameterMap, TEXT("WorldToShadow"));
		ShadowToWorld.Bind(Initializer.ParameterMap, TEXT("ShadowToWorld"));
		LightDirectionAndTraceDistance.Bind(Initializer.ParameterMap, TEXT("LightDirectionAndTraceDistance"));
		LightColor.Bind(Initializer.ParameterMap, TEXT("LightColor"));
		ObjectParameters.Bind(Initializer.ParameterMap);
		ShadowTileHeadDataUnpacked.Bind(Initializer.ParameterMap, TEXT("ShadowTileHeadDataUnpacked"));
		ShadowTileArrayData.Bind(Initializer.ParameterMap, TEXT("ShadowTileArrayData"));
		ShadowTileListGroupSize.Bind(Initializer.ParameterMap, TEXT("ShadowTileListGroupSize"));
		VPLPlacementCameraRadius.Bind(Initializer.ParameterMap, TEXT("VPLPlacementCameraRadius"));
	}

	void SetParameters(
		FRHICommandList& RHICmdList,
		const FSceneView& View, 
		const FLightSceneProxy* LightSceneProxy,
		FVector2D InvPlacementGridSizeValue,
		const FMatrix& WorldToShadowValue,
		const FMatrix& ShadowToWorldValue,
		FLightTileIntersectionResources* TileIntersectionResources)
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		extern TGlobalResource<FDistanceFieldObjectBufferResource> GShadowCulledObjectBuffers;
		ObjectParameters.Set(RHICmdList, ShaderRHI, GShadowCulledObjectBuffers.Buffers);

		SetShaderValue(RHICmdList, ShaderRHI, InvPlacementGridSize, InvPlacementGridSizeValue);
		SetShaderValue(RHICmdList, ShaderRHI, WorldToShadow, WorldToShadowValue);
		SetShaderValue(RHICmdList, ShaderRHI, ShadowToWorld, ShadowToWorldValue);
		SetShaderValue(RHICmdList, ShaderRHI, LightDirectionAndTraceDistance, FVector4(LightSceneProxy->GetDirection(), GVPLDirectionalLightTraceDistance));
		SetShaderValue(RHICmdList, ShaderRHI, LightColor, LightSceneProxy->GetColor() * LightSceneProxy->GetIndirectLightingScale());

		VPLParameterBuffer.SetBuffer(RHICmdList, ShaderRHI, GVPLResources.VPLParameterBuffer);
		VPLClusterData.SetBuffer(RHICmdList, ShaderRHI, GVPLResources.VPLClusterData);
		VPLData.SetBuffer(RHICmdList, ShaderRHI, GVPLResources.VPLData);

		check(TileIntersectionResources || !ShadowTileHeadDataUnpacked.IsBound());

		if (TileIntersectionResources)
		{
			SetSRVParameter(RHICmdList, ShaderRHI, ShadowTileHeadDataUnpacked, TileIntersectionResources->TileHeadDataUnpacked.SRV);
			SetSRVParameter(RHICmdList, ShaderRHI, ShadowTileArrayData, TileIntersectionResources->TileArrayData.SRV);
			SetShaderValue(RHICmdList, ShaderRHI, ShadowTileListGroupSize, TileIntersectionResources->TileDimensions);
		}

		SetShaderValue(RHICmdList, ShaderRHI, VPLPlacementCameraRadius, GVPLPlacementCameraRadius);
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		VPLParameterBuffer.UnsetUAV(RHICmdList, GetComputeShader());
		VPLClusterData.UnsetUAV(RHICmdList, GetComputeShader());
		VPLData.UnsetUAV(RHICmdList, GetComputeShader());
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << VPLParameterBuffer;
		Ar << VPLClusterData;
		Ar << VPLData;
		Ar << InvPlacementGridSize;
		Ar << WorldToShadow;
		Ar << ShadowToWorld;
		Ar << LightDirectionAndTraceDistance;
		Ar << LightColor;
		Ar << ObjectParameters;
		Ar << ShadowTileHeadDataUnpacked;
		Ar << ShadowTileArrayData;
		Ar << ShadowTileListGroupSize;
		Ar << VPLPlacementCameraRadius;
		return bShaderHasOutdatedParameters;
	}

private:

	FRWShaderParameter VPLParameterBuffer;
	FRWShaderParameter VPLClusterData;
	FRWShaderParameter VPLData;
	FShaderParameter InvPlacementGridSize;
	FShaderParameter WorldToShadow;
	FShaderParameter ShadowToWorld;
	FShaderParameter LightDirectionAndTraceDistance;
	FShaderParameter LightColor;
	FDistanceFieldCulledObjectBufferParameters ObjectParameters;
	FShaderResourceParameter ShadowTileHeadDataUnpacked;
	FShaderResourceParameter ShadowTileArrayData;
	FShaderParameter ShadowTileListGroupSize;
	FShaderParameter VPLPlacementCameraRadius;
};

IMPLEMENT_SHADER_TYPE(,FVPLPlacementCS,TEXT("DistanceFieldGlobalIllumination"),TEXT("VPLPlacementCS"),SF_Compute);



class FSetupVPLCullndirectArgumentsCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSetupVPLCullndirectArgumentsCS,Global)
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), GDistanceFieldAOTileSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), GDistanceFieldAOTileSizeY);
	}

	FSetupVPLCullndirectArgumentsCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DispatchParameters.Bind(Initializer.ParameterMap, TEXT("DispatchParameters"));
		VPLParameterBuffer.Bind(Initializer.ParameterMap, TEXT("VPLParameterBuffer"));
	}

	FSetupVPLCullndirectArgumentsCS()
	{
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View)
	{
		FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);

		DispatchParameters.SetBuffer(RHICmdList, ShaderRHI, GVPLResources.VPLDispatchIndirectBuffer);
		SetSRVParameter(RHICmdList, ShaderRHI, VPLParameterBuffer, GVPLResources.VPLParameterBuffer.SRV);
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		DispatchParameters.UnsetUAV(RHICmdList, GetComputeShader());
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DispatchParameters;
		Ar << VPLParameterBuffer;
		return bShaderHasOutdatedParameters;
	}

private:

	FRWShaderParameter DispatchParameters;
	FShaderResourceParameter VPLParameterBuffer;
};

IMPLEMENT_SHADER_TYPE(,FSetupVPLCullndirectArgumentsCS,TEXT("DistanceFieldGlobalIllumination"),TEXT("SetupVPLCullndirectArgumentsCS"),SF_Compute);


class FCullVPLsForViewCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCullVPLsForViewCS,Global)
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), GDistanceFieldAOTileSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), GDistanceFieldAOTileSizeY);
	}

	FCullVPLsForViewCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		VPLParameterBuffer.Bind(Initializer.ParameterMap, TEXT("VPLParameterBuffer"));
		VPLClusterData.Bind(Initializer.ParameterMap, TEXT("VPLClusterData"));
		VPLData.Bind(Initializer.ParameterMap, TEXT("VPLData"));
		CulledVPLParameterBuffer.Bind(Initializer.ParameterMap, TEXT("CulledVPLParameterBuffer"));
		CulledVPLClusterData.Bind(Initializer.ParameterMap, TEXT("CulledVPLClusterData"));
		CulledVPLData.Bind(Initializer.ParameterMap, TEXT("CulledVPLData"));
		AOParameters.Bind(Initializer.ParameterMap);
		NumConvexHullPlanes.Bind(Initializer.ParameterMap, TEXT("NumConvexHullPlanes"));
		ViewFrustumConvexHull.Bind(Initializer.ParameterMap, TEXT("ViewFrustumConvexHull"));
	}

	FCullVPLsForViewCS()
	{
	}

	void SetParameters(FRHICommandList& RHICmdList, const FScene* Scene, const FSceneView& View, const FDistanceFieldAOParameters& Parameters)
	{
		FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);

		CulledVPLParameterBuffer.SetBuffer(RHICmdList, ShaderRHI, GCulledVPLResources.VPLParameterBuffer);
		CulledVPLClusterData.SetBuffer(RHICmdList, ShaderRHI, GCulledVPLResources.VPLClusterData);
		CulledVPLData.SetBuffer(RHICmdList, ShaderRHI, GCulledVPLResources.VPLData);
		AOParameters.Set(RHICmdList, ShaderRHI, Parameters);

		SetSRVParameter(RHICmdList, ShaderRHI, VPLParameterBuffer, GVPLResources.VPLParameterBuffer.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, VPLClusterData, GVPLResources.VPLClusterData.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, VPLData, GVPLResources.VPLData.SRV);

		// Shader assumes max 6
		check(View.ViewFrustum.Planes.Num() <= 6);
		SetShaderValue(RHICmdList, ShaderRHI, NumConvexHullPlanes, View.ViewFrustum.Planes.Num());
		SetShaderValueArray(RHICmdList, ShaderRHI, ViewFrustumConvexHull, View.ViewFrustum.Planes.GetData(), View.ViewFrustum.Planes.Num());
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		CulledVPLParameterBuffer.UnsetUAV(RHICmdList, GetComputeShader());
		CulledVPLClusterData.UnsetUAV(RHICmdList, GetComputeShader());
		CulledVPLData.UnsetUAV(RHICmdList, GetComputeShader());
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << VPLParameterBuffer;
		Ar << VPLClusterData;
		Ar << VPLData;
		Ar << CulledVPLParameterBuffer;
		Ar << CulledVPLClusterData;
		Ar << CulledVPLData;
		Ar << AOParameters;
		Ar << NumConvexHullPlanes;
		Ar << ViewFrustumConvexHull;
		return bShaderHasOutdatedParameters;
	}

private:

	FShaderResourceParameter VPLParameterBuffer;
	FShaderResourceParameter VPLClusterData;
	FShaderResourceParameter VPLData;
	FRWShaderParameter CulledVPLParameterBuffer;
	FRWShaderParameter CulledVPLClusterData;
	FRWShaderParameter CulledVPLData;
	FAOParameters AOParameters;
	FShaderParameter NumConvexHullPlanes;
	FShaderParameter ViewFrustumConvexHull;
};

IMPLEMENT_SHADER_TYPE(,FCullVPLsForViewCS,TEXT("DistanceFieldGlobalIllumination"),TEXT("CullVPLsForViewCS"),SF_Compute);


/**  */
class FVPLTileCullCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FVPLTileCullCS,Global)
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), GDistanceFieldAOTileSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), GDistanceFieldAOTileSizeY);
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	FVPLTileCullCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		AOParameters.Bind(Initializer.ParameterMap);
		TileHeadDataUnpacked.Bind(Initializer.ParameterMap, TEXT("TileHeadDataUnpacked"));
		TileArrayData.Bind(Initializer.ParameterMap, TEXT("TileArrayData"));
		VPLParameterBuffer.Bind(Initializer.ParameterMap, TEXT("VPLParameterBuffer"));
		VPLClusterData.Bind(Initializer.ParameterMap, TEXT("VPLClusterData"));
		TileConeAxisAndCos.Bind(Initializer.ParameterMap, TEXT("TileConeAxisAndCos"));
		TileConeDepthRanges.Bind(Initializer.ParameterMap, TEXT("TileConeDepthRanges"));
		NumGroups.Bind(Initializer.ParameterMap, TEXT("NumGroups"));
	}

	FVPLTileCullCS()
	{
	}
	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, const FScene* Scene, const FDistanceFieldAOParameters& Parameters, const FVPLResources& VPLResources)
	{
		FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		AOParameters.Set(RHICmdList, ShaderRHI, Parameters);

		FTileIntersectionResources* TileIntersectionResources = ((FSceneViewState*)View.State)->AOTileIntersectionResources;
		FVPLTileIntersectionResources* VPLTileIntersectionResources = ((FSceneViewState*)View.State)->VPLTileIntersectionResources;

		TileHeadDataUnpacked.SetBuffer(RHICmdList, ShaderRHI, VPLTileIntersectionResources->TileHeadDataUnpacked);
		TileArrayData.SetBuffer(RHICmdList, ShaderRHI, VPLTileIntersectionResources->TileArrayData);

		SetSRVParameter(RHICmdList, ShaderRHI, VPLParameterBuffer, VPLResources.VPLParameterBuffer.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, VPLClusterData, VPLResources.VPLClusterData.SRV);

		SetSRVParameter(RHICmdList, ShaderRHI, TileConeAxisAndCos, TileIntersectionResources->TileConeAxisAndCos.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, TileConeDepthRanges, TileIntersectionResources->TileConeDepthRanges.SRV);
	
		SetShaderValue(RHICmdList, ShaderRHI, NumGroups, VPLTileIntersectionResources->TileDimensions);
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		TileHeadDataUnpacked.UnsetUAV(RHICmdList, GetComputeShader());
		TileArrayData.UnsetUAV(RHICmdList, GetComputeShader());
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << AOParameters;
		Ar << TileHeadDataUnpacked;
		Ar << TileArrayData;
		Ar << VPLParameterBuffer;
		Ar << VPLClusterData;
		Ar << TileConeAxisAndCos;
		Ar << TileConeDepthRanges;
		Ar << NumGroups;
		return bShaderHasOutdatedParameters;
	}

private:

	FAOParameters AOParameters;
	FRWShaderParameter TileHeadDataUnpacked;
	FRWShaderParameter TileArrayData;
	FShaderResourceParameter VPLParameterBuffer;
	FShaderResourceParameter VPLClusterData;
	FShaderResourceParameter TileConeAxisAndCos;
	FShaderResourceParameter TileConeDepthRanges;
	FShaderParameter NumGroups;
};

IMPLEMENT_SHADER_TYPE(,FVPLTileCullCS,TEXT("DistanceFieldGlobalIllumination"),TEXT("VPLTileCullCS"),SF_Compute);


enum EComputeIrradianceMode
{
	IrradianceMode_IrradianceFromAOCones,
	IrradianceMode_IrradianceFinalGather
};

template<EComputeIrradianceMode CombineMode>
class TComputeIrradianceCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(TComputeIrradianceCS,Global)
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);
		OutEnvironment.SetDefine(TEXT("NUM_CONE_DIRECTIONS"), NumConeSampleDirections);
		OutEnvironment.SetDefine(TEXT("IRRADIANCE_FROM_AO_CONES"), CombineMode == IrradianceMode_IrradianceFromAOCones ? TEXT("1") : TEXT("0"));

		// To reduce shader compile time of compute shaders with shared memory, doesn't have an impact on generated code with current compiler (June 2010 DX SDK)
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	TComputeIrradianceCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		AOParameters.Bind(Initializer.ParameterMap);
		IrradianceCacheIrradiance.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheIrradiance"));
		IrradianceCachePositionRadius.Bind(Initializer.ParameterMap, TEXT("IrradianceCachePositionRadius"));
		IrradianceCacheNormal.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheNormal"));
		IrradianceCacheTileCoordinate.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheTileCoordinate"));
		RecordConeData.Bind(Initializer.ParameterMap, TEXT("RecordConeData"));
		ScatterDrawParameters.Bind(Initializer.ParameterMap, TEXT("ScatterDrawParameters"));
		SavedStartIndex.Bind(Initializer.ParameterMap, TEXT("SavedStartIndex"));
		DistanceFieldNormalTexture.Bind(Initializer.ParameterMap, TEXT("DistanceFieldNormalTexture"));
		DistanceFieldNormalSampler.Bind(Initializer.ParameterMap, TEXT("DistanceFieldNormalSampler"));
		GlobalObjectParameters.Bind(Initializer.ParameterMap);
		VPLParameterBuffer.Bind(Initializer.ParameterMap, TEXT("VPLParameterBuffer"));
		VPLClusterData.Bind(Initializer.ParameterMap, TEXT("VPLClusterData"));
		VPLData.Bind(Initializer.ParameterMap, TEXT("VPLData"));
		VPLGatherRadius.Bind(Initializer.ParameterMap, TEXT("VPLGatherRadius"));
		ConeHalfAngle.Bind(Initializer.ParameterMap, TEXT("ConeHalfAngle"));
		BentNormalNormalizeFactor.Bind(Initializer.ParameterMap, TEXT("BentNormalNormalizeFactor"));
		TileListGroupSize.Bind(Initializer.ParameterMap, TEXT("TileListGroupSize"));
		TileHeadDataUnpacked.Bind(Initializer.ParameterMap, TEXT("TileHeadDataUnpacked"));
		TileArrayData.Bind(Initializer.ParameterMap, TEXT("TileArrayData"));
	}

	TComputeIrradianceCS()
	{
	}

	void SetParameters(
		FRHICommandList& RHICmdList, 
		const FSceneView& View, 
		int32 DepthLevel, 
		const FDistanceFieldAOParameters& Parameters,
		FTemporaryIrradianceCacheResources* TemporaryIrradianceCacheResources)
	{
		FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);

		AOParameters.Set(RHICmdList, ShaderRHI, Parameters);

		const FScene* Scene = (const FScene*)View.Family->Scene;
		FSurfaceCacheResources& SurfaceCacheResources = *Scene->SurfaceCacheResources;

		SetSRVParameter(RHICmdList, ShaderRHI, IrradianceCachePositionRadius, SurfaceCacheResources.Level[DepthLevel]->PositionAndRadius.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, IrradianceCacheNormal, SurfaceCacheResources.Level[DepthLevel]->Normal.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, IrradianceCacheTileCoordinate, SurfaceCacheResources.Level[DepthLevel]->TileCoordinate.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, ScatterDrawParameters, SurfaceCacheResources.Level[DepthLevel]->ScatterDrawParameters.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, SavedStartIndex, SurfaceCacheResources.Level[DepthLevel]->SavedStartIndex.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, RecordConeData, TemporaryIrradianceCacheResources->ConeData.SRV);

		IrradianceCacheIrradiance.SetBuffer(RHICmdList, ShaderRHI, SurfaceCacheResources.Level[DepthLevel]->Irradiance);

		FAOSampleData2 AOSampleData;

		TArray<FVector, TInlineAllocator<9> > SampleDirections;
		GetSpacedVectors(SampleDirections);

		for (int32 SampleIndex = 0; SampleIndex < NumConeSampleDirections; SampleIndex++)
		{
			AOSampleData.SampleDirections[SampleIndex] = FVector4(SampleDirections[SampleIndex]);
		}

		SetUniformBufferParameterImmediate(RHICmdList, ShaderRHI, GetUniformBufferParameter<FAOSampleData2>(), AOSampleData);

		FVector UnoccludedVector(0);

		for (int32 SampleIndex = 0; SampleIndex < NumConeSampleDirections; SampleIndex++)
		{
			UnoccludedVector += SampleDirections[SampleIndex];
		}

		float BentNormalNormalizeFactorValue = 1.0f / (UnoccludedVector / NumConeSampleDirections).Size();
		SetShaderValue(RHICmdList, ShaderRHI, BentNormalNormalizeFactor, BentNormalNormalizeFactorValue);

		GlobalObjectParameters.Set(RHICmdList, ShaderRHI, *(Scene->DistanceFieldSceneData.ObjectBuffers), Scene->DistanceFieldSceneData.NumObjectsInBuffer);

		FVPLResources& SourceVPLResources = GVPLViewCulling ? GCulledVPLResources : GVPLResources;
		SetSRVParameter(RHICmdList, ShaderRHI, VPLParameterBuffer, SourceVPLResources.VPLParameterBuffer.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, VPLClusterData, SourceVPLResources.VPLClusterData.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, VPLData, SourceVPLResources.VPLData.SRV);

		SetShaderValue(RHICmdList, ShaderRHI, VPLGatherRadius, Parameters.OcclusionMaxDistance);
		extern float GAOConeHalfAngle;
		SetShaderValue(RHICmdList, ShaderRHI, ConeHalfAngle, GAOConeHalfAngle);

		FVPLTileIntersectionResources* VPLTileIntersectionResources = ((FSceneViewState*)View.State)->VPLTileIntersectionResources;

		SetShaderValue(RHICmdList, ShaderRHI, TileListGroupSize, VPLTileIntersectionResources->TileDimensions);
		SetSRVParameter(RHICmdList, ShaderRHI, TileHeadDataUnpacked, VPLTileIntersectionResources->TileHeadDataUnpacked.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, TileArrayData, VPLTileIntersectionResources->TileArrayData.SRV);
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		IrradianceCacheIrradiance.UnsetUAV(RHICmdList, GetComputeShader());
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << AOParameters;
		Ar << IrradianceCacheIrradiance;
		Ar << IrradianceCachePositionRadius;
		Ar << IrradianceCacheNormal;
		Ar << IrradianceCacheTileCoordinate;
		Ar << RecordConeData;
		Ar << ScatterDrawParameters;
		Ar << SavedStartIndex;
		Ar << DistanceFieldNormalTexture;
		Ar << DistanceFieldNormalSampler;
		Ar << GlobalObjectParameters;
		Ar << VPLParameterBuffer;
		Ar << VPLClusterData;
		Ar << VPLData;
		Ar << VPLGatherRadius;
		Ar << ConeHalfAngle;
		Ar << BentNormalNormalizeFactor;
		Ar << TileListGroupSize;
		Ar << TileHeadDataUnpacked;
		Ar << TileArrayData;
		return bShaderHasOutdatedParameters;
	}

private:

	FAOParameters AOParameters;
	FRWShaderParameter IrradianceCacheIrradiance;
	FShaderResourceParameter IrradianceCachePositionRadius;
	FShaderResourceParameter IrradianceCacheNormal;
	FShaderResourceParameter IrradianceCacheTileCoordinate;
	FShaderResourceParameter RecordConeData;
	FShaderResourceParameter ScatterDrawParameters;
	FShaderResourceParameter SavedStartIndex;
	FShaderResourceParameter DistanceFieldNormalTexture;
	FShaderResourceParameter DistanceFieldNormalSampler;
	FDistanceFieldObjectBufferParameters GlobalObjectParameters;
	FShaderResourceParameter VPLParameterBuffer;
	FShaderResourceParameter VPLClusterData;
	FShaderResourceParameter VPLData;
	FShaderParameter VPLGatherRadius;
	FShaderParameter ConeHalfAngle;
	FShaderParameter BentNormalNormalizeFactor;
	FShaderParameter TileListGroupSize;
	FShaderResourceParameter TileHeadDataUnpacked;
	FShaderResourceParameter TileArrayData;
};

IMPLEMENT_SHADER_TYPE(template<>,TComputeIrradianceCS<IrradianceMode_IrradianceFromAOCones>,TEXT("DistanceFieldGlobalIllumination"),TEXT("ComputeIrradianceCS"),SF_Compute);
IMPLEMENT_SHADER_TYPE(template<>,TComputeIrradianceCS<IrradianceMode_IrradianceFinalGather>,TEXT("DistanceFieldGlobalIllumination"),TEXT("ComputeIrradianceCS"),SF_Compute);

// Must match usf
const int32 MaxVPLsPerTile = 1024;

void FVPLTileIntersectionResources::InitDynamicRHI()
{
	TileHeadDataUnpacked.Initialize(sizeof(uint32), TileDimensions.X * TileDimensions.Y * 2, PF_R32_UINT, BUF_Static);

	//@todo - handle max exceeded
	TileArrayData.Initialize(sizeof(uint32), MaxVPLsPerTile * TileDimensions.X * TileDimensions.Y, PF_R32_UINT, BUF_Static);
	TileArrayNextAllocation.Initialize(sizeof(uint32), 1, PF_R32_UINT, BUF_Static);
}

TScopedPointer<FLightTileIntersectionResources> GVPLPlacementTileIntersectionResources;

void PlaceVPLs(
	FRHICommandListImmediate& RHICmdList,
	const FViewInfo& View,
	const FScene* Scene,
	const FDistanceFieldAOParameters& Parameters)
{
	GVPLResources.AllocateFor(GVPLGridDimension * GVPLGridDimension);

	uint32 ClearValues[4] = { 0 };
	RHICmdList.ClearUAV(GVPLResources.VPLParameterBuffer.UAV, ClearValues);

	FVPLTileIntersectionResources*& VPLTileIntersectionResources = ((FSceneViewState*)View.State)->VPLTileIntersectionResources;

	{
		uint32 GroupSizeX = (View.ViewRect.Size().X / GAODownsampleFactor + GDistanceFieldAOTileSizeX - 1) / GDistanceFieldAOTileSizeX;
		uint32 GroupSizeY = (View.ViewRect.Size().Y / GAODownsampleFactor + GDistanceFieldAOTileSizeY - 1) / GDistanceFieldAOTileSizeY;

		if (!VPLTileIntersectionResources || VPLTileIntersectionResources->TileDimensions != FIntPoint(GroupSizeX, GroupSizeY))
		{
			if (VPLTileIntersectionResources)
			{
				VPLTileIntersectionResources->ReleaseResource();
			}
			else
			{
				VPLTileIntersectionResources = new FVPLTileIntersectionResources();
			}

			VPLTileIntersectionResources->TileDimensions = FIntPoint(GroupSizeX, GroupSizeY);

			VPLTileIntersectionResources->InitResource();
		}

		// Indicates the clear value for each channel of the UAV format
		uint32 ClearValues[4] = { 0 };
		RHICmdList.ClearUAV(VPLTileIntersectionResources->TileHeadDataUnpacked.UAV, ClearValues);
	}

	const FLightSceneProxy* DirectionalLightProxy = NULL;

	for (TSparseArray<FLightSceneInfoCompact>::TConstIterator LightIt(Scene->Lights); LightIt; ++LightIt)
	{
		const FLightSceneInfoCompact& LightSceneInfoCompact = *LightIt;
		const FLightSceneInfo* const LightSceneInfo = LightSceneInfoCompact.LightSceneInfo;

		if (LightSceneInfo->ShouldRenderLightViewIndependent() 
			&& LightSceneInfo->Proxy->GetLightType() == LightType_Directional
			&& LightSceneInfo->Proxy->CastsDynamicShadow())
		{
			DirectionalLightProxy = LightSceneInfo->Proxy;
			break;
		}
	}

	if (DirectionalLightProxy)
	{
		SCOPED_DRAW_EVENT(RHICmdList, VPLPlacement);
		FMatrix DirectionalLightShadowToWorld;

		{
			int32 NumPlanes = 0;
			const FPlane* PlaneData = NULL;
			FVector4 ShadowBoundingSphereValue(0, 0, 0, 0);
			FShadowCascadeSettings CascadeSettings;
			FSphere ShadowBounds;
			FConvexVolume FrustumVolume;

			static bool bUseShadowmapBounds = true;

			if (bUseShadowmapBounds)
			{
				ShadowBounds = DirectionalLightProxy->GetShadowSplitBoundsDepthRange(
					View, 
					View.NearClippingDistance, 
					GVPLPlacementCameraRadius, 
					&CascadeSettings);

				FSphere SubjectBounds(FVector::ZeroVector, ShadowBounds.W);

				const FMatrix& WorldToLight = DirectionalLightProxy->GetWorldToLight();
				const FMatrix InitializerWorldToLight = FInverseRotationMatrix(FVector(WorldToLight.M[0][0],WorldToLight.M[1][0],WorldToLight.M[2][0]).GetSafeNormal().Rotation());
				const FVector InitializerFaceDirection = FVector(1,0,0);

				FVector	XAxis, YAxis;
				InitializerFaceDirection.FindBestAxisVectors(XAxis, YAxis);
				const FMatrix WorldToLightScaled = InitializerWorldToLight * FScaleMatrix(FVector(1.0f,1.0f / SubjectBounds.W,1.0f / SubjectBounds.W));
				const FMatrix WorldToFace = WorldToLightScaled * FBasisVectorMatrix(-XAxis,YAxis,InitializerFaceDirection.GetSafeNormal(),FVector::ZeroVector);

				static bool bSnapPosition = true;

				if (bSnapPosition)
				{
					// Transform the shadow's position into shadowmap space
					const FVector TransformedPosition = WorldToFace.TransformPosition(ShadowBounds.Center);

					// Determine the distance necessary to snap the shadow's position to the nearest texel
					const float SnapX = FMath::Fmod(TransformedPosition.X, 2.0f / GVPLGridDimension);
					const float SnapY = FMath::Fmod(TransformedPosition.Y, 2.0f / GVPLGridDimension);
					// Snap the shadow's position and transform it back into world space
					// This snapping prevents sub-texel camera movements which removes view dependent aliasing from the final shadow result
					// This only maintains stable shadows under camera translation and rotation
					const FVector SnappedWorldPosition = WorldToFace.InverseFast().TransformPosition(TransformedPosition - FVector(SnapX, SnapY, 0.0f));
					ShadowBounds.Center = SnappedWorldPosition;
				}

				NumPlanes = CascadeSettings.ShadowBoundsAccurate.Planes.Num();
				PlaneData = CascadeSettings.ShadowBoundsAccurate.Planes.GetData();

				DirectionalLightShadowToWorld = FTranslationMatrix(-ShadowBounds.Center) * WorldToFace * FShadowProjectionMatrix(-GVPLDirectionalLightTraceDistance / 2, GVPLDirectionalLightTraceDistance / 2, FVector4(0,0,0,1));
			}
			else
			{
				ShadowBounds = FSphere(View.ViewMatrices.ViewOrigin, GVPLPlacementCameraRadius);

				FSphere SubjectBounds(FVector::ZeroVector, ShadowBounds.W);

				const FMatrix& WorldToLight = DirectionalLightProxy->GetWorldToLight();
				const FMatrix InitializerWorldToLight = FInverseRotationMatrix(FVector(WorldToLight.M[0][0],WorldToLight.M[1][0],WorldToLight.M[2][0]).GetSafeNormal().Rotation());
				const FVector InitializerFaceDirection = FVector(1,0,0);

				FVector	XAxis, YAxis;
				InitializerFaceDirection.FindBestAxisVectors(XAxis, YAxis);
				const FMatrix WorldToLightScaled = InitializerWorldToLight * FScaleMatrix(FVector(1.0f,1.0f / GVPLPlacementCameraRadius,1.0f / GVPLPlacementCameraRadius));
				const FMatrix WorldToFace = WorldToLightScaled * FBasisVectorMatrix(-XAxis,YAxis,InitializerFaceDirection.GetSafeNormal(),FVector::ZeroVector);

				static bool bSnapPosition = true;

				if (bSnapPosition)
				{
					// Transform the shadow's position into shadowmap space
					const FVector TransformedPosition = WorldToFace.TransformPosition(ShadowBounds.Center);

					// Determine the distance necessary to snap the shadow's position to the nearest texel
					const float SnapX = FMath::Fmod(TransformedPosition.X, 2.0f / GVPLGridDimension);
					const float SnapY = FMath::Fmod(TransformedPosition.Y, 2.0f / GVPLGridDimension);
					// Snap the shadow's position and transform it back into world space
					// This snapping prevents sub-texel camera movements which removes view dependent aliasing from the final shadow result
					// This only maintains stable shadows under camera translation and rotation
					const FVector SnappedWorldPosition = WorldToFace.InverseFast().TransformPosition(TransformedPosition - FVector(SnapX, SnapY, 0.0f));
					ShadowBounds.Center = SnappedWorldPosition;
				}

				const float MaxSubjectZ = WorldToFace.TransformPosition(SubjectBounds.Center).Z + SubjectBounds.W;
				const float MinSubjectZ = FMath::Max(MaxSubjectZ - SubjectBounds.W * 2, (float)-HALF_WORLD_MAX);

				DirectionalLightShadowToWorld = FTranslationMatrix(-ShadowBounds.Center) * WorldToFace * FShadowProjectionMatrix(MinSubjectZ, MaxSubjectZ, FVector4(0,0,0,1));

				GetViewFrustumBounds(FrustumVolume, DirectionalLightShadowToWorld, true);

				NumPlanes = FrustumVolume.Planes.Num();
				PlaneData = FrustumVolume.Planes.GetData();
			}

			CullDistanceFieldObjectsForLight(
				RHICmdList,
				View,
				DirectionalLightProxy, 
				DirectionalLightShadowToWorld,
				NumPlanes,
				PlaneData,
				ShadowBoundingSphereValue,
				ShadowBounds.W,
				GVPLPlacementTileIntersectionResources);
		}

		{
			SCOPED_DRAW_EVENT(RHICmdList, PlaceVPLs);

			TShaderMapRef<FVPLPlacementCS> ComputeShader(View.ShaderMap);

			RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
			ComputeShader->SetParameters(RHICmdList, View, DirectionalLightProxy, FVector2D(1.0f / GVPLGridDimension, 1.0f / GVPLGridDimension), DirectionalLightShadowToWorld, DirectionalLightShadowToWorld.InverseFast(), GVPLPlacementTileIntersectionResources);
			DispatchComputeShader(RHICmdList, *ComputeShader, FMath::DivideAndRoundUp<int32>(GVPLGridDimension, GDistanceFieldAOTileSizeX), FMath::DivideAndRoundUp<int32>(GVPLGridDimension, GDistanceFieldAOTileSizeY), 1);

			ComputeShader->UnsetParameters(RHICmdList);
		}
		
		if (GVPLViewCulling)
		{
			{
				TShaderMapRef<FSetupVPLCullndirectArgumentsCS> ComputeShader(View.ShaderMap);
				RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
				ComputeShader->SetParameters(RHICmdList, View);

				DispatchComputeShader(RHICmdList, *ComputeShader, 1, 1, 1);
				ComputeShader->UnsetParameters(RHICmdList);
			}

			{
				GCulledVPLResources.AllocateFor(GVPLGridDimension * GVPLGridDimension);

				uint32 ClearValues[4] = { 0 };
				RHICmdList.ClearUAV(GCulledVPLResources.VPLParameterBuffer.UAV, ClearValues);

				TShaderMapRef<FCullVPLsForViewCS> ComputeShader(GetGlobalShaderMap(Scene->GetFeatureLevel()));
				RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
				ComputeShader->SetParameters(RHICmdList, Scene, View, Parameters);

				DispatchIndirectComputeShader(RHICmdList, *ComputeShader, GVPLResources.VPLDispatchIndirectBuffer.Buffer, 0);
				ComputeShader->UnsetParameters(RHICmdList);
			}
		}
		/*
		{
			SCOPED_DRAW_EVENT(RHICmdList, TileCulling);

			TShaderMapRef<FVPLTileCullCS> ComputeShader(View.ShaderMap);

			RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
			ComputeShader->SetParameters(RHICmdList, View, Scene, Parameters, GVPLViewCulling ? GCulledVPLResources : GVPLResources);
			DispatchComputeShader(RHICmdList, *ComputeShader, VPLTileIntersectionResources->TileDimensions.X, VPLTileIntersectionResources->TileDimensions.Y, 1);

			ComputeShader->UnsetParameters(RHICmdList);
		}*/
	}
}

void ComputeIrradianceForSamples(
	int32 DepthLevel,
	FRHICommandListImmediate& RHICmdList,
	const FViewInfo& View,
	const FScene* Scene,
	const FDistanceFieldAOParameters& Parameters,
	FTemporaryIrradianceCacheResources* TemporaryIrradianceCacheResources)
{
	SCOPED_DRAW_EVENT(RHICmdList, ComputeIrradiance);

	FSurfaceCacheResources& SurfaceCacheResources = *Scene->SurfaceCacheResources;

	if (GAOUseConesForGI)
	{
		TShaderMapRef<TComputeIrradianceCS<IrradianceMode_IrradianceFromAOCones> > ComputeShader(View.ShaderMap);

		RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
		ComputeShader->SetParameters(RHICmdList, View, DepthLevel, Parameters, TemporaryIrradianceCacheResources);
		DispatchIndirectComputeShader(RHICmdList, *ComputeShader, SurfaceCacheResources.DispatchParameters.Buffer, 0);

		ComputeShader->UnsetParameters(RHICmdList);
	}
	else
	{
		TShaderMapRef<TComputeIrradianceCS<IrradianceMode_IrradianceFinalGather> > ComputeShader(View.ShaderMap);

		RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
		ComputeShader->SetParameters(RHICmdList, View, DepthLevel, Parameters, TemporaryIrradianceCacheResources);
		DispatchIndirectComputeShader(RHICmdList, *ComputeShader, SurfaceCacheResources.DispatchParameters.Buffer, 0);

		ComputeShader->UnsetParameters(RHICmdList);
	}
}