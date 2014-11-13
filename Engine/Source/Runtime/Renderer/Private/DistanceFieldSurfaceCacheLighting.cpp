// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DistanceFieldSurfaceCacheLighting.cpp
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "UniformBuffer.h"
#include "ShaderParameters.h"
#include "PostProcessing.h"
#include "SceneFilterRendering.h"
#include "DistanceFieldLightingShared.h"
#include "DistanceFieldSurfaceCacheLighting.h"
#include "PostProcessAmbientOcclusion.h"
#include "RHICommandList.h"
#include "SceneUtils.h"
#include "OneColorShader.h"

int32 GDistanceFieldAO = 1;
FAutoConsoleVariableRef CVarDistanceFieldAO(
	TEXT("r.DistanceFieldAO"),
	GDistanceFieldAO,
	TEXT("Whether the distance field AO feature is allowed, which is used to implement shadows of Movable sky lights from static meshes."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

int32 GAOPowerOfTwoBetweenLevels = 2;
FAutoConsoleVariableRef CVarAOPowerOfTwoBetweenLevels(
	TEXT("r.AOPowerOfTwoBetweenLevels"),
	GAOPowerOfTwoBetweenLevels,
	TEXT("Power of two in resolution between refinement levels of the surface cache"),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

int32 GAOMinLevel = 1;
FAutoConsoleVariableRef CVarAOMinLevel(
	TEXT("r.AOMinLevel"),
	GAOMinLevel,
	TEXT("Smallest downsample power of 4 to use for surface cache population.\n")
	TEXT("The default is 1, which means every 8 full resolution pixels (BaseDownsampleFactor(2) * 4^1) will be checked for a valid interpolation from the cache or shaded.\n")
	TEXT("Going down to 0 gives less aliasing, and removes the need for gap filling, but costs a lot."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

int32 GAOMaxLevel = FMath::Min(4, GAOMaxSupportedLevel);
FAutoConsoleVariableRef CVarAOMaxLevel(
	TEXT("r.AOMaxLevel"),
	GAOMaxLevel,
	TEXT("Largest downsample power of 4 to use for surface cache population."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

int32 GAOReuseAcrossFrames = 1;
FAutoConsoleVariableRef CVarAOReuseAcrossFrames(
	TEXT("r.AOReuseAcrossFrames"),
	GAOReuseAcrossFrames,
	TEXT("Whether to allow reusing surface cache results across frames."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

float GAOTrimOldRecordsFraction = .1f;
FAutoConsoleVariableRef CVarAOTrimOldRecordsFraction(
	TEXT("r.AOTrimOldRecordsFraction"),
	GAOTrimOldRecordsFraction,
	TEXT("When r.AOReuseAcrossFrames is enabled, this is the fraction of the last frame's surface cache records that will not be reused.\n")
	TEXT("Low settings provide better performance, while values closer to 1 give faster lighting updates when dynamic scene changes are happening."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

float GetTrimOldRecordsFraction(int32 DepthLevel)
{
	return GAOTrimOldRecordsFraction;
}

int32 GAOFillGaps = 1;
FAutoConsoleVariableRef CVarAOFillGaps(
	TEXT("r.AOFillGaps"),
	GAOFillGaps,
	TEXT("Whether to fill in pixels using a screen space filter that had no valid world space interpolation weight from surface cache samples.\n")
	TEXT("This is needed whenever r.AOMinLevel is not 0."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

int32 GAOUseHistory = 1;
FAutoConsoleVariableRef CVarAOUseHistory(
	TEXT("r.AOUseHistory"),
	GAOUseHistory,
	TEXT("Whether to apply a temporal filter to the distance field AO, which reduces flickering but also adds trails when occluders are moving."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

float GAOHistoryWeight = .7f;
FAutoConsoleVariableRef CVarAOHistoryWeight(
	TEXT("r.AOHistoryWeight"),
	GAOHistoryWeight,
	TEXT("Amount of last frame's AO to lerp into the final result.  Higher values increase stability, lower values have less streaking under occluder movement."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

float GAORecordRadiusScale = .3f;
FAutoConsoleVariableRef CVarAORecordRadiusScale(
	TEXT("r.AORecordRadiusScale"),
	GAORecordRadiusScale,
	TEXT("Scale applied to the minimum occluder distance to produce the record radius.  This effectively controls how dense shading samples are."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

float GAOInterpolationRadiusScale = 1.3f;
FAutoConsoleVariableRef CVarAOInterpolationRadiusScale(
	TEXT("r.AOInterpolationRadiusScale"),
	GAOInterpolationRadiusScale,
	TEXT("Scale applied to record radii during the final interpolation pass.  Values larger than 1 result in world space smoothing."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

float GAOMinPointBehindPlaneAngle = 4;
FAutoConsoleVariableRef CVarAOMinPointBehindPlaneAngle(
	TEXT("r.AOMinPointBehindPlaneAngle"),
	GAOMinPointBehindPlaneAngle,
	TEXT("Minimum angle that a point can lie behind a record and still be considered valid.\n")
	TEXT("This threshold helps reduce leaking that happens when interpolating records in front of the shading point, ignoring occluders in between."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

float GAOInterpolationMaxAngle = 15;
FAutoConsoleVariableRef CVarAOInterpolationMaxAngle(
	TEXT("r.AOInterpolationMaxAngle"),
	GAOInterpolationMaxAngle,
	TEXT("Largest angle allowed between the shading point's normal and a nearby record's normal."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

float GAOInterpolationAngleScale = 1.5f;
FAutoConsoleVariableRef CVarAOInterpolationAngleScale(
	TEXT("r.AOInterpolationAngleScale"),
	GAOInterpolationAngleScale,
	TEXT("Scale applied to angle error during the final interpolation pass.  Values larger than 1 result in smoothing."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

float GAOStepExponentScale = .5f;
FAutoConsoleVariableRef CVarAOStepExponentScale(
	TEXT("r.AOStepExponentScale"),
	GAOStepExponentScale,
	TEXT("Exponent used to distribute AO samples along a cone direction."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

int32 GAOCPUFrustumCull = 1;
FAutoConsoleVariableRef CVarAOCPUFrustumCull(
	TEXT("r.AOCPUFrustumCull"),
	GAOCPUFrustumCull,
	TEXT("Whether to perform CPU frustum culling of occluder objects."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

float GAOMaxViewDistance = 10000;
FAutoConsoleVariableRef CVarAOMaxViewDistance(
	TEXT("r.AOMaxViewDistance"),
	GAOMaxViewDistance,
	TEXT("The maximum distance that AO will be computed at."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

int32 GAOScatterTileCulling = 1;
FAutoConsoleVariableRef CVarAOScatterTileCulling(
	TEXT("r.AOScatterTileCulling"),
	GAOScatterTileCulling,
	TEXT("Whether to use the rasterizer for binning occluder objects into screenspace tiles."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

int32 GAOScatterInterpolation = 1;
FAutoConsoleVariableRef CVarAOScatterInterpolation(
	TEXT("r.AOScatterInterpolation"),
	GAOScatterInterpolation,
	TEXT(""),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

int32 GAOComputeShaderNormalCalculation = 1;
FAutoConsoleVariableRef CVarAOComputeShaderNormalCalculation(
	TEXT("r.AOComputeShaderNormalCalculation"),
	GAOComputeShaderNormalCalculation,
	TEXT("Whether to use the compute shader version of the distance field normal computation."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

int32 GAOSampleSet = 1;
FAutoConsoleVariableRef CVarAOSampleSet(
	TEXT("r.AOSampleSet"),
	GAOSampleSet,
	TEXT("0 = Original set, 1 = Relaxed set"),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

int32 GAOInterpolationStencilTesting = 1;
FAutoConsoleVariableRef CVarAOInterpolationStencilTesting(
	TEXT("r.AOInterpolationStencilTesting"),
	GAOInterpolationStencilTesting,
	TEXT("Whether to stencil out distant pixels from the interpolation splat pass, useful for debugging"),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

int32 GAOInterpolationDepthTesting = 0;
FAutoConsoleVariableRef CVarAOInterpolationDepthTesting(
	TEXT("r.AOInterpolationDepthTesting"),
	GAOInterpolationDepthTesting,
	TEXT("Whether to use depth testing during the interpolation splat pass, useful for debugging"),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FAOSampleData2,TEXT("AOSamples2"));

FIntPoint GetBufferSizeForAO()
{
	return FIntPoint::DivideAndRoundDown(GSceneRenderTargets.GetBufferSizeXY(), GAODownsampleFactor);
}

bool DoesPlatformSupportDistanceFieldAO(EShaderPlatform Platform)
{
	return Platform == SP_PCD3D_SM5;
}

// Sample set restricted to not self-intersect a surface based on cone angle .475882232
// Coverage of hemisphere = 0.755312979
const FVector SpacedVectors9[] = 
{
	FVector(-0.573257625, 0.625250816, 0.529563010),
	FVector(0.253354192, -0.840093017, 0.479640961),
	FVector(-0.421664953, -0.718063235, 0.553700149),
	FVector(0.249163717, 0.796005428, 0.551627457),
	FVector(0.375082791, 0.295851320, 0.878512800),
	FVector(-0.217619032, 0.00193520682, 0.976031899),
	FVector(-0.852834642, 0.0111727007, 0.522061586),
	FVector(0.745701790, 0.239393353, 0.621787369),
	FVector(-0.151036426, -0.465937436, 0.871831656)
};

// Generated from SpacedVectors9 by applying repulsion forces until convergence
const FVector RelaxedSpacedVectors9[] = 
{
	FVector(-0.467612, 0.739424, 0.484347),
	FVector(0.517459, -0.705440, 0.484346),
	FVector(-0.419848, -0.767551, 0.484347),
	FVector(0.343077, 0.804802, 0.484347),
	FVector(0.364239, 0.244290, 0.898695),
	FVector(-0.381547, 0.185815, 0.905481),
	FVector(-0.870176, -0.090559, 0.484347),
	FVector(0.874448, 0.027390, 0.484346),
	FVector(0.032967, -0.435625, 0.899524)
};

void GetSpacedVectors(TArray<FVector, TInlineAllocator<9> >& OutVectors)
{
	OutVectors.Empty(ARRAY_COUNT(SpacedVectors9));

	if (GAOSampleSet == 0)
	{
		for (int32 i = 0; i < ARRAY_COUNT(SpacedVectors9); i++)
		{
			OutVectors.Add(SpacedVectors9[i]);
		}
	}
	else
	{
		for (int32 i = 0; i < ARRAY_COUNT(RelaxedSpacedVectors9); i++)
		{
			OutVectors.Add(RelaxedSpacedVectors9[i]);
		}
	}
}

// Cone half angle derived from each cone covering an equal solid angle
float GAOConeHalfAngle = FMath::Acos(1 - 1.0f / (float)ARRAY_COUNT(SpacedVectors9));

// Number of AO sample positions along each cone
// Must match shader code
uint32 GAONumConeSteps = 10;

class FCircleVertexBuffer : public FVertexBuffer
{
public:

	int32 NumSections;

	FCircleVertexBuffer()
	{
		NumSections = 8;
	}

	virtual void InitRHI() override
	{
		// Used as a non-indexed triangle list, so 3 vertices per triangle
		const uint32 Size = 3 * NumSections * sizeof(FScreenVertex);
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(Size, BUF_Static, CreateInfo);
		void* Buffer = RHILockVertexBuffer(VertexBufferRHI, 0, Size, RLM_WriteOnly);
		FScreenVertex* DestVertex = (FScreenVertex*)Buffer;

		const float RadiansPerRingSegment = PI / (float)NumSections;

		// Boost the effective radius so that the edges of the circle approximation lie on the circle, instead of the vertices
		const float Radius = 1.0f / FMath::Cos(RadiansPerRingSegment);

		for (int32 SectionIndex = 0; SectionIndex < NumSections; SectionIndex++)
		{
			float Fraction = SectionIndex / (float)NumSections;
			float CurrentAngle = Fraction * 2 * PI;
			float NextAngle = ((SectionIndex + 1) / (float)NumSections) * 2 * PI;
			FVector2D CurrentPosition(Radius * FMath::Cos(CurrentAngle), Radius * FMath::Sin(CurrentAngle));
			FVector2D NextPosition(Radius * FMath::Cos(NextAngle), Radius * FMath::Sin(NextAngle));

			DestVertex[SectionIndex * 3 + 0].Position = FVector2D(0, 0);
			DestVertex[SectionIndex * 3 + 0].UV = CurrentPosition;
			DestVertex[SectionIndex * 3 + 1].Position = FVector2D(0, 0);
			DestVertex[SectionIndex * 3 + 1].UV = NextPosition;
			DestVertex[SectionIndex * 3 + 2].Position = FVector2D(0, 0);
			DestVertex[SectionIndex * 3 + 2].UV = FVector2D(.5f, .5f);
		}

		RHIUnlockVertexBuffer(VertexBufferRHI);      
	}
};

TGlobalResource<FCircleVertexBuffer> GCircleVertexBuffer;

// Only 7 needed atm, pad to cache line size
// Must match equivalent shader defines
int32 FDistanceFieldObjectBuffers::ObjectDataStride = 8;
int32 FDistanceFieldObjectBuffers::ObjectData2Stride = 4;
int32 FDistanceFieldObjectBuffers::ObjectBoxBoundsStride = 5;

class FDistanceFieldObjectBufferResource : public FRenderResource
{
public:
	FDistanceFieldObjectBuffers Buffers;

	virtual void InitDynamicRHI()  override
	{
		Buffers.Initialize();
	}

	virtual void ReleaseDynamicRHI() override
	{
		Buffers.Release();
	}
};

TGlobalResource<FDistanceFieldObjectBufferResource> GAOObjectBuffers;

void FTileIntersectionResources::InitDynamicRHI()
{
	TileConeAxisAndCos.Initialize(sizeof(float)* 4, TileDimensions.X * TileDimensions.Y, PF_A32B32G32R32F, BUF_Static);
	TileConeDepthRanges.Initialize(sizeof(float)* 4, TileDimensions.X * TileDimensions.Y, PF_A32B32G32R32F, BUF_Static);

	TileHeadDataUnpacked.Initialize(sizeof(uint32), TileDimensions.X * TileDimensions.Y * 4, PF_R32_UINT, BUF_Static);
	TileHeadData.Initialize(sizeof(uint32)* 4, TileDimensions.X * TileDimensions.Y, PF_R32G32B32A32_UINT, BUF_Static);

	//@todo - handle max exceeded
	TileArrayData.Initialize(sizeof(uint16), GMaxNumObjectsPerTile * TileDimensions.X * TileDimensions.Y * 3, PF_R16_UINT, BUF_Static);
	TileArrayNextAllocation.Initialize(sizeof(uint32), 1, PF_R32_UINT, BUF_Static);
}

void FSceneViewState::DestroyAOTileResources()
{
	if (AOTileIntersectionResources) 
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			DeleteTileResources,
			FTileIntersectionResources*, AOTileIntersectionResources, AOTileIntersectionResources,
			{
				AOTileIntersectionResources->ReleaseResource();
				delete AOTileIntersectionResources;
			}
		);
	}
	AOTileIntersectionResources = NULL;
}

void OnClearSurfaceCache(UWorld* InWorld)
{
	FlushRenderingCommands();

	FScene* Scene = (FScene*)InWorld->Scene;

	if (Scene && Scene->SurfaceCacheResources)
	{
		Scene->SurfaceCacheResources->bClearedResources = false;
	}
}

FAutoConsoleCommandWithWorld ClearCacheConsoleCommand(
	TEXT("r.AOClearCache"),
	TEXT(""),
	FConsoleCommandWithWorldDelegate::CreateStatic(OnClearSurfaceCache)
	);

/**  */
class FBuildTileConesCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FBuildTileConesCS,Global)
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
		OutEnvironment.SetDefine(TEXT("NUM_CONE_DIRECTIONS"), NumConeSampleDirections);
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);
		OutEnvironment.SetDefine(TEXT("MAX_OBJECTS_PER_TILE"), GMaxNumObjectsPerTile);
		
		// To reduce shader compile time of compute shaders with shared memory, doesn't have an impact on generated code with current compiler (June 2010 DX SDK)
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	FBuildTileConesCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		AOParameters.Bind(Initializer.ParameterMap);
		TileConeAxisAndCos.Bind(Initializer.ParameterMap, TEXT("TileConeAxisAndCos"));
		TileConeDepthRanges.Bind(Initializer.ParameterMap, TEXT("TileConeDepthRanges"));
		TileHeadDataUnpacked.Bind(Initializer.ParameterMap, TEXT("TileHeadDataUnpacked"));
		NumGroups.Bind(Initializer.ParameterMap, TEXT("NumGroups"));
		ViewDimensionsParameter.Bind(Initializer.ParameterMap, TEXT("ViewDimensions"));
	}

	FBuildTileConesCS()
	{
	}
	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, FScene* Scene, FVector2D NumGroupsValue, const FDistanceFieldAOParameters& Parameters)
	{
		FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);
		AOParameters.Set(RHICmdList, ShaderRHI, Parameters);

		FTileIntersectionResources* TileIntersectionResources = ((FSceneViewState*)View.State)->AOTileIntersectionResources;

		TileConeAxisAndCos.SetBuffer(RHICmdList, ShaderRHI, TileIntersectionResources->TileConeAxisAndCos);
		TileConeDepthRanges.SetBuffer(RHICmdList, ShaderRHI, TileIntersectionResources->TileConeDepthRanges);
		TileHeadDataUnpacked.SetBuffer(RHICmdList, ShaderRHI, TileIntersectionResources->TileHeadDataUnpacked);

		SetShaderValue(RHICmdList, ShaderRHI, ViewDimensionsParameter, View.ViewRect);

		SetShaderValue(RHICmdList, ShaderRHI, NumGroups, NumGroupsValue);
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		TileConeAxisAndCos.UnsetUAV(RHICmdList, GetComputeShader());
		TileConeDepthRanges.UnsetUAV(RHICmdList, GetComputeShader());
		TileHeadDataUnpacked.UnsetUAV(RHICmdList, GetComputeShader());
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << AOParameters;
		Ar << TileConeAxisAndCos;
		Ar << TileConeDepthRanges;
		Ar << TileHeadDataUnpacked;
		Ar << NumGroups;
		Ar << ViewDimensionsParameter;
		return bShaderHasOutdatedParameters;
	}

private:

	FDeferredPixelShaderParameters DeferredParameters;
	FAOParameters AOParameters;
	FRWShaderParameter TileConeAxisAndCos;
	FRWShaderParameter TileConeDepthRanges;
	FRWShaderParameter TileHeadDataUnpacked;
	FShaderParameter ViewDimensionsParameter;
	FShaderParameter NumGroups;
};

IMPLEMENT_SHADER_TYPE(,FBuildTileConesCS,TEXT("DistanceFieldSurfaceCacheLightingCompute"),TEXT("BuildTileConesMain"),SF_Compute);


/**  */
class FObjectCullVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FObjectCullVS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) 
	{ 
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform); 
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);
	}

	FObjectCullVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		ObjectParameters.Bind(Initializer.ParameterMap);
		AOParameters.Bind(Initializer.ParameterMap);
		ConservativeRadiusScale.Bind(Initializer.ParameterMap, TEXT("ConservativeRadiusScale"));
	}

	FObjectCullVS() {}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, int32 NumObjects, const FDistanceFieldAOParameters& Parameters)
	{
		const FVertexShaderRHIParamRef ShaderRHI = GetVertexShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		
		ObjectParameters.Set(RHICmdList, ShaderRHI, GAOObjectBuffers.Buffers, NumObjects);
		AOParameters.Set(RHICmdList, ShaderRHI, Parameters);

		const int32 NumRings = StencilingGeometry::GLowPolyStencilSphereVertexBuffer.GetNumRings();
		const float RadiansPerRingSegment = PI / (float)NumRings;

		// Boost the effective radius so that the edges of the sphere approximation lie on the sphere, instead of the vertices
		const float ConservativeRadiusScaleValue = 1.0f / FMath::Cos(RadiansPerRingSegment);

		SetShaderValue(RHICmdList, ShaderRHI, ConservativeRadiusScale, ConservativeRadiusScaleValue);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << ObjectParameters;
		Ar << AOParameters;
		Ar << ConservativeRadiusScale;
		return bShaderHasOutdatedParameters;
	}

private:
	FDistanceFieldObjectBufferParameters ObjectParameters;
	FAOParameters AOParameters;
	FShaderParameter ConservativeRadiusScale;
};

IMPLEMENT_SHADER_TYPE(,FObjectCullVS,TEXT("DistanceFieldSurfaceCacheLighting"),TEXT("ObjectCullVS"),SF_Vertex);

class FObjectCullPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FObjectCullPS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);
		
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);
		OutEnvironment.SetDefine(TEXT("MAX_OBJECTS_PER_TILE"), GMaxNumObjectsPerTile);
	}

	/** Default constructor. */
	FObjectCullPS() {}

	/** Initialization constructor. */
	FObjectCullPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		AOParameters.Bind(Initializer.ParameterMap);
		TileHeadDataUnpacked.Bind(Initializer.ParameterMap, TEXT("TileHeadDataUnpacked"));
		TileArrayData.Bind(Initializer.ParameterMap, TEXT("TileArrayData"));
		TileConeAxisAndCos.Bind(Initializer.ParameterMap, TEXT("TileConeAxisAndCos"));
		TileConeDepthRanges.Bind(Initializer.ParameterMap, TEXT("TileConeDepthRanges"));
		NumGroups.Bind(Initializer.ParameterMap, TEXT("NumGroups"));
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, FVector2D NumGroupsValue, const FDistanceFieldAOParameters& Parameters)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		AOParameters.Set(RHICmdList, ShaderRHI, Parameters);

		FTileIntersectionResources* TileIntersectionResources = ((FSceneViewState*)View.State)->AOTileIntersectionResources;

		SetSRVParameter(RHICmdList, ShaderRHI, TileConeAxisAndCos, TileIntersectionResources->TileConeAxisAndCos.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, TileConeDepthRanges, TileIntersectionResources->TileConeDepthRanges.SRV);

		SetShaderValue(RHICmdList, ShaderRHI, NumGroups, NumGroupsValue);
	}

	void GetUAVs(const FSceneView& View, TArray<FUnorderedAccessViewRHIParamRef>& UAVs)
	{
		FTileIntersectionResources* TileIntersectionResources = ((FSceneViewState*)View.State)->AOTileIntersectionResources;

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
		Ar << AOParameters;
		Ar << TileHeadDataUnpacked;
		Ar << TileArrayData;
		Ar << TileConeAxisAndCos;
		Ar << TileConeDepthRanges;
		Ar << NumGroups;
		return bShaderHasOutdatedParameters;
	}

private:
	FAOParameters AOParameters;
	FRWShaderParameter TileHeadDataUnpacked;
	FRWShaderParameter TileArrayData;
	FShaderResourceParameter TileConeAxisAndCos;
	FShaderResourceParameter TileConeDepthRanges;
	FShaderParameter NumGroups;
};

IMPLEMENT_SHADER_TYPE(,FObjectCullPS,TEXT("DistanceFieldSurfaceCacheLighting"),TEXT("ObjectCullPS"),SF_Pixel);


/**  */
class FDistanceFieldBuildTileListCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDistanceFieldBuildTileListCS,Global)
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
		OutEnvironment.SetDefine(TEXT("NUM_CONE_DIRECTIONS"), NumConeSampleDirections);
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);
		OutEnvironment.SetDefine(TEXT("MAX_OBJECTS_PER_TILE"), GMaxNumObjectsPerTile);
		// To reduce shader compile time of compute shaders with shared memory, doesn't have an impact on generated code with current compiler (June 2010 DX SDK)
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	FDistanceFieldBuildTileListCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		ObjectParameters.Bind(Initializer.ParameterMap);
		AOParameters.Bind(Initializer.ParameterMap);
		TileHeadData.Bind(Initializer.ParameterMap, TEXT("TileHeadData"));
		TileArrayData.Bind(Initializer.ParameterMap, TEXT("TileArrayData"));
		TileArrayNextAllocation.Bind(Initializer.ParameterMap, TEXT("TileArrayNextAllocation"));
		NumGroups.Bind(Initializer.ParameterMap, TEXT("NumGroups"));
		ViewDimensionsParameter.Bind(Initializer.ParameterMap, TEXT("ViewDimensions"));
	}

	FDistanceFieldBuildTileListCS()
	{
	}
	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, FScene* Scene, FVector2D NumGroupsValue, int32 NumObjects, const FDistanceFieldAOParameters& Parameters)
	{
		FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);
		ObjectParameters.Set(RHICmdList, ShaderRHI, GAOObjectBuffers.Buffers, NumObjects);
		AOParameters.Set(RHICmdList, ShaderRHI, Parameters);

		FTileIntersectionResources* TileIntersectionResources = ((FSceneViewState*)View.State)->AOTileIntersectionResources;

		TileHeadData.SetBuffer(RHICmdList, ShaderRHI, TileIntersectionResources->TileHeadData);
		TileArrayData.SetBuffer(RHICmdList, ShaderRHI, TileIntersectionResources->TileArrayData);
		TileArrayNextAllocation.SetBuffer(RHICmdList, ShaderRHI, TileIntersectionResources->TileArrayNextAllocation);

		SetShaderValue(RHICmdList, ShaderRHI, ViewDimensionsParameter, View.ViewRect);

		SetShaderValue(RHICmdList, ShaderRHI, NumGroups, NumGroupsValue);
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		TileHeadData.UnsetUAV(RHICmdList, GetComputeShader());
		TileArrayData.UnsetUAV(RHICmdList, GetComputeShader());
		TileArrayNextAllocation.UnsetUAV(RHICmdList, GetComputeShader());
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << ObjectParameters;
		Ar << AOParameters;
		Ar << TileHeadData;
		Ar << TileArrayData;
		Ar << TileArrayNextAllocation;
		Ar << NumGroups;
		Ar << ViewDimensionsParameter;
		return bShaderHasOutdatedParameters;
	}

private:

	FDeferredPixelShaderParameters DeferredParameters;
	FDistanceFieldObjectBufferParameters ObjectParameters;
	FAOParameters AOParameters;
	FRWShaderParameter TileHeadData;
	FRWShaderParameter TileArrayData;
	FRWShaderParameter TileArrayNextAllocation;
	FShaderParameter ViewDimensionsParameter;
	FShaderParameter NumGroups;
};

IMPLEMENT_SHADER_TYPE(,FDistanceFieldBuildTileListCS,TEXT("DistanceFieldSurfaceCacheLightingCompute"),TEXT("DistanceFieldAOBuildTileListMain"),SF_Compute);

class FAOLevelParameters
{
public:
	void Bind(const FShaderParameterMap& ParameterMap)
	{
		CurrentLevelDownsampleFactor.Bind(ParameterMap, TEXT("CurrentLevelDownsampleFactor"));
		AOBufferSize.Bind(ParameterMap, TEXT("AOBufferSize"));
		DownsampleFactorToBaseLevel.Bind(ParameterMap, TEXT("DownsampleFactorToBaseLevel"));
		BaseLevelTexelSize.Bind(ParameterMap, TEXT("BaseLevelTexelSize"));
	}

	template<typename TParamRef>
	void Set(FRHICommandList& RHICmdList, const TParamRef& ShaderRHI, const FSceneView& View, int32 CurrentLevelDownsampleFactorValue)
	{
		SetShaderValue(RHICmdList, ShaderRHI, CurrentLevelDownsampleFactor, CurrentLevelDownsampleFactorValue);

		// Round up, to match render target allocation
		const FVector2D AOBufferSizeValue = FIntPoint::DivideAndRoundUp(GSceneRenderTargets.GetBufferSizeXY(), CurrentLevelDownsampleFactorValue);
		SetShaderValue(RHICmdList, ShaderRHI, AOBufferSize, AOBufferSizeValue);

		SetShaderValue(RHICmdList, ShaderRHI, DownsampleFactorToBaseLevel, CurrentLevelDownsampleFactorValue / GAODownsampleFactor);

		const FIntPoint DownsampledBufferSize = GetBufferSizeForAO();
		const FVector2D BaseLevelBufferSizeValue(1.0f / DownsampledBufferSize.X, 1.0f / DownsampledBufferSize.Y);
		SetShaderValue(RHICmdList, ShaderRHI, BaseLevelTexelSize, BaseLevelBufferSizeValue);
	}

	friend FArchive& operator<<(FArchive& Ar,FAOLevelParameters& P)
	{
		Ar << P.CurrentLevelDownsampleFactor << P.AOBufferSize << P.DownsampleFactorToBaseLevel << P.BaseLevelTexelSize;
		return Ar;
	}

private:
	FShaderParameter CurrentLevelDownsampleFactor;
	FShaderParameter AOBufferSize;
	FShaderParameter DownsampleFactorToBaseLevel;
	FShaderParameter BaseLevelTexelSize;
};

class FComputeDistanceFieldNormalPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FComputeDistanceFieldNormalPS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), GDistanceFieldAOTileSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), GDistanceFieldAOTileSizeY);
		OutEnvironment.SetDefine(TEXT("MAX_OBJECTS_PER_TILE"), GMaxNumObjectsPerTile);
	}

	/** Default constructor. */
	FComputeDistanceFieldNormalPS() {}

	/** Initialization constructor. */
	FComputeDistanceFieldNormalPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		ObjectParameters.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		TileHeadData.Bind(Initializer.ParameterMap, TEXT("TileHeadData"));
		TileHeadDataUnpacked.Bind(Initializer.ParameterMap, TEXT("TileHeadDataUnpacked"));
		TileArrayData.Bind(Initializer.ParameterMap, TEXT("TileArrayData"));
		TileListGroupSize.Bind(Initializer.ParameterMap, TEXT("TileListGroupSize"));
		AOParameters.Bind(Initializer.ParameterMap);
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, int32 NumObjects, const FDistanceFieldAOParameters& Parameters)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		ObjectParameters.Set(RHICmdList, ShaderRHI, GAOObjectBuffers.Buffers, NumObjects);
		AOParameters.Set(RHICmdList, ShaderRHI, Parameters);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);

		FTileIntersectionResources* TileIntersectionResources = ((FSceneViewState*)View.State)->AOTileIntersectionResources;

		SetSRVParameter(RHICmdList, ShaderRHI, TileHeadData, TileIntersectionResources->TileHeadData.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, TileHeadDataUnpacked, TileIntersectionResources->TileHeadDataUnpacked.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, TileArrayData, TileIntersectionResources->TileArrayData.SRV);

		SetShaderValue(RHICmdList, ShaderRHI, TileListGroupSize, TileIntersectionResources->TileDimensions);
	}
	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << ObjectParameters;
		Ar << DeferredParameters;
		Ar << TileHeadData;
		Ar << TileHeadDataUnpacked;
		Ar << TileArrayData;
		Ar << TileListGroupSize;
		Ar << AOParameters;
		return bShaderHasOutdatedParameters;
	}

private:

	FDistanceFieldObjectBufferParameters ObjectParameters;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderResourceParameter TileHeadData;
	FShaderResourceParameter TileHeadDataUnpacked;
	FShaderResourceParameter TileArrayData;
	FShaderParameter TileListGroupSize;
	FAOParameters AOParameters;
};

IMPLEMENT_SHADER_TYPE(,FComputeDistanceFieldNormalPS,TEXT("DistanceFieldSurfaceCacheLighting"),TEXT("ComputeDistanceFieldNormalPS"),SF_Pixel);


class FComputeDistanceFieldNormalCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FComputeDistanceFieldNormalCS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), GDistanceFieldAOTileSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), GDistanceFieldAOTileSizeY);
		OutEnvironment.SetDefine(TEXT("MAX_OBJECTS_PER_TILE"), GMaxNumObjectsPerTile);
	}

	/** Default constructor. */
	FComputeDistanceFieldNormalCS() {}

	/** Initialization constructor. */
	FComputeDistanceFieldNormalCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DistanceFieldNormal.Bind(Initializer.ParameterMap, TEXT("DistanceFieldNormal"));
		ObjectParameters.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		UVToTileHead.Bind(Initializer.ParameterMap, TEXT("UVToTileHead"));
		TileHeadData.Bind(Initializer.ParameterMap, TEXT("TileHeadData"));
		TileHeadDataUnpacked.Bind(Initializer.ParameterMap, TEXT("TileHeadDataUnpacked"));
		TileArrayData.Bind(Initializer.ParameterMap, TEXT("TileArrayData"));
		TileListGroupSize.Bind(Initializer.ParameterMap, TEXT("TileListGroupSize"));
		AOParameters.Bind(Initializer.ParameterMap);
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, FSceneRenderTargetItem& DistanceFieldNormalValue, int32 NumObjects, const FDistanceFieldAOParameters& Parameters)
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);

		DistanceFieldNormal.SetTexture(RHICmdList, ShaderRHI, DistanceFieldNormalValue.ShaderResourceTexture, DistanceFieldNormalValue.UAV);

		ObjectParameters.Set(RHICmdList, ShaderRHI, GAOObjectBuffers.Buffers, NumObjects);
		AOParameters.Set(RHICmdList, ShaderRHI, Parameters);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);

		SetShaderValue(RHICmdList, ShaderRHI, UVToTileHead, FVector2D(View.ViewRect.Size().X / (float)(GDistanceFieldAOTileSizeX * GAODownsampleFactor), View.ViewRect.Size().Y / (float)(GDistanceFieldAOTileSizeY * GAODownsampleFactor)));
	
		FTileIntersectionResources* TileIntersectionResources = ((FSceneViewState*)View.State)->AOTileIntersectionResources;

		SetSRVParameter(RHICmdList, ShaderRHI, TileHeadData, TileIntersectionResources->TileHeadData.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, TileHeadDataUnpacked, TileIntersectionResources->TileHeadDataUnpacked.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, TileArrayData, TileIntersectionResources->TileArrayData.SRV);

		SetShaderValue(RHICmdList, ShaderRHI, TileListGroupSize, TileIntersectionResources->TileDimensions);
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		DistanceFieldNormal.UnsetUAV(RHICmdList, GetComputeShader());
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DistanceFieldNormal;
		Ar << ObjectParameters;
		Ar << UVToTileHead;
		Ar << DeferredParameters;
		Ar << TileHeadData;
		Ar << TileHeadDataUnpacked;
		Ar << TileArrayData;
		Ar << TileListGroupSize;
		Ar << AOParameters;
		return bShaderHasOutdatedParameters;
	}

private:

	FRWShaderParameter DistanceFieldNormal;
	FDistanceFieldObjectBufferParameters ObjectParameters;
	FShaderParameter UVToTileHead;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderResourceParameter TileHeadData;
	FShaderResourceParameter TileHeadDataUnpacked;
	FShaderResourceParameter TileArrayData;
	FShaderParameter TileListGroupSize;
	FAOParameters AOParameters;
};

IMPLEMENT_SHADER_TYPE(,FComputeDistanceFieldNormalCS,TEXT("DistanceFieldSurfaceCacheLighting"),TEXT("ComputeDistanceFieldNormalCS"),SF_Compute);

void ComputeDistanceFieldNormal(FRHICommandListImmediate& RHICmdList, const TArray<FViewInfo>& Views, FSceneRenderTargetItem& DistanceFieldNormal, int32 NumObjects, const FDistanceFieldAOParameters& Parameters)
{
	if (GAOComputeShaderNormalCalculation)
	{
		SetRenderTarget(RHICmdList, NULL, NULL);

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			const FViewInfo& View = Views[ViewIndex];

			uint32 GroupSizeX = FMath::DivideAndRoundUp(View.ViewRect.Size().X / GAODownsampleFactor, GDistanceFieldAOTileSizeX);
			uint32 GroupSizeY = FMath::DivideAndRoundUp(View.ViewRect.Size().Y / GAODownsampleFactor, GDistanceFieldAOTileSizeY);

			{
				SCOPED_DRAW_EVENT(RHICmdList, ComputeNormalCS);
				TShaderMapRef<FComputeDistanceFieldNormalCS> ComputeShader(View.ShaderMap);

				RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
				ComputeShader->SetParameters(RHICmdList, View, DistanceFieldNormal, NumObjects, Parameters);
				DispatchComputeShader(RHICmdList, *ComputeShader, GroupSizeX, GroupSizeY, 1);

				ComputeShader->UnsetParameters(RHICmdList);
			}
		}
	}
	else
	{
		SetRenderTarget(RHICmdList, DistanceFieldNormal.TargetableTexture, NULL);

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			const FViewInfo& View = Views[ViewIndex];

			SCOPED_DRAW_EVENT(RHICmdList, ComputeNormal);

			RHICmdList.SetViewport(0, 0, 0.0f, View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor, 1.0f);
			RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
			RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
			RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());
			
			TShaderMapRef<FPostProcessVS> VertexShader(View.ShaderMap);
			TShaderMapRef<FComputeDistanceFieldNormalPS> PixelShader(View.ShaderMap);

			static FGlobalBoundShaderState BoundShaderState;
			
			SetGlobalBoundShaderState(RHICmdList, View.FeatureLevel, BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

			PixelShader->SetParameters(RHICmdList, View, NumObjects, Parameters);

			DrawRectangle( 
				RHICmdList,
				0, 0, 
				View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor,
				0, 0, 
				View.ViewRect.Width(), View.ViewRect.Height(),
				FIntPoint(View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor),
				GSceneRenderTargets.GetBufferSizeXY(),
				*VertexShader);
		}
	}
}

class FSetupCopyIndirectArgumentsCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSetupCopyIndirectArgumentsCS,Global)
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	FSetupCopyIndirectArgumentsCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DrawParameters.Bind(Initializer.ParameterMap, TEXT("DrawParameters"));
		DispatchParameters.Bind(Initializer.ParameterMap, TEXT("DispatchParameters"));
		TrimFraction.Bind(Initializer.ParameterMap, TEXT("TrimFraction"));
	}

	FSetupCopyIndirectArgumentsCS()
	{
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, int32 DepthLevel)
	{
		FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);

		const FScene* Scene = (const FScene*)View.Family->Scene;
		FSurfaceCacheResources& SurfaceCacheResources = *Scene->SurfaceCacheResources;

		SetSRVParameter(RHICmdList, ShaderRHI, DrawParameters, SurfaceCacheResources.Level[DepthLevel]->ScatterDrawParameters.SRV);

		DispatchParameters.SetBuffer(RHICmdList, ShaderRHI, SurfaceCacheResources.DispatchParameters);

		SetShaderValue(RHICmdList, ShaderRHI, TrimFraction, GetTrimOldRecordsFraction(DepthLevel));
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		DispatchParameters.UnsetUAV(RHICmdList, GetComputeShader());
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DrawParameters;
		Ar << DispatchParameters;
		Ar << TrimFraction;
		return bShaderHasOutdatedParameters;
	}

private:

	FShaderResourceParameter DrawParameters;
	FRWShaderParameter DispatchParameters;
	FShaderParameter TrimFraction;
};

IMPLEMENT_SHADER_TYPE(,FSetupCopyIndirectArgumentsCS,TEXT("DistanceFieldSurfaceCacheLightingCompute"),TEXT("SetupCopyIndirectArgumentsCS"),SF_Compute);


class FCopyIrradianceCacheSamplesCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCopyIrradianceCacheSamplesCS,Global)
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	FCopyIrradianceCacheSamplesCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		IrradianceCachePositionRadius.Bind(Initializer.ParameterMap, TEXT("IrradianceCachePositionRadius"));
		IrradianceCacheNormal.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheNormal"));
		OccluderRadius.Bind(Initializer.ParameterMap, TEXT("OccluderRadius"));
		IrradianceCacheBentNormal.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheBentNormal"));
		IrradianceCacheTileCoordinate.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheTileCoordinate"));
		DrawParameters.Bind(Initializer.ParameterMap, TEXT("DrawParameters"));
		CopyIrradianceCachePositionRadius.Bind(Initializer.ParameterMap, TEXT("CopyIrradianceCachePositionRadius"));
		CopyIrradianceCacheNormal.Bind(Initializer.ParameterMap, TEXT("CopyIrradianceCacheNormal"));
		CopyOccluderRadius.Bind(Initializer.ParameterMap, TEXT("CopyOccluderRadius"));
		CopyIrradianceCacheBentNormal.Bind(Initializer.ParameterMap, TEXT("CopyIrradianceCacheBentNormal"));
		CopyIrradianceCacheTileCoordinate.Bind(Initializer.ParameterMap, TEXT("CopyIrradianceCacheTileCoordinate"));
		ScatterDrawParameters.Bind(Initializer.ParameterMap, TEXT("ScatterDrawParameters"));
		TrimFraction.Bind(Initializer.ParameterMap, TEXT("TrimFraction"));
	}

	FCopyIrradianceCacheSamplesCS()
	{
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, int32 DepthLevel)
	{
		FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);

		const FScene* Scene = (const FScene*)View.Family->Scene;
		FSurfaceCacheResources& SurfaceCacheResources = *Scene->SurfaceCacheResources;

		SetSRVParameter(RHICmdList, ShaderRHI, IrradianceCachePositionRadius, SurfaceCacheResources.Level[DepthLevel]->PositionAndRadius.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, IrradianceCacheNormal, SurfaceCacheResources.Level[DepthLevel]->Normal.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, OccluderRadius, SurfaceCacheResources.Level[DepthLevel]->OccluderRadius.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, IrradianceCacheBentNormal, SurfaceCacheResources.Level[DepthLevel]->BentNormal.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, IrradianceCacheTileCoordinate, SurfaceCacheResources.Level[DepthLevel]->TileCoordinate.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, DrawParameters, SurfaceCacheResources.Level[DepthLevel]->ScatterDrawParameters.SRV);

		CopyIrradianceCachePositionRadius.SetBuffer(RHICmdList, ShaderRHI, SurfaceCacheResources.TempResources->PositionAndRadius);
		CopyIrradianceCacheNormal.SetBuffer(RHICmdList, ShaderRHI, SurfaceCacheResources.TempResources->Normal);
		CopyOccluderRadius.SetBuffer(RHICmdList, ShaderRHI, SurfaceCacheResources.TempResources->OccluderRadius);
		CopyIrradianceCacheBentNormal.SetBuffer(RHICmdList, ShaderRHI, SurfaceCacheResources.TempResources->BentNormal);
		CopyIrradianceCacheTileCoordinate.SetBuffer(RHICmdList, ShaderRHI, SurfaceCacheResources.TempResources->TileCoordinate);
		ScatterDrawParameters.SetBuffer(RHICmdList, ShaderRHI, SurfaceCacheResources.TempResources->ScatterDrawParameters);

		SetShaderValue(RHICmdList, ShaderRHI, TrimFraction, GetTrimOldRecordsFraction(DepthLevel));
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		CopyIrradianceCachePositionRadius.UnsetUAV(RHICmdList, GetComputeShader());
		CopyIrradianceCacheNormal.UnsetUAV(RHICmdList, GetComputeShader());
		CopyOccluderRadius.UnsetUAV(RHICmdList, GetComputeShader());
		CopyIrradianceCacheBentNormal.UnsetUAV(RHICmdList, GetComputeShader());
		CopyIrradianceCacheTileCoordinate.UnsetUAV(RHICmdList, GetComputeShader());
		ScatterDrawParameters.UnsetUAV(RHICmdList, GetComputeShader());
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << IrradianceCachePositionRadius;
		Ar << IrradianceCacheNormal;
		Ar << OccluderRadius;
		Ar << IrradianceCacheBentNormal;
		Ar << IrradianceCacheTileCoordinate;
		Ar << DrawParameters;
		Ar << CopyIrradianceCachePositionRadius;
		Ar << CopyIrradianceCacheNormal;
		Ar << CopyOccluderRadius;
		Ar << CopyIrradianceCacheBentNormal;
		Ar << CopyIrradianceCacheTileCoordinate;
		Ar << ScatterDrawParameters;
		Ar << TrimFraction;
		return bShaderHasOutdatedParameters;
	}

private:

	FShaderResourceParameter IrradianceCachePositionRadius;
	FShaderResourceParameter IrradianceCacheNormal;
	FShaderResourceParameter OccluderRadius;
	FShaderResourceParameter IrradianceCacheBentNormal;
	FShaderResourceParameter IrradianceCacheTileCoordinate;
	FShaderResourceParameter DrawParameters;
	FRWShaderParameter CopyIrradianceCachePositionRadius;
	FRWShaderParameter CopyIrradianceCacheNormal;
	FRWShaderParameter CopyOccluderRadius;
	FRWShaderParameter CopyIrradianceCacheBentNormal;
	FRWShaderParameter CopyIrradianceCacheTileCoordinate;
	FRWShaderParameter ScatterDrawParameters;
	FShaderParameter TrimFraction;
};

IMPLEMENT_SHADER_TYPE(,FCopyIrradianceCacheSamplesCS,TEXT("DistanceFieldSurfaceCacheLightingCompute"),TEXT("CopyIrradianceCacheSamplesCS"),SF_Compute);


class FSaveStartIndexCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSaveStartIndexCS,Global)
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	FSaveStartIndexCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DrawParameters.Bind(Initializer.ParameterMap, TEXT("DrawParameters"));
		SavedStartIndex.Bind(Initializer.ParameterMap, TEXT("SavedStartIndex"));
	}

	FSaveStartIndexCS()
	{
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, int32 DepthLevel)
	{
		FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);

		const FScene* Scene = (const FScene*)View.Family->Scene;
		FSurfaceCacheResources& SurfaceCacheResources = *Scene->SurfaceCacheResources;

		SetSRVParameter(RHICmdList, ShaderRHI, DrawParameters, SurfaceCacheResources.Level[DepthLevel]->ScatterDrawParameters.SRV);
		SavedStartIndex.SetBuffer(RHICmdList, ShaderRHI, SurfaceCacheResources.Level[DepthLevel]->SavedStartIndex);
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		SavedStartIndex.UnsetUAV(RHICmdList, GetComputeShader());
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DrawParameters;
		Ar << SavedStartIndex;
		return bShaderHasOutdatedParameters;
	}

private:

	FShaderResourceParameter DrawParameters;
	FRWShaderParameter SavedStartIndex;
};

IMPLEMENT_SHADER_TYPE(,FSaveStartIndexCS,TEXT("DistanceFieldSurfaceCacheLightingCompute"),TEXT("SaveStartIndexCS"),SF_Compute);

class FPopulateCacheCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPopulateCacheCS,Global)
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
		OutEnvironment.SetDefine(TEXT("NUM_CONE_DIRECTIONS"), NumConeSampleDirections);
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);

		// To reduce shader compile time of compute shaders with shared memory, doesn't have an impact on generated code with current compiler (June 2010 DX SDK)
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	FPopulateCacheCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		AOParameters.Bind(Initializer.ParameterMap);
		IrradianceCachePositionRadius.Bind(Initializer.ParameterMap, TEXT("IrradianceCachePositionRadius"));
		IrradianceCacheNormal.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheNormal"));
		IrradianceCacheTileCoordinate.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheTileCoordinate"));
		ScatterDrawParameters.Bind(Initializer.ParameterMap, TEXT("ScatterDrawParameters"));
		ViewDimensionsParameter.Bind(Initializer.ParameterMap, TEXT("ViewDimensions"));
		ThreadToCulledTile.Bind(Initializer.ParameterMap, TEXT("ThreadToCulledTile"));
		TileListGroupSize.Bind(Initializer.ParameterMap, TEXT("TileListGroupSize"));
		NumCircleSections.Bind(Initializer.ParameterMap, TEXT("NumCircleSections"));
		DistanceFieldNormalTexture.Bind(Initializer.ParameterMap, TEXT("DistanceFieldNormalTexture"));
		DistanceFieldNormalSampler.Bind(Initializer.ParameterMap, TEXT("DistanceFieldNormalSampler"));
		IrradianceCacheSplatTexture.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheSplatTexture"));
		IrradianceCacheSplatSampler.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheSplatSampler"));
		AOLevelParameters.Bind(Initializer.ParameterMap);
	}

	FPopulateCacheCS()
	{
	}

	void SetParameters(
		FRHICommandList& RHICmdList, 
		const FSceneView& View, 
		FSceneRenderTargetItem& DistanceFieldAOIrradianceCacheSplat, 
		FSceneRenderTargetItem& DistanceFieldNormal, 
		int32 DownsampleFactorValue, 
		int32 DepthLevel, 
		FIntPoint TileListGroupSizeValue, 
		const FDistanceFieldAOParameters& Parameters)
	{
		FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);
		AOParameters.Set(RHICmdList, ShaderRHI, Parameters);
		AOLevelParameters.Set(RHICmdList, ShaderRHI, View, DownsampleFactorValue);

		const FScene* Scene = (const FScene*)View.Family->Scene;
		FSurfaceCacheResources& SurfaceCacheResources = *Scene->SurfaceCacheResources;

		IrradianceCachePositionRadius.SetBuffer(RHICmdList, ShaderRHI, SurfaceCacheResources.Level[DepthLevel]->PositionAndRadius);
		IrradianceCacheNormal.SetBuffer(RHICmdList, ShaderRHI, SurfaceCacheResources.Level[DepthLevel]->Normal);
		IrradianceCacheTileCoordinate.SetBuffer(RHICmdList, ShaderRHI, SurfaceCacheResources.Level[DepthLevel]->TileCoordinate);
		ScatterDrawParameters.SetBuffer(RHICmdList, ShaderRHI, SurfaceCacheResources.Level[DepthLevel]->ScatterDrawParameters);

		SetShaderValue(RHICmdList, ShaderRHI, ViewDimensionsParameter, View.ViewRect);

		FAOSampleData2 AOSampleData;

		TArray<FVector, TInlineAllocator<9> > SampleDirections;
		GetSpacedVectors(SampleDirections);

		for (int32 SampleIndex = 0; SampleIndex < NumConeSampleDirections; SampleIndex++)
		{
			AOSampleData.SampleDirections[SampleIndex] = FVector4(SampleDirections[SampleIndex]);
		}

		SetUniformBufferParameterImmediate(RHICmdList, ShaderRHI, GetUniformBufferParameter<FAOSampleData2>(), AOSampleData);

		FVector2D ThreadToCulledTileValue(DownsampleFactorValue / (float)(GAODownsampleFactor * GDistanceFieldAOTileSizeX), DownsampleFactorValue / (float)(GAODownsampleFactor * GDistanceFieldAOTileSizeY));
		SetShaderValue(RHICmdList, ShaderRHI, ThreadToCulledTile, ThreadToCulledTileValue);

		SetShaderValue(RHICmdList, ShaderRHI, TileListGroupSize, TileListGroupSizeValue);

		SetShaderValue(RHICmdList, ShaderRHI, NumCircleSections, GCircleVertexBuffer.NumSections);

		SetTextureParameter(
			RHICmdList,
			ShaderRHI,
			DistanceFieldNormalTexture,
			DistanceFieldNormalSampler,
			TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			DistanceFieldNormal.ShaderResourceTexture
			);

		SetTextureParameter(
			RHICmdList,
			ShaderRHI,
			IrradianceCacheSplatTexture,
			IrradianceCacheSplatSampler,
			TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			DistanceFieldAOIrradianceCacheSplat.ShaderResourceTexture
			);
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		IrradianceCachePositionRadius.UnsetUAV(RHICmdList, GetComputeShader());
		IrradianceCacheNormal.UnsetUAV(RHICmdList, GetComputeShader());
		IrradianceCacheTileCoordinate.UnsetUAV(RHICmdList, GetComputeShader());
		ScatterDrawParameters.UnsetUAV(RHICmdList, GetComputeShader());
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << AOParameters;
		Ar << AOLevelParameters;
		Ar << IrradianceCachePositionRadius;
		Ar << IrradianceCacheNormal;
		Ar << IrradianceCacheTileCoordinate;
		Ar << ScatterDrawParameters;
		Ar << ViewDimensionsParameter;
		Ar << ThreadToCulledTile;
		Ar << TileListGroupSize;
		Ar << NumCircleSections;
		Ar << DistanceFieldNormalTexture;
		Ar << DistanceFieldNormalSampler;
		Ar << IrradianceCacheSplatTexture;
		Ar << IrradianceCacheSplatSampler;
		return bShaderHasOutdatedParameters;
	}

private:

	FDeferredPixelShaderParameters DeferredParameters;
	FAOParameters AOParameters;
	FAOLevelParameters AOLevelParameters;
	FRWShaderParameter IrradianceCachePositionRadius;
	FRWShaderParameter IrradianceCacheNormal;
	FRWShaderParameter IrradianceCacheTileCoordinate;
	FRWShaderParameter ScatterDrawParameters;
	FShaderParameter ViewDimensionsParameter;
	FShaderParameter ThreadToCulledTile;
	FShaderParameter TileListGroupSize;
	FShaderParameter NumCircleSections;
	FShaderResourceParameter DistanceFieldNormalTexture;
	FShaderResourceParameter DistanceFieldNormalSampler;
	FShaderResourceParameter IrradianceCacheSplatTexture;
	FShaderResourceParameter IrradianceCacheSplatSampler;
};

IMPLEMENT_SHADER_TYPE(,FPopulateCacheCS,TEXT("DistanceFieldSurfaceCacheLightingCompute"),TEXT("PopulateCacheCS"),SF_Compute);

class FSetupFinalGatherIndirectArgumentsCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSetupFinalGatherIndirectArgumentsCS,Global)
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);

		// To reduce shader compile time of compute shaders with shared memory, doesn't have an impact on generated code with current compiler (June 2010 DX SDK)
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	FSetupFinalGatherIndirectArgumentsCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DrawParameters.Bind(Initializer.ParameterMap, TEXT("DrawParameters"));
		DispatchParameters.Bind(Initializer.ParameterMap, TEXT("DispatchParameters"));
		SavedStartIndex.Bind(Initializer.ParameterMap, TEXT("SavedStartIndex"));
	}

	FSetupFinalGatherIndirectArgumentsCS()
	{
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, int32 DepthLevel)
	{
		FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);

		const FScene* Scene = (const FScene*)View.Family->Scene;
		FSurfaceCacheResources& SurfaceCacheResources = *Scene->SurfaceCacheResources;

		SetSRVParameter(RHICmdList, ShaderRHI, DrawParameters, SurfaceCacheResources.Level[DepthLevel]->ScatterDrawParameters.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, SavedStartIndex, SurfaceCacheResources.Level[DepthLevel]->SavedStartIndex.SRV);

		DispatchParameters.SetBuffer(RHICmdList, ShaderRHI, SurfaceCacheResources.DispatchParameters);
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		DispatchParameters.UnsetUAV(RHICmdList, GetComputeShader());
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DrawParameters;
		Ar << SavedStartIndex;
		Ar << DispatchParameters;
		return bShaderHasOutdatedParameters;
	}

private:

	FShaderResourceParameter DrawParameters;
	FShaderResourceParameter SavedStartIndex;
	FRWShaderParameter DispatchParameters;
};

IMPLEMENT_SHADER_TYPE(,FSetupFinalGatherIndirectArgumentsCS,TEXT("DistanceFieldSurfaceCacheLightingCompute"),TEXT("SetupFinalGatherIndirectArgumentsCS"),SF_Compute);

class FFinalGatherCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FFinalGatherCS,Global)
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);
		OutEnvironment.SetDefine(TEXT("NUM_CONE_DIRECTIONS"), NumConeSampleDirections);
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);
		OutEnvironment.SetDefine(TEXT("MAX_OBJECTS_PER_TILE"), GMaxNumObjectsPerTile);

		// To reduce shader compile time of compute shaders with shared memory, doesn't have an impact on generated code with current compiler (June 2010 DX SDK)
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	FFinalGatherCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		ObjectParameters.Bind(Initializer.ParameterMap);
		AOParameters.Bind(Initializer.ParameterMap);
		IrradianceCachePositionRadius.Bind(Initializer.ParameterMap, TEXT("IrradianceCachePositionRadius"));
		IrradianceCacheNormal.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheNormal"));
		OccluderRadius.Bind(Initializer.ParameterMap, TEXT("OccluderRadius"));
		IrradianceCacheBentNormal.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheBentNormal"));
		IrradianceCacheTileCoordinate.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheTileCoordinate"));
		ScatterDrawParameters.Bind(Initializer.ParameterMap, TEXT("ScatterDrawParameters"));
		SavedStartIndex.Bind(Initializer.ParameterMap, TEXT("SavedStartIndex"));
		ViewDimensionsParameter.Bind(Initializer.ParameterMap, TEXT("ViewDimensions"));
		ThreadToCulledTile.Bind(Initializer.ParameterMap, TEXT("ThreadToCulledTile"));
		TileHeadData.Bind(Initializer.ParameterMap, TEXT("TileHeadData"));
		TileHeadDataUnpacked.Bind(Initializer.ParameterMap, TEXT("TileHeadDataUnpacked"));
		TileArrayData.Bind(Initializer.ParameterMap, TEXT("TileArrayData"));
		TileListGroupSize.Bind(Initializer.ParameterMap, TEXT("TileListGroupSize"));
		TanConeHalfAngle.Bind(Initializer.ParameterMap, TEXT("TanConeHalfAngle"));
		BentNormalNormalizeFactor.Bind(Initializer.ParameterMap, TEXT("BentNormalNormalizeFactor"));
		DistanceFieldAtlasTexelSize.Bind(Initializer.ParameterMap, TEXT("DistanceFieldAtlasTexelSize"));
		DistanceFieldNormalTexture.Bind(Initializer.ParameterMap, TEXT("DistanceFieldNormalTexture"));
		DistanceFieldNormalSampler.Bind(Initializer.ParameterMap, TEXT("DistanceFieldNormalSampler"));
		IrradianceCacheSplatTexture.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheSplatTexture"));
		IrradianceCacheSplatSampler.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheSplatSampler"));
		AOLevelParameters.Bind(Initializer.ParameterMap);
		RecordRadiusScale.Bind(Initializer.ParameterMap, TEXT("RecordRadiusScale"));
	}

	FFinalGatherCS()
	{
	}

	void SetParameters(
		FRHICommandList& RHICmdList, 
		const FSceneView& View, 
		FSceneRenderTargetItem& DistanceFieldAOIrradianceCacheSplat, 
		FSceneRenderTargetItem& DistanceFieldNormal, 
		int32 NumObjects, 
		int32 DownsampleFactorValue, 
		int32 DepthLevel, 
		FIntPoint TileListGroupSizeValue, 
		const FDistanceFieldAOParameters& Parameters)
	{
		FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);
		ObjectParameters.Set(RHICmdList, ShaderRHI, GAOObjectBuffers.Buffers, NumObjects);
		AOParameters.Set(RHICmdList, ShaderRHI, Parameters);
		AOLevelParameters.Set(RHICmdList, ShaderRHI, View, DownsampleFactorValue);

		const FScene* Scene = (const FScene*)View.Family->Scene;
		FSurfaceCacheResources& SurfaceCacheResources = *Scene->SurfaceCacheResources;

		SetSRVParameter(RHICmdList, ShaderRHI, IrradianceCacheNormal, SurfaceCacheResources.Level[DepthLevel]->Normal.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, ScatterDrawParameters, SurfaceCacheResources.Level[DepthLevel]->ScatterDrawParameters.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, SavedStartIndex, SurfaceCacheResources.Level[DepthLevel]->SavedStartIndex.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, IrradianceCachePositionRadius, SurfaceCacheResources.Level[DepthLevel]->PositionAndRadius.SRV);

		OccluderRadius.SetBuffer(RHICmdList, ShaderRHI, SurfaceCacheResources.Level[DepthLevel]->OccluderRadius);
		IrradianceCacheBentNormal.SetBuffer(RHICmdList, ShaderRHI, SurfaceCacheResources.Level[DepthLevel]->BentNormal);

		SetShaderValue(RHICmdList, ShaderRHI, ViewDimensionsParameter, View.ViewRect);

		FAOSampleData2 AOSampleData;

		TArray<FVector, TInlineAllocator<9> > SampleDirections;
		GetSpacedVectors(SampleDirections);

		for (int32 SampleIndex = 0; SampleIndex < NumConeSampleDirections; SampleIndex++)
		{
			AOSampleData.SampleDirections[SampleIndex] = FVector4(SampleDirections[SampleIndex]);
		}

		SetUniformBufferParameterImmediate(RHICmdList, ShaderRHI, GetUniformBufferParameter<FAOSampleData2>(), AOSampleData);

		FVector2D ThreadToCulledTileValue(DownsampleFactorValue / (float)(GAODownsampleFactor * GDistanceFieldAOTileSizeX), DownsampleFactorValue / (float)(GAODownsampleFactor * GDistanceFieldAOTileSizeY));
		SetShaderValue(RHICmdList, ShaderRHI, ThreadToCulledTile, ThreadToCulledTileValue);

		FTileIntersectionResources* TileIntersectionResources = ((FSceneViewState*)View.State)->AOTileIntersectionResources;

		SetSRVParameter(RHICmdList, ShaderRHI, TileHeadDataUnpacked, TileIntersectionResources->TileHeadDataUnpacked.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, TileHeadData, TileIntersectionResources->TileHeadData.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, TileArrayData, TileIntersectionResources->TileArrayData.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, IrradianceCacheTileCoordinate, SurfaceCacheResources.Level[DepthLevel]->TileCoordinate.SRV);

		SetShaderValue(RHICmdList, ShaderRHI, TileListGroupSize, TileListGroupSizeValue);

		SetShaderValue(RHICmdList, ShaderRHI, TanConeHalfAngle, FMath::Tan(GAOConeHalfAngle));

		FVector UnoccludedVector(0);

		for (int32 SampleIndex = 0; SampleIndex < NumConeSampleDirections; SampleIndex++)
		{
			UnoccludedVector += SampleDirections[SampleIndex];
		}

		float BentNormalNormalizeFactorValue = 1.0f / (UnoccludedVector / NumConeSampleDirections).Size();
		SetShaderValue(RHICmdList, ShaderRHI, BentNormalNormalizeFactor, BentNormalNormalizeFactorValue);

		const int32 NumTexelsOneDimX = GDistanceFieldVolumeTextureAtlas.GetSizeX();
		const int32 NumTexelsOneDimY = GDistanceFieldVolumeTextureAtlas.GetSizeY();
		const int32 NumTexelsOneDimZ = GDistanceFieldVolumeTextureAtlas.GetSizeZ();
		const FVector InvTextureDim(1.0f / NumTexelsOneDimX, 1.0f / NumTexelsOneDimY, 1.0f / NumTexelsOneDimZ);
		SetShaderValue(RHICmdList, ShaderRHI, DistanceFieldAtlasTexelSize, InvTextureDim);

		SetTextureParameter(
			RHICmdList,
			ShaderRHI,
			DistanceFieldNormalTexture,
			DistanceFieldNormalSampler,
			TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			DistanceFieldNormal.ShaderResourceTexture
			);

		SetTextureParameter(
			RHICmdList,
			ShaderRHI,
			IrradianceCacheSplatTexture,
			IrradianceCacheSplatSampler,
			TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			DistanceFieldAOIrradianceCacheSplat.ShaderResourceTexture
			);

		SetShaderValue(RHICmdList, ShaderRHI, RecordRadiusScale, GAORecordRadiusScale);
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		OccluderRadius.UnsetUAV(RHICmdList, GetComputeShader());
		IrradianceCacheBentNormal.UnsetUAV(RHICmdList, GetComputeShader());
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << ObjectParameters;
		Ar << AOParameters;
		Ar << AOLevelParameters;
		Ar << IrradianceCachePositionRadius;
		Ar << IrradianceCacheNormal;
		Ar << OccluderRadius;
		Ar << IrradianceCacheBentNormal;
		Ar << IrradianceCacheTileCoordinate;
		Ar << ScatterDrawParameters;
		Ar << SavedStartIndex;
		Ar << ViewDimensionsParameter;
		Ar << ThreadToCulledTile;
		Ar << TileHeadDataUnpacked;
		Ar << TileHeadData;
		Ar << TileArrayData;
		Ar << TileListGroupSize;
		Ar << TanConeHalfAngle;
		Ar << BentNormalNormalizeFactor;
		Ar << DistanceFieldAtlasTexelSize;
		Ar << DistanceFieldNormalTexture;
		Ar << DistanceFieldNormalSampler;
		Ar << IrradianceCacheSplatTexture;
		Ar << IrradianceCacheSplatSampler;
		Ar << RecordRadiusScale;
		return bShaderHasOutdatedParameters;
	}

private:

	FDeferredPixelShaderParameters DeferredParameters;
	FDistanceFieldObjectBufferParameters ObjectParameters;
	FAOParameters AOParameters;
	FAOLevelParameters AOLevelParameters;
	FShaderResourceParameter IrradianceCacheNormal;
	FShaderResourceParameter IrradianceCachePositionRadius;
	FRWShaderParameter OccluderRadius;
	FRWShaderParameter IrradianceCacheBentNormal;
	FShaderResourceParameter ScatterDrawParameters;
	FShaderResourceParameter SavedStartIndex;
	FShaderResourceParameter IrradianceCacheTileCoordinate;
	FShaderParameter ViewDimensionsParameter;
	FShaderParameter ThreadToCulledTile;
	FShaderResourceParameter TileHeadDataUnpacked;
	FShaderResourceParameter TileHeadData;
	FShaderResourceParameter TileArrayData;
	FShaderParameter TileListGroupSize;
	FShaderParameter TanConeHalfAngle;
	FShaderParameter BentNormalNormalizeFactor;
	FShaderParameter DistanceFieldAtlasTexelSize;
	FShaderResourceParameter DistanceFieldNormalTexture;
	FShaderResourceParameter DistanceFieldNormalSampler;
	FShaderResourceParameter IrradianceCacheSplatTexture;
	FShaderResourceParameter IrradianceCacheSplatSampler;
	FShaderParameter RecordRadiusScale;
};

IMPLEMENT_SHADER_TYPE(,FFinalGatherCS,TEXT("DistanceFieldSurfaceCacheLightingCompute"),TEXT("FinalGatherCS"),SF_Compute);

class FWriteDownsampledDepthPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FWriteDownsampledDepthPS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);
	}

	/** Default constructor. */
	FWriteDownsampledDepthPS() {}

	/** Initialization constructor. */
	FWriteDownsampledDepthPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		DistanceFieldNormalTexture.Bind(Initializer.ParameterMap,TEXT("DistanceFieldNormalTexture"));
		DistanceFieldNormalSampler.Bind(Initializer.ParameterMap,TEXT("DistanceFieldNormalSampler"));
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, FSceneRenderTargetItem& DistanceFieldNormal)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);

		SetTextureParameter(RHICmdList, ShaderRHI, DistanceFieldNormalTexture, DistanceFieldNormalSampler, TStaticSamplerState<SF_Point>::GetRHI(), DistanceFieldNormal.ShaderResourceTexture);
	}
	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << DistanceFieldNormalTexture;
		Ar << DistanceFieldNormalSampler;
		return bShaderHasOutdatedParameters;
	}

private:

	FDeferredPixelShaderParameters DeferredParameters;
	FShaderResourceParameter DistanceFieldNormalTexture;
	FShaderResourceParameter DistanceFieldNormalSampler;
};

IMPLEMENT_SHADER_TYPE(,FWriteDownsampledDepthPS,TEXT("DistanceFieldSurfaceCacheLighting"),TEXT("WriteDownsampledDepthPS"),SF_Pixel);


/**  */
template<bool bFinalInterpolationPass>
class TIrradianceCacheSplatVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(TIrradianceCacheSplatVS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) 
	{ 
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);

		OutEnvironment.SetDefine(TEXT("FINAL_INTERPOLATION_PASS"), (uint32)bFinalInterpolationPass);
	}

	TIrradianceCacheSplatVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		IrradianceCachePositionRadius.Bind(Initializer.ParameterMap, TEXT("IrradianceCachePositionRadius"));
		IrradianceCacheOccluderRadius.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheOccluderRadius"));
		IrradianceCacheNormal.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheNormal"));
		IrradianceCacheBentNormal.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheBentNormal"));
		InterpolationRadiusScale.Bind(Initializer.ParameterMap, TEXT("InterpolationRadiusScale"));
		NormalizedOffsetToPixelCenter.Bind(Initializer.ParameterMap, TEXT("NormalizedOffsetToPixelCenter"));
		HackExpand.Bind(Initializer.ParameterMap, TEXT("HackExpand"));
	}

	TIrradianceCacheSplatVS() {}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, int32 DepthLevel, int32 CurrentLevelDownsampleFactorValue, FVector2D NormalizedOffsetToPixelCenterValue)
	{
		const FVertexShaderRHIParamRef ShaderRHI = GetVertexShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		
		const FScene* Scene = (const FScene*)View.Family->Scene;
		FSurfaceCacheResources& SurfaceCacheResources = *Scene->SurfaceCacheResources;

		SetSRVParameter(RHICmdList, ShaderRHI, IrradianceCachePositionRadius, SurfaceCacheResources.Level[DepthLevel]->PositionAndRadius.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, IrradianceCacheOccluderRadius, SurfaceCacheResources.Level[DepthLevel]->OccluderRadius.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, IrradianceCacheNormal, SurfaceCacheResources.Level[DepthLevel]->Normal.SRV);
		SetSRVParameter(RHICmdList, ShaderRHI, IrradianceCacheBentNormal, SurfaceCacheResources.Level[DepthLevel]->BentNormal.SRV);

		SetShaderValue(RHICmdList, ShaderRHI, InterpolationRadiusScale, (bFinalInterpolationPass ? GAOInterpolationRadiusScale : 1.0f));
		SetShaderValue(RHICmdList, ShaderRHI, NormalizedOffsetToPixelCenter, NormalizedOffsetToPixelCenterValue);

		const FIntPoint AOViewRectSize = FIntPoint::DivideAndRoundUp(View.ViewRect.Size(), CurrentLevelDownsampleFactorValue);
		const FVector2D HackExpandValue(.5f / AOViewRectSize.X, .5f / AOViewRectSize.Y);
		SetShaderValue(RHICmdList, ShaderRHI, HackExpand, HackExpandValue);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << IrradianceCachePositionRadius;
		Ar << IrradianceCacheOccluderRadius;
		Ar << IrradianceCacheNormal;
		Ar << IrradianceCacheBentNormal;
		Ar << InterpolationRadiusScale;
		Ar << NormalizedOffsetToPixelCenter;
		Ar << HackExpand;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderResourceParameter IrradianceCachePositionRadius;
	FShaderResourceParameter IrradianceCacheOccluderRadius;
	FShaderResourceParameter IrradianceCacheNormal;
	FShaderResourceParameter IrradianceCacheBentNormal;
	FShaderParameter InterpolationRadiusScale;
	FShaderParameter NormalizedOffsetToPixelCenter;
	FShaderParameter HackExpand;
};

IMPLEMENT_SHADER_TYPE(template<>,TIrradianceCacheSplatVS<true>,TEXT("DistanceFieldSurfaceCacheLighting"),TEXT("IrradianceCacheSplatVS"),SF_Vertex);
IMPLEMENT_SHADER_TYPE(template<>,TIrradianceCacheSplatVS<false>,TEXT("DistanceFieldSurfaceCacheLighting"),TEXT("IrradianceCacheSplatVS"),SF_Vertex);

template<bool bFinalInterpolationPass>
class TIrradianceCacheSplatPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(TIrradianceCacheSplatPS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);
		
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);
		OutEnvironment.SetDefine(TEXT("FINAL_INTERPOLATION_PASS"), (uint32)bFinalInterpolationPass);
	}

	/** Default constructor. */
	TIrradianceCacheSplatPS() {}

	/** Initialization constructor. */
	TIrradianceCacheSplatPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		AOParameters.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		AOLevelParameters.Bind(Initializer.ParameterMap);
		DistanceFieldNormalTexture.Bind(Initializer.ParameterMap, TEXT("DistanceFieldNormalTexture"));
		DistanceFieldNormalSampler.Bind(Initializer.ParameterMap, TEXT("DistanceFieldNormalSampler"));
		InterpolationAngleNormalization.Bind(Initializer.ParameterMap, TEXT("InterpolationAngleNormalization"));
		InvMinCosPointBehindPlane.Bind(Initializer.ParameterMap, TEXT("InvMinCosPointBehindPlane"));
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, FSceneRenderTargetItem& DistanceFieldNormal, int32 DestLevelDownsampleFactor, const FDistanceFieldAOParameters& Parameters)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		AOParameters.Set(RHICmdList, ShaderRHI, Parameters);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);
		AOLevelParameters.Set(RHICmdList, ShaderRHI, View, DestLevelDownsampleFactor);

		SetTextureParameter(
			RHICmdList,
			ShaderRHI,
			DistanceFieldNormalTexture,
			DistanceFieldNormalSampler,
			TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			DistanceFieldNormal.ShaderResourceTexture
			);

		const float EffectiveMaxAngle = bFinalInterpolationPass ? GAOInterpolationMaxAngle * GAOInterpolationAngleScale : GAOInterpolationMaxAngle;
		const float InterpolationAngleNormalizationValue = 1.0f / FMath::Sqrt(1.0f - FMath::Cos(EffectiveMaxAngle * PI / 180.0f));
		SetShaderValue(RHICmdList, ShaderRHI, InterpolationAngleNormalization, InterpolationAngleNormalizationValue);

		const float MinCosPointBehindPlaneValue = FMath::Cos((GAOMinPointBehindPlaneAngle + 90.0f) * PI / 180.0f);
		SetShaderValue(RHICmdList, ShaderRHI, InvMinCosPointBehindPlane, 1.0f / MinCosPointBehindPlaneValue);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << AOParameters;
		Ar << AOLevelParameters;
		Ar << DeferredParameters;
		Ar << DistanceFieldNormalTexture;
		Ar << DistanceFieldNormalSampler;
		Ar << InterpolationAngleNormalization;
		Ar << InvMinCosPointBehindPlane;
		return bShaderHasOutdatedParameters;
	}

private:
	FAOParameters AOParameters;
	FAOLevelParameters AOLevelParameters;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderResourceParameter DistanceFieldNormalTexture;
	FShaderResourceParameter DistanceFieldNormalSampler;
	FShaderParameter InterpolationAngleNormalization;
	FShaderParameter InvMinCosPointBehindPlane;
};

IMPLEMENT_SHADER_TYPE(template<>,TIrradianceCacheSplatPS<true>,TEXT("DistanceFieldSurfaceCacheLighting"),TEXT("IrradianceCacheSplatPS"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>,TIrradianceCacheSplatPS<false>,TEXT("DistanceFieldSurfaceCacheLighting"),TEXT("IrradianceCacheSplatPS"),SF_Pixel);


template<uint32 NumLevels, bool bFinalInterpolationPass>
class TInterpolateCacheCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(TInterpolateCacheCS,Global)
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);

		OutEnvironment.SetDefine(TEXT("FINAL_INTERPOLATION_PASS"), (uint32)bFinalInterpolationPass);
		OutEnvironment.SetDefine(TEXT("NUM_INTERPOLATE_LEVELS"), NumLevels);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), GDistanceFieldAOTileSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), GDistanceFieldAOTileSizeY);
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	TInterpolateCacheCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		AOParameters.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		AOLevelParameters.Bind(Initializer.ParameterMap);
		DistanceFieldNormalTexture.Bind(Initializer.ParameterMap, TEXT("DistanceFieldNormalTexture"));
		DistanceFieldNormalSampler.Bind(Initializer.ParameterMap, TEXT("DistanceFieldNormalSampler"));
		InterpolationAngleNormalization.Bind(Initializer.ParameterMap, TEXT("InterpolationAngleNormalization"));
		InvMinCosPointBehindPlane.Bind(Initializer.ParameterMap, TEXT("InvMinCosPointBehindPlane"));
		IrradianceCacheSplat.Bind(Initializer.ParameterMap, TEXT("IrradianceCacheSplat"));
		IrradianceCachePositionRadius.Bind(Initializer.ParameterMap, TEXT("LevelIrradianceCachePositionRadius"));
		IrradianceCacheOccluderRadius.Bind(Initializer.ParameterMap, TEXT("LevelIrradianceCacheOccluderRadius"));
		IrradianceCacheNormal.Bind(Initializer.ParameterMap, TEXT("LevelIrradianceCacheNormal"));
		IrradianceCacheBentNormal.Bind(Initializer.ParameterMap, TEXT("LevelIrradianceCacheBentNormal"));
		InterpolationRadiusScale.Bind(Initializer.ParameterMap, TEXT("InterpolationRadiusScale"));
		DrawParameters.Bind(Initializer.ParameterMap, TEXT("LevelScatterDrawParameters"));
		NumGroups.Bind(Initializer.ParameterMap, TEXT("NumGroups"));
	}

	TInterpolateCacheCS()
	{
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, FSceneRenderTargetItem& DistanceFieldNormal, int32 DepthLevel, int32 DestLevelDownsampleFactor, IPooledRenderTarget* InterpolationTarget, const FDistanceFieldAOParameters& Parameters, FVector2D NumGroupsValue, FSurfaceCacheResources& SurfaceCacheResources)
	{
		FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		AOParameters.Set(RHICmdList, ShaderRHI, Parameters);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);
		AOLevelParameters.Set(RHICmdList, ShaderRHI, View, DestLevelDownsampleFactor);

		SetTextureParameter(
			RHICmdList,
			ShaderRHI,
			DistanceFieldNormalTexture,
			DistanceFieldNormalSampler,
			TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			DistanceFieldNormal.ShaderResourceTexture
			);

		const float EffectiveMaxAngle = bFinalInterpolationPass ? GAOInterpolationMaxAngle * GAOInterpolationAngleScale : GAOInterpolationMaxAngle;
		const float InterpolationAngleNormalizationValue = 1.0f / FMath::Sqrt(1.0f - FMath::Cos(EffectiveMaxAngle * PI / 180.0f));
		SetShaderValue(RHICmdList, ShaderRHI, InterpolationAngleNormalization, InterpolationAngleNormalizationValue);

		const float MinCosPointBehindPlaneValue = FMath::Cos((GAOMinPointBehindPlaneAngle + 90.0f) * PI / 180.0f);
		SetShaderValue(RHICmdList, ShaderRHI, InvMinCosPointBehindPlane, 1.0f / MinCosPointBehindPlaneValue);

		IrradianceCacheSplat.SetTexture(
			RHICmdList,
			ShaderRHI, 
			InterpolationTarget->GetRenderTargetItem().TargetableTexture, 
			InterpolationTarget->GetRenderTargetItem().UAV);

		const int32 MaxAllowedLevel = ARRAY_COUNT(SurfaceCacheResources.Level) - 1;
		const int32 MinLevel = MaxAllowedLevel + 1 - NumLevels;

		for (int32 LevelIndex = MaxAllowedLevel; LevelIndex >= MinLevel; LevelIndex--)
		{
			if (IrradianceCachePositionRadius.IsBound())
			{
				check(MaxAllowedLevel - LevelIndex < (int32)IrradianceCachePositionRadius.GetNumResources());

				RHICmdList.SetShaderResourceViewParameter(
					ShaderRHI,
					IrradianceCachePositionRadius.GetBaseIndex() + MaxAllowedLevel - LevelIndex,
					SurfaceCacheResources.Level[LevelIndex]->PositionAndRadius.SRV
					);
			}

			if (IrradianceCacheOccluderRadius.IsBound())
			{
				check(MaxAllowedLevel - LevelIndex < (int32)IrradianceCacheOccluderRadius.GetNumResources());

				RHICmdList.SetShaderResourceViewParameter(
					ShaderRHI,
					IrradianceCacheOccluderRadius.GetBaseIndex() + MaxAllowedLevel - LevelIndex,
					SurfaceCacheResources.Level[LevelIndex]->OccluderRadius.SRV
					);
			}

			if (IrradianceCacheNormal.IsBound())
			{
				check(MaxAllowedLevel - LevelIndex < (int32)IrradianceCacheNormal.GetNumResources());

				RHICmdList.SetShaderResourceViewParameter(
					ShaderRHI,
					IrradianceCacheNormal.GetBaseIndex() + MaxAllowedLevel - LevelIndex,
					SurfaceCacheResources.Level[LevelIndex]->Normal.SRV
					);
			}

			if (IrradianceCacheBentNormal.IsBound())
			{
				check(MaxAllowedLevel - LevelIndex < (int32)IrradianceCacheBentNormal.GetNumResources());

				RHICmdList.SetShaderResourceViewParameter(
					ShaderRHI,
					IrradianceCacheBentNormal.GetBaseIndex() + MaxAllowedLevel - LevelIndex,
					SurfaceCacheResources.Level[LevelIndex]->BentNormal.SRV
					);
			}

			if (DrawParameters.IsBound())
			{
				check(MaxAllowedLevel - LevelIndex < (int32)DrawParameters.GetNumResources());

				RHICmdList.SetShaderResourceViewParameter(
					ShaderRHI,
					DrawParameters.GetBaseIndex() + MaxAllowedLevel - LevelIndex,
					SurfaceCacheResources.Level[LevelIndex]->ScatterDrawParameters.SRV
					);
			}
		}

		SetShaderValue(RHICmdList, ShaderRHI, InterpolationRadiusScale, (bFinalInterpolationPass ? GAOInterpolationRadiusScale : 1.0f));

		SetShaderValue(RHICmdList, ShaderRHI, NumGroups, NumGroupsValue);
	}

	void UnsetParameters(FRHICommandListImmediate& RHICmdList)
	{
		IrradianceCacheSplat.UnsetUAV(RHICmdList, GetComputeShader());
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << AOParameters;
		Ar << AOLevelParameters;
		Ar << DeferredParameters;
		Ar << DistanceFieldNormalTexture;
		Ar << DistanceFieldNormalSampler;
		Ar << InterpolationAngleNormalization;
		Ar << InvMinCosPointBehindPlane;
		Ar << IrradianceCacheSplat;
		Ar << IrradianceCachePositionRadius;
		Ar << IrradianceCacheOccluderRadius;
		Ar << IrradianceCacheNormal;
		Ar << IrradianceCacheBentNormal;
		Ar << InterpolationRadiusScale;
		Ar << DrawParameters;
		Ar << NumGroups;
		return bShaderHasOutdatedParameters;
	}

private:

	FAOParameters AOParameters;
	FAOLevelParameters AOLevelParameters;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderResourceParameter DistanceFieldNormalTexture;
	FShaderResourceParameter DistanceFieldNormalSampler;
	FShaderParameter InterpolationAngleNormalization;
	FShaderParameter InvMinCosPointBehindPlane;
	FRWShaderParameter IrradianceCacheSplat;
	FShaderResourceParameter IrradianceCachePositionRadius;
	FShaderResourceParameter IrradianceCacheOccluderRadius;
	FShaderResourceParameter IrradianceCacheNormal;
	FShaderResourceParameter IrradianceCacheBentNormal;
	FShaderParameter InterpolationRadiusScale;
	FShaderResourceParameter DrawParameters;
	FShaderParameter NumGroups;
};

#define IMPLEMENT_INTERPOLATION_TYPE(NumLevels,bFinalInterpolationPass) \
	typedef TInterpolateCacheCS<NumLevels,bFinalInterpolationPass> TInterpolateCacheCS##NumLevels##bFinalInterpolationPass; \
	IMPLEMENT_SHADER_TYPE(template<>,TInterpolateCacheCS##NumLevels##bFinalInterpolationPass,TEXT("DistanceFieldSurfaceCacheLighting"),TEXT("InterpolateCacheCS"),SF_Compute);

IMPLEMENT_INTERPOLATION_TYPE(5, true);
IMPLEMENT_INTERPOLATION_TYPE(4, true);
IMPLEMENT_INTERPOLATION_TYPE(1, false);
IMPLEMENT_INTERPOLATION_TYPE(2, false);
IMPLEMENT_INTERPOLATION_TYPE(3, false);
IMPLEMENT_INTERPOLATION_TYPE(4, false);
IMPLEMENT_INTERPOLATION_TYPE(5, false);


class FDistanceFieldAOCombinePS2 : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDistanceFieldAOCombinePS2, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);
	}

	/** Default constructor. */
	FDistanceFieldAOCombinePS2() {}

	/** Initialization constructor. */
	FDistanceFieldAOCombinePS2(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		AOParameters.Bind(Initializer.ParameterMap);
		AOLevelParameters.Bind(Initializer.ParameterMap);
		BentNormalAOTexture3.Bind(Initializer.ParameterMap,TEXT("BentNormalAOTexture3"));
		BentNormalAOSampler3.Bind(Initializer.ParameterMap,TEXT("BentNormalAOSampler3"));
		DistanceFieldNormalTexture.Bind(Initializer.ParameterMap, TEXT("DistanceFieldNormalTexture"));
		DistanceFieldNormalSampler.Bind(Initializer.ParameterMap, TEXT("DistanceFieldNormalSampler"));
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, FSceneRenderTargetItem& InBentNormalTexture, FSceneRenderTargetItem& DistanceFieldNormal, const FDistanceFieldAOParameters& Parameters)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);
		AOParameters.Set(RHICmdList, ShaderRHI, Parameters);
		AOLevelParameters.Set(RHICmdList, ShaderRHI, View, GAODownsampleFactor);

		SetTextureParameter(RHICmdList, ShaderRHI, BentNormalAOTexture3, BentNormalAOSampler3, TStaticSamplerState<SF_Point>::GetRHI(), InBentNormalTexture.ShaderResourceTexture);

		SetTextureParameter(
			RHICmdList,
			ShaderRHI,
			DistanceFieldNormalTexture,
			DistanceFieldNormalSampler,
			TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			DistanceFieldNormal.ShaderResourceTexture
			);
	}
	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << AOParameters;
		Ar << AOLevelParameters;
		Ar << BentNormalAOTexture3;
		Ar << BentNormalAOSampler3;
		Ar << DistanceFieldNormalTexture;
		Ar << DistanceFieldNormalSampler;
		return bShaderHasOutdatedParameters;
	}

private:

	FDeferredPixelShaderParameters DeferredParameters;
	FAOParameters AOParameters;
	FAOLevelParameters AOLevelParameters;
	FShaderResourceParameter BentNormalAOTexture3;
	FShaderResourceParameter BentNormalAOSampler3;
	FShaderResourceParameter DistanceFieldNormalTexture;
	FShaderResourceParameter DistanceFieldNormalSampler;
};

IMPLEMENT_SHADER_TYPE(,FDistanceFieldAOCombinePS2,TEXT("DistanceFieldSurfaceCacheLighting"),TEXT("AOCombinePS2"),SF_Pixel);

class FFillGapsPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FFillGapsPS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);
	}

	/** Default constructor. */
	FFillGapsPS() {}

	/** Initialization constructor. */
	FFillGapsPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		BentNormalAOTexture.Bind(Initializer.ParameterMap, TEXT("BentNormalAOTexture"));
		BentNormalAOSampler.Bind(Initializer.ParameterMap, TEXT("BentNormalAOSampler"));
		BentNormalAOTexelSize.Bind(Initializer.ParameterMap, TEXT("BentNormalAOTexelSize"));
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, FSceneRenderTargetItem& DistanceFieldAOBentNormal2)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);

		SetTextureParameter(
			RHICmdList,
			ShaderRHI,
			BentNormalAOTexture,
			BentNormalAOSampler,
			TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			DistanceFieldAOBentNormal2.ShaderResourceTexture
			);

		const FIntPoint DownsampledBufferSize(GSceneRenderTargets.GetBufferSizeXY() / FIntPoint(GAODownsampleFactor, GAODownsampleFactor));
		const FVector2D BaseLevelTexelSizeValue(1.0f / DownsampledBufferSize.X, 1.0f / DownsampledBufferSize.Y);
		SetShaderValue(RHICmdList, ShaderRHI, BentNormalAOTexelSize, BaseLevelTexelSizeValue);
	}
	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << BentNormalAOTexture;
		Ar << BentNormalAOSampler;
		Ar << BentNormalAOTexelSize;
		return bShaderHasOutdatedParameters;
	}

private:

	FShaderResourceParameter BentNormalAOTexture;
	FShaderResourceParameter BentNormalAOSampler;
	FShaderParameter BentNormalAOTexelSize;
};

IMPLEMENT_SHADER_TYPE(,FFillGapsPS,TEXT("DistanceFieldSurfaceCacheLighting"),TEXT("FillGapsPS"),SF_Pixel);


class FUpdateHistoryPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FUpdateHistoryPS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);
	}

	/** Default constructor. */
	FUpdateHistoryPS() {}

	/** Initialization constructor. */
	FUpdateHistoryPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		BentNormalAOTexture.Bind(Initializer.ParameterMap, TEXT("BentNormalAOTexture"));
		BentNormalAOSampler.Bind(Initializer.ParameterMap, TEXT("BentNormalAOSampler"));
		HistoryTexture.Bind(Initializer.ParameterMap, TEXT("HistoryTexture"));
		HistorySampler.Bind(Initializer.ParameterMap, TEXT("HistorySampler"));
		CameraMotion.Bind(Initializer.ParameterMap);
		HistoryWeight.Bind(Initializer.ParameterMap, TEXT("HistoryWeight"));
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, FSceneRenderTargetItem& HistoryTextureValue, FSceneRenderTargetItem& DistanceFieldAOBentNormal)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);

		SetTextureParameter(
			RHICmdList,
			ShaderRHI,
			BentNormalAOTexture,
			BentNormalAOSampler,
			TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			DistanceFieldAOBentNormal.ShaderResourceTexture
			);

		SetTextureParameter(
			RHICmdList,
			ShaderRHI,
			HistoryTexture,
			HistorySampler,
			TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			HistoryTextureValue.ShaderResourceTexture
			);

		CameraMotion.Set(RHICmdList, View, ShaderRHI);

		SetShaderValue(RHICmdList, ShaderRHI, HistoryWeight, GAOHistoryWeight);
	}
	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << BentNormalAOTexture;
		Ar << BentNormalAOSampler;
		Ar << HistoryTexture;
		Ar << HistorySampler;
		Ar << CameraMotion;
		Ar << HistoryWeight;
		return bShaderHasOutdatedParameters;
	}

private:

	FDeferredPixelShaderParameters DeferredParameters;
	FShaderResourceParameter BentNormalAOTexture;
	FShaderResourceParameter BentNormalAOSampler;
	FShaderResourceParameter HistoryTexture;
	FShaderResourceParameter HistorySampler;
	FCameraMotionParameters CameraMotion;
	FShaderParameter HistoryWeight;
};

IMPLEMENT_SHADER_TYPE(,FUpdateHistoryPS,TEXT("DistanceFieldSurfaceCacheLighting"),TEXT("UpdateHistoryPS"),SF_Pixel);

void AllocateOrReuseAORenderTarget(TRefCountPtr<IPooledRenderTarget>& Target, const TCHAR* Name)
{
	if (!Target)
	{
		FIntPoint BufferSize = GetBufferSizeForAO();

		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, PF_FloatRGBA, TexCreate_None, TexCreate_RenderTargetable | TexCreate_UAV, false));
		GRenderTargetPool.FindFreeElement(Desc, Target, Name);
	}
}

void UpdateHistory(
	FRHICommandList& RHICmdList,
	FViewInfo& View, 
	const TCHAR* HistoryRTName,
	/** Contains last frame's history, if non-NULL.  This will be updated with the new frame's history. */
	TRefCountPtr<IPooledRenderTarget>* HistoryState,
	/** Source */
	TRefCountPtr<IPooledRenderTarget>& AOSource, 
	/** Output of Temporal Reprojection for the next step in the pipeline. */
	TRefCountPtr<IPooledRenderTarget>& HistoryOutput)
{
	if (HistoryState)
	{
		if (*HistoryState && !View.bCameraCut && !View.bPrevTransformsReset)
		{
			// Reuse a render target from the pool with a consistent name, for vis purposes
			TRefCountPtr<IPooledRenderTarget> NewHistory;
			AllocateOrReuseAORenderTarget(NewHistory, HistoryRTName);

			{
				SCOPED_DRAW_EVENT(RHICmdList, UpdateHistory);
				SetRenderTarget(RHICmdList, NewHistory->GetRenderTargetItem().TargetableTexture, NULL);

				RHICmdList.SetViewport(0, 0, 0.0f, View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor, 1.0f);
				RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
				RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
				RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());

				TShaderMapRef<FPostProcessVS> VertexShader(View.ShaderMap);
				TShaderMapRef<FUpdateHistoryPS> PixelShader(View.ShaderMap);

				static FGlobalBoundShaderState BoundShaderState;
				
				SetGlobalBoundShaderState(RHICmdList, View.GetFeatureLevel(), BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

				PixelShader->SetParameters(RHICmdList, View, (*HistoryState)->GetRenderTargetItem(), AOSource->GetRenderTargetItem());

				DrawRectangle( 
					RHICmdList,
					0, 0, 
					View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor,
					View.ViewRect.Min.X / GAODownsampleFactor, View.ViewRect.Min.Y / GAODownsampleFactor,
					View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor,
					FIntPoint(View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor),
					GSceneRenderTargets.GetBufferSizeXY() / FIntPoint(GAODownsampleFactor, GAODownsampleFactor),
					*VertexShader);
			}

			// Update the view state's render target reference with the new history
			*HistoryState = NewHistory;
			HistoryOutput = NewHistory;
		}
		else
		{
			// Use the current frame's mask for next frame's history
			*HistoryState = AOSource;
			HistoryOutput = AOSource;
			AOSource = NULL;
		}
	}
	else
	{
		// Temporal reprojection is disabled or there is no view state - pass through
		HistoryOutput = AOSource;
	}
}

void PostProcessBentNormalAO(
	FRHICommandList& RHICmdList, 
	const FDistanceFieldAOParameters& Parameters, 
	TArray<FViewInfo>& Views, 
	FSceneRenderTargetItem& IrradianceCacheInterpolation, 
	FSceneRenderTargetItem& DistanceFieldNormal,
	TRefCountPtr<IPooledRenderTarget>& AOOutput)
{
	TRefCountPtr<IPooledRenderTarget> DistanceFieldAOBentNormal;
	AllocateOrReuseAORenderTarget(DistanceFieldAOBentNormal, TEXT("DistanceFieldBentNormalAO"));

	TRefCountPtr<IPooledRenderTarget> DistanceFieldAOBentNormal2;

	if (GAOFillGaps)
	{
		AllocateOrReuseAORenderTarget(DistanceFieldAOBentNormal2, TEXT("DistanceFieldBentNormalAO2"));
	}

	{
		SCOPED_DRAW_EVENT(RHICmdList, AOCombine);

		SetRenderTarget(RHICmdList, GAOFillGaps
			? DistanceFieldAOBentNormal2->GetRenderTargetItem().TargetableTexture
			: DistanceFieldAOBentNormal->GetRenderTargetItem().TargetableTexture, 
			NULL);

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			FViewInfo& View = Views[ViewIndex];

			RHICmdList.SetViewport(0, 0, 0.0f, View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor, 1.0f);
			RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
			RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
			RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());

			TShaderMapRef<FPostProcessVS> VertexShader(View.ShaderMap);
			TShaderMapRef<FDistanceFieldAOCombinePS2> PixelShader(View.ShaderMap);

			static FGlobalBoundShaderState BoundShaderState;
			
			SetGlobalBoundShaderState(RHICmdList, View.GetFeatureLevel(), BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

			PixelShader->SetParameters(RHICmdList, View, IrradianceCacheInterpolation, DistanceFieldNormal, Parameters);

			DrawRectangle( 
				RHICmdList,
				0, 0, 
				View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor,
				0, 0, 
				View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor,
				FIntPoint(View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor),
				GetBufferSizeForAO(),
				*VertexShader);
		}
	}

	if (GAOFillGaps)
	{
		SCOPED_DRAW_EVENT(RHICmdList, FillGaps);
		SetRenderTarget(RHICmdList, DistanceFieldAOBentNormal->GetRenderTargetItem().TargetableTexture, NULL);

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			const FViewInfo& View = Views[ViewIndex];

			RHICmdList.SetViewport(0, 0, 0.0f, View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor, 1.0f);
			RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
			RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
			RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());

			TShaderMapRef<FPostProcessVS> VertexShader(View.ShaderMap);
			TShaderMapRef<FFillGapsPS> PixelShader(View.ShaderMap);

			static FGlobalBoundShaderState BoundShaderState;
			
			SetGlobalBoundShaderState(RHICmdList, View.GetFeatureLevel(), BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

			VertexShader->SetParameters(RHICmdList, View);
			PixelShader->SetParameters(RHICmdList, View, DistanceFieldAOBentNormal2->GetRenderTargetItem());

			DrawRectangle( 
				RHICmdList,
				0, 0, 
				View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor,
				0, 0, 
				View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor,
				FIntPoint(View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor),
				GSceneRenderTargets.GetBufferSizeXY() / FIntPoint(GAODownsampleFactor, GAODownsampleFactor),
				*VertexShader);
		}
	}

	FSceneViewState* ViewState = (FSceneViewState*)Views[0].State;
	TRefCountPtr<IPooledRenderTarget>* HistoryState = ViewState ? &ViewState->DistanceFieldAOHistoryRT : NULL;
	AOOutput = DistanceFieldAOBentNormal;

	if (GAOUseHistory)
	{
		FViewInfo& View = Views[0];

		UpdateHistory(
			RHICmdList,
			View, 
			TEXT("DistanceFieldAOHistory"),
			HistoryState,
			DistanceFieldAOBentNormal, 
			AOOutput);
	}
}

template<bool bOutputBentNormal>
class TDistanceFieldAOUpsamplePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(TDistanceFieldAOUpsamplePS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);
		OutEnvironment.SetDefine(TEXT("OUTPUT_BENT_NORMAL"), bOutputBentNormal ? TEXT("1") : TEXT("0"));
	}

	/** Default constructor. */
	TDistanceFieldAOUpsamplePS() {}

	/** Initialization constructor. */
	TDistanceFieldAOUpsamplePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		BentNormalAOTexture.Bind(Initializer.ParameterMap,TEXT("BentNormalAOTexture"));
		BentNormalAOSampler.Bind(Initializer.ParameterMap,TEXT("BentNormalAOSampler"));
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, TRefCountPtr<IPooledRenderTarget>& DistanceFieldAOBentNormal)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);

		SetTextureParameter(RHICmdList, ShaderRHI, BentNormalAOTexture, BentNormalAOSampler, TStaticSamplerState<SF_Bilinear>::GetRHI(), DistanceFieldAOBentNormal->GetRenderTargetItem().ShaderResourceTexture);
	}
	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << BentNormalAOTexture;
		Ar << BentNormalAOSampler;
		return bShaderHasOutdatedParameters;
	}

private:

	FDeferredPixelShaderParameters DeferredParameters;
	FShaderResourceParameter BentNormalAOTexture;
	FShaderResourceParameter BentNormalAOSampler;
};

IMPLEMENT_SHADER_TYPE(template<>,TDistanceFieldAOUpsamplePS<true>,TEXT("DistanceFieldSurfaceCacheLighting"),TEXT("AOUpsamplePS"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>,TDistanceFieldAOUpsamplePS<false>,TEXT("DistanceFieldSurfaceCacheLighting"),TEXT("AOUpsamplePS"),SF_Pixel);

void UpsampleBentNormalAO(FRHICommandList& RHICmdList, const TArray<FViewInfo>& Views, TRefCountPtr<IPooledRenderTarget>& DistanceFieldAOBentNormal, bool bOutputBentNormal)
{
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		const FViewInfo& View = Views[ViewIndex];

		SCOPED_DRAW_EVENT(RHICmdList, UpsampleAO);

		RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);
		RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
		RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
		RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());

		TShaderMapRef<FPostProcessVS> VertexShader(View.ShaderMap);

		if (bOutputBentNormal)
		{
			TShaderMapRef<TDistanceFieldAOUpsamplePS<true> > PixelShader(View.ShaderMap);
			static FGlobalBoundShaderState BoundShaderState;
			SetGlobalBoundShaderState(RHICmdList, View.GetFeatureLevel(), BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);
			PixelShader->SetParameters(RHICmdList, View, DistanceFieldAOBentNormal);
		}
		else
		{
			TShaderMapRef<TDistanceFieldAOUpsamplePS<false> > PixelShader(View.ShaderMap);
			static FGlobalBoundShaderState BoundShaderState;
			SetGlobalBoundShaderState(RHICmdList, View.GetFeatureLevel(), BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);
			PixelShader->SetParameters(RHICmdList, View, DistanceFieldAOBentNormal);
		}

		DrawRectangle( 
			RHICmdList,
			0, 0, 
			View.ViewRect.Width(), View.ViewRect.Height(),
			View.ViewRect.Min.X / GAODownsampleFactor, View.ViewRect.Min.Y / GAODownsampleFactor, 
			View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor,
			FIntPoint(View.ViewRect.Width(), View.ViewRect.Height()),
			GetBufferSizeForAO(),
			*VertexShader);
	}
}

/** Generates a pseudo-random position inside the unit sphere, uniformly distributed over the volume of the sphere. */
FVector GetUnitPosition2(FRandomStream& RandomStream)
{
	FVector Result;
	// Use rejection sampling to generate a valid sample
	do
	{
		Result.X = RandomStream.GetFraction() * 2 - 1;
		Result.Y = RandomStream.GetFraction() * 2 - 1;
		Result.Z = RandomStream.GetFraction() * 2 - 1;
	} while( Result.SizeSquared() > 1.f );
	return Result;
}

/** Generates a pseudo-random unit vector, uniformly distributed over all directions. */
FVector GetUnitVector2(FRandomStream& RandomStream)
{
	return GetUnitPosition2(RandomStream).UnsafeNormal();
}

void GenerateBestSpacedVectors()
{
	static bool bGenerated = false;
	bool bApplyRepulsion = false;

	if (bApplyRepulsion && !bGenerated)
	{
		bGenerated = true;

		FVector OriginalSpacedVectors9[ARRAY_COUNT(SpacedVectors9)];

		for (int32 i = 0; i < ARRAY_COUNT(OriginalSpacedVectors9); i++)
		{
			OriginalSpacedVectors9[i] = SpacedVectors9[i];
		}

		float CosHalfAngle = 1 - 1.0f / (float)ARRAY_COUNT(OriginalSpacedVectors9);
		// Used to prevent self-shadowing on a plane
		float AngleBias = .03f;
		float MinAngle = FMath::Acos(CosHalfAngle) + AngleBias;
		float MinZ = FMath::Sin(MinAngle);

		// Relaxation iterations by repulsion
		for (int32 Iteration = 0; Iteration < 10000; Iteration++)
		{
			for (int32 i = 0; i < ARRAY_COUNT(OriginalSpacedVectors9); i++)
			{
				FVector Force(0.0f, 0.0f, 0.0f);

				for (int32 j = 0; j < ARRAY_COUNT(OriginalSpacedVectors9); j++)
				{
					if (i != j)
					{
						FVector Distance = OriginalSpacedVectors9[i] - OriginalSpacedVectors9[j];
						float Dot = OriginalSpacedVectors9[i] | OriginalSpacedVectors9[j];

						if (Dot > 0)
						{
							// Repulsion force
							Force += .001f * Distance.SafeNormal() * Dot * Dot * Dot * Dot;
						}
					}
				}

				FVector NewPosition = OriginalSpacedVectors9[i] + Force;
				NewPosition.Z = FMath::Max(NewPosition.Z, MinZ);
				NewPosition = NewPosition.SafeNormal();
				OriginalSpacedVectors9[i] = NewPosition;
			}
		}

		for (int32 i = 0; i < ARRAY_COUNT(OriginalSpacedVectors9); i++)
		{
			UE_LOG(LogTemp, Log, TEXT("FVector(%f, %f, %f),"), OriginalSpacedVectors9[i].X, OriginalSpacedVectors9[i].Y, OriginalSpacedVectors9[i].Z);
		}

		int32 temp = 0;
	}

	bool bBruteForceGenerateConeDirections = false;

	if (bBruteForceGenerateConeDirections)
	{
		FVector BestSpacedVectors9[9];
		float BestCoverage = 0;
		// Each cone covers an area of ConeSolidAngle = HemisphereSolidAngle / NumCones
		// HemisphereSolidAngle = 2 * PI
		// ConeSolidAngle = 2 * PI * (1 - cos(ConeHalfAngle))
		// cos(ConeHalfAngle) = 1 - 1 / NumCones
		float CosHalfAngle = 1 - 1.0f / (float)ARRAY_COUNT(BestSpacedVectors9);
		// Prevent self-intersection in sample set
		float MinAngle = FMath::Acos(CosHalfAngle);
		float MinZ = FMath::Sin(MinAngle);
		FRandomStream RandomStream(123567);

		// Super slow random brute force search
		for (int i = 0; i < 1000000; i++)
		{
			FVector CandidateSpacedVectors[ARRAY_COUNT(BestSpacedVectors9)];

			for (int j = 0; j < ARRAY_COUNT(CandidateSpacedVectors); j++)
			{
				FVector NewSample;

				// Reject invalid directions until we get a valid one
				do 
				{
					NewSample = GetUnitVector2(RandomStream);
				} 
				while (NewSample.Z <= MinZ);

				CandidateSpacedVectors[j] = NewSample;
			}

			float Coverage = 0;
			int NumSamples = 10000;

			// Determine total cone coverage with monte carlo estimation
			for (int sample = 0; sample < NumSamples; sample++)
			{
				FVector NewSample;

				do 
				{
					NewSample = GetUnitVector2(RandomStream);
				} 
				while (NewSample.Z <= 0);

				bool bIntersects = false;

				for (int j = 0; j < ARRAY_COUNT(CandidateSpacedVectors); j++)
				{
					if (FVector::DotProduct(CandidateSpacedVectors[j], NewSample) > CosHalfAngle)
					{
						bIntersects = true;
						break;
					}
				}

				Coverage += bIntersects ? 1 / (float)NumSamples : 0;
			}

			if (Coverage > BestCoverage)
			{
				BestCoverage = Coverage;

				for (int j = 0; j < ARRAY_COUNT(CandidateSpacedVectors); j++)
				{
					BestSpacedVectors9[j] = CandidateSpacedVectors[j];
				}
			}
		}

		int32 temp = 0;
	}
}

FIntPoint BuildTileObjectLists(FRHICommandListImmediate& RHICmdList, FScene* Scene, TArray<FViewInfo>& Views, int32 NumObjects, const FDistanceFieldAOParameters& Parameters)
{
	SCOPED_DRAW_EVENT(RHICmdList, BuildTileList);
	SetRenderTarget(RHICmdList, NULL, NULL);

	FIntPoint TileListGroupSize;

	if (GAOScatterTileCulling)
	{

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			const FViewInfo& View = Views[ViewIndex];

			uint32 GroupSizeX = FMath::DivideAndRoundUp(View.ViewRect.Size().X / GAODownsampleFactor, GDistanceFieldAOTileSizeX);
			uint32 GroupSizeY = FMath::DivideAndRoundUp(View.ViewRect.Size().Y / GAODownsampleFactor, GDistanceFieldAOTileSizeY);
			TileListGroupSize = FIntPoint(GroupSizeX, GroupSizeY);

			FTileIntersectionResources*& TileIntersectionResources = ((FSceneViewState*)View.State)->AOTileIntersectionResources;

			if (!TileIntersectionResources || TileIntersectionResources->TileDimensions != TileListGroupSize)
			{
				if (TileIntersectionResources)
				{
					TileIntersectionResources->ReleaseResource();
				}
				else
				{
					TileIntersectionResources = new FTileIntersectionResources();
				}

				TileIntersectionResources->TileDimensions = TileListGroupSize;

				TileIntersectionResources->InitResource();
			}

			{
				SCOPED_DRAW_EVENT(RHICmdList, BuildTileCones);
				TShaderMapRef<FBuildTileConesCS> ComputeShader(View.ShaderMap);

				RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
				ComputeShader->SetParameters(RHICmdList, View, Scene, FVector2D(GroupSizeX, GroupSizeY), Parameters);
				DispatchComputeShader(RHICmdList, *ComputeShader, GroupSizeX, GroupSizeY, 1);

				ComputeShader->UnsetParameters(RHICmdList);
			}

			{
				SCOPED_DRAW_EVENT(RHICmdList, CullObjects);

				TShaderMapRef<FObjectCullVS> VertexShader(View.ShaderMap);
				TShaderMapRef<FObjectCullPS> PixelShader(View.ShaderMap);

				TArray<FUnorderedAccessViewRHIParamRef> UAVs;
				PixelShader->GetUAVs(Views[0], UAVs);
				RHICmdList.SetRenderTargets(0, (const FRHIRenderTargetView *)NULL, NULL, UAVs.Num(), UAVs.GetData());

				RHICmdList.SetViewport(0, 0, 0.0f, GroupSizeX, GroupSizeY, 1.0f);

				// Render backfaces since camera may intersect
				RHICmdList.SetRasterizerState(View.bReverseCulling ? TStaticRasterizerState<FM_Solid, CM_CW>::GetRHI() : TStaticRasterizerState<FM_Solid, CM_CCW>::GetRHI());
				RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
				RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());

				static FGlobalBoundShaderState BoundShaderState;
				
				SetGlobalBoundShaderState(RHICmdList, View.GetFeatureLevel(), BoundShaderState, GetVertexDeclarationFVector4(), *VertexShader, *PixelShader);

				VertexShader->SetParameters(RHICmdList, View, NumObjects, Parameters);
				PixelShader->SetParameters(RHICmdList, View, FVector2D(GroupSizeX, GroupSizeY), Parameters);

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
	}
	else
	{
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			const FViewInfo& View = Views[ViewIndex];

			uint32 GroupSizeX = (View.ViewRect.Size().X / GAODownsampleFactor + GDistanceFieldAOTileSizeX - 1) / GDistanceFieldAOTileSizeX;
			uint32 GroupSizeY = (View.ViewRect.Size().Y / GAODownsampleFactor + GDistanceFieldAOTileSizeY - 1) / GDistanceFieldAOTileSizeY;
			TileListGroupSize = FIntPoint(GroupSizeX, GroupSizeY);

			FTileIntersectionResources*& TileIntersectionResources = ((FSceneViewState*)View.State)->AOTileIntersectionResources;

			if (!TileIntersectionResources || TileIntersectionResources->TileDimensions != TileListGroupSize)
			{
				if (TileIntersectionResources)
				{
					TileIntersectionResources->ReleaseResource();
				}
				else
				{
					TileIntersectionResources = new FTileIntersectionResources();
				}

				TileIntersectionResources->TileDimensions = TileListGroupSize;

				TileIntersectionResources->InitResource();
			}

			// Indicates the clear value for each channel of the UAV format
			uint32 ClearValues[4] = { 0 };
			RHICmdList.ClearUAV(TileIntersectionResources->TileArrayNextAllocation.UAV, ClearValues);
			RHICmdList.ClearUAV(TileIntersectionResources->TileHeadData.UAV, ClearValues);

			TShaderMapRef<FDistanceFieldBuildTileListCS > ComputeShader(View.ShaderMap);

			RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
			ComputeShader->SetParameters(RHICmdList, View, Scene, FVector2D(GroupSizeX, GroupSizeY), NumObjects, Parameters);
			DispatchComputeShader(RHICmdList, *ComputeShader, GroupSizeX, GroupSizeY, 1);

			ComputeShader->UnsetParameters(RHICmdList);
		}
	}

	return TileListGroupSize;
}

void SetupDepthStencil(FRHICommandListImmediate& RHICmdList, FViewInfo& View, FSceneRenderTargetItem& DistanceFieldNormal, int32 DepthLevel, int32 DestLevelDownsampleFactor)
{
	SCOPED_DRAW_EVENT(RHICmdList, SetupDepthStencil);

	{		
		RHICmdList.SetViewport(0, 0, 0.0f, View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor, 1.0f);
		RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
		// Depth writes on
		RHICmdList.SetDepthStencilState(TStaticDepthStencilState<true, CF_Always>::GetRHI());
		// Color writes off
		RHICmdList.SetBlendState(TStaticBlendState<CW_NONE>::GetRHI());

		TShaderMapRef<FPostProcessVS> VertexShader(View.ShaderMap);
		TShaderMapRef<FWriteDownsampledDepthPS> PixelShader(View.ShaderMap);

		static FGlobalBoundShaderState BoundShaderState;
		SetGlobalBoundShaderState(RHICmdList, View.GetFeatureLevel(), BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);
		PixelShader->SetParameters(RHICmdList, View, DistanceFieldNormal);

		// Draw a quad writing depth out to the depth stencil view
		DrawRectangle( 
			RHICmdList,
			0, 0, 
			View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor,
			0, 0, 
			View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor,
			FIntPoint(View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor),
			GetBufferSizeForAO(),
			*VertexShader);
	}

	if (GAOInterpolationStencilTesting)
	{
		RHICmdList.SetViewport(0, 0, 0.0f, View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor, 1.0f);
		RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
		RHICmdList.SetBlendState(TStaticBlendState<CW_NONE>::GetRHI());

		// Depth tests on, write 1 to stencil if depth test passed
		RHICmdList.SetDepthStencilState(TStaticDepthStencilState<
			// Note, this is a reversed Z depth surface, using CF_GreaterEqual.
			false,CF_GreaterEqual,
			true,CF_Always,SO_Keep,SO_Keep,SO_Replace,
			false,CF_Always,SO_Keep,SO_Keep,SO_Keep,
			0xff,0xff
			>::GetRHI(), 1);

		TShaderMapRef<TOneColorVS<true> > VertexShader(View.ShaderMap);
		TShaderMapRef<FOneColorPS> PixelShader(View.ShaderMap);

		static FGlobalBoundShaderState BoundShaderState;
		SetGlobalBoundShaderState(RHICmdList, View.GetFeatureLevel(), BoundShaderState, GetVertexDeclarationFVector4(), *VertexShader, *PixelShader);

		SetShaderValue(RHICmdList, PixelShader->GetPixelShader(), PixelShader->ColorParameter, FLinearColor::Black);

		FVector ViewSpaceMaxDistance(0.0f, 0.0f, GAOMaxViewDistance);
		FVector4 ClipSpaceMaxDistance = View.ViewMatrices.ProjMatrix.TransformPosition(ViewSpaceMaxDistance);
		float ClipSpaceZ = ClipSpaceMaxDistance.Z / ClipSpaceMaxDistance.W;

		// Place the quad's depth at the max AO view distance
		// Any pixels that pass the depth test should not receive AO and therefore write a 1 to stencil
		FVector4 ClearQuadVertices[4] = 
		{
			FVector4( -1.0f,  1.0f, ClipSpaceZ, 1.0f ),
			FVector4(  1.0f,  1.0f, ClipSpaceZ, 1.0f ),
			FVector4( -1.0f, -1.0f, ClipSpaceZ, 1.0f ),
			FVector4(  1.0f, -1.0f, ClipSpaceZ, 1.0f )
		};

		DrawPrimitiveUP(RHICmdList, PT_TriangleStrip, 2, ClearQuadVertices, sizeof(ClearQuadVertices[0]));
	}
}

void RenderIrradianceCacheInterpolation(
	FRHICommandListImmediate& RHICmdList, 
	TArray<FViewInfo>& Views, 
	IPooledRenderTarget* InterpolationTarget, 
	FSceneRenderTargetItem& DistanceFieldNormal, 
	int32 DepthLevel, 
	int32 DestLevelDownsampleFactor, 
	const FDistanceFieldAOParameters& Parameters,
	bool bFinalInterpolation)
{
	check(!(bFinalInterpolation && DepthLevel != 0));

	if (bFinalInterpolation && !GAOScatterInterpolation)
	{
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			FViewInfo& View = Views[ViewIndex];

			const FScene* Scene = (const FScene*)View.Family->Scene;
			FSurfaceCacheResources& SurfaceCacheResources = *Scene->SurfaceCacheResources;

			FIntPoint DownsampledViewSize = FIntPoint::DivideAndRoundUp(View.ViewRect.Size(), DestLevelDownsampleFactor);
			uint32 GroupSizeX = FMath::DivideAndRoundUp(DownsampledViewSize.X, GDistanceFieldAOTileSizeX);
			uint32 GroupSizeY = FMath::DivideAndRoundUp(DownsampledViewSize.Y, GDistanceFieldAOTileSizeY);

			TShaderMapRef<TInterpolateCacheCS<4, true> > ComputeShader(View.ShaderMap);

			RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());	
			ComputeShader->SetParameters(RHICmdList, View, DistanceFieldNormal, DepthLevel, DestLevelDownsampleFactor, InterpolationTarget, Parameters, FVector2D(GroupSizeX, GroupSizeY), SurfaceCacheResources);
			DispatchComputeShader(RHICmdList, *ComputeShader, GroupSizeX, GroupSizeY, 1);

			ComputeShader->UnsetParameters(RHICmdList);
		}
	}
	else
	{
		TRefCountPtr<IPooledRenderTarget> SplatDepthStencilBuffer;

		if (bFinalInterpolation && (GAOInterpolationDepthTesting || GAOInterpolationStencilTesting))
		{
			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(InterpolationTarget->GetDesc().Extent, PF_DepthStencil, TexCreate_None, TexCreate_DepthStencilTargetable, false));
			GRenderTargetPool.FindFreeElement(Desc, SplatDepthStencilBuffer, TEXT("DistanceFieldAOSplatDepthBuffer"));

			SetRenderTarget(RHICmdList, InterpolationTarget->GetRenderTargetItem().TargetableTexture, SplatDepthStencilBuffer->GetRenderTargetItem().TargetableTexture);
			RHICmdList.Clear(true, FLinearColor(0, 0, 0, 0), true, 0, true, 0, FIntRect());

			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				FViewInfo& View = Views[ViewIndex];
				SetupDepthStencil(RHICmdList, View, DistanceFieldNormal, DepthLevel, DestLevelDownsampleFactor);
			}

			GRenderTargetPool.VisualizeTexture.SetCheckPoint(RHICmdList, SplatDepthStencilBuffer);

			// Restore render target after checkpoint
			SetRenderTarget(RHICmdList, InterpolationTarget->GetRenderTargetItem().TargetableTexture, SplatDepthStencilBuffer ? SplatDepthStencilBuffer->GetRenderTargetItem().TargetableTexture : NULL);
		}
		else
		{
			SetRenderTarget(RHICmdList, InterpolationTarget->GetRenderTargetItem().TargetableTexture, NULL);
			RHICmdList.Clear(true, FLinearColor(0, 0, 0, 0), false, 0, false, 0, FIntRect());
		}

		SCOPED_DRAW_EVENT(RHICmdList, IrradianceCacheSplat);

		static int32 NumInterpolationSections = 8;
		if (GCircleVertexBuffer.NumSections != NumInterpolationSections)
		{
			GCircleVertexBuffer.NumSections = NumInterpolationSections;
			GCircleVertexBuffer.UpdateRHI();
		}

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			FViewInfo& View = Views[ViewIndex];

			const FScene* Scene = (const FScene*)View.Family->Scene;
			FSurfaceCacheResources& SurfaceCacheResources = *Scene->SurfaceCacheResources;

			const uint32 DownsampledViewSizeX = FMath::DivideAndRoundUp(View.ViewRect.Width(), DestLevelDownsampleFactor);
			const uint32 DownsampledViewSizeY = FMath::DivideAndRoundUp(View.ViewRect.Height(), DestLevelDownsampleFactor);

			RHICmdList.SetViewport(0, 0, 0.0f, DownsampledViewSizeX, DownsampledViewSizeY, 1.0f);
			RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());

			if (bFinalInterpolation && (GAOInterpolationDepthTesting || GAOInterpolationStencilTesting))
			{
				if (GAOInterpolationDepthTesting)
				{
					if (GAOInterpolationStencilTesting)
					{
						// Depth tests enabled, pass stencil test if stencil is zero
						RHICmdList.SetDepthStencilState(TStaticDepthStencilState<
							false,CF_GreaterEqual,
							true,CF_Equal,SO_Keep,SO_Keep,SO_Keep,
							false,CF_Always,SO_Keep,SO_Keep,SO_Keep,
							0xff,0xff
							>::GetRHI());
					}
					else
					{
						// Depth tests enabled
						RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_GreaterEqual>::GetRHI());
					}
				}
				else if (GAOInterpolationStencilTesting)
				{
					// Pass stencil test if stencil is zero
					RHICmdList.SetDepthStencilState(TStaticDepthStencilState<
						false,CF_Always,
						true,CF_Equal,SO_Keep,SO_Keep,SO_Keep,
						false,CF_Always,SO_Keep,SO_Keep,SO_Keep,
						0xff,0xff
						>::GetRHI());
				}
			}
			else
			{
				//@todo - render front faces with depth testing, requires depth buffer
				RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
			}

			RHICmdList.SetBlendState(TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One>::GetRHI());

			const FIntPoint BufferSize = GSceneRenderTargets.GetBufferSizeXY();
			const float InvBufferSizeX = 1.0f / BufferSize.X;
			const float InvBufferSizeY = 1.0f / BufferSize.Y;

			const FVector2D ScreenPositionScale(
				View.ViewRect.Width() * InvBufferSizeX / +2.0f,
				View.ViewRect.Height() * InvBufferSizeY / (-2.0f * GProjectionSignY));

			const FVector2D ScreenPositionBias(
				(View.ViewRect.Width() / 2.0f) * InvBufferSizeX,
				(View.ViewRect.Height() / 2.0f) * InvBufferSizeY);

			FVector2D OffsetToLowResCorner = (FVector2D(.5f, .5f) / FVector2D(DownsampledViewSizeX, DownsampledViewSizeY) - FVector2D(.5f, .5f)) * FVector2D(2, -2);
			FVector2D OffsetToTopResPixel = (FVector2D(.5f, .5f) / BufferSize - ScreenPositionBias) / ScreenPositionScale;
			FVector2D NormalizedOffsetToPixelCenter = OffsetToLowResCorner - OffsetToTopResPixel;

			RHICmdList.SetStreamSource(0, GCircleVertexBuffer.VertexBufferRHI, sizeof(FScreenVertex), 0);

			if (bFinalInterpolation)
			{
				TShaderMapRef<TIrradianceCacheSplatVS<true> > VertexShader(View.ShaderMap);
				TShaderMapRef<TIrradianceCacheSplatPS<true> > PixelShader(View.ShaderMap);

				for (int32 SplatSourceDepthLevel = GAOMaxLevel; SplatSourceDepthLevel >= FMath::Max(DepthLevel, GAOMinLevel); SplatSourceDepthLevel--)
				{
					static FGlobalBoundShaderState BoundShaderState;
					
					SetGlobalBoundShaderState(RHICmdList, View.GetFeatureLevel(), BoundShaderState, GScreenVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

					VertexShader->SetParameters(RHICmdList, View, SplatSourceDepthLevel, DestLevelDownsampleFactor, NormalizedOffsetToPixelCenter);
					PixelShader->SetParameters(RHICmdList, View, DistanceFieldNormal, DestLevelDownsampleFactor, Parameters);

					RHICmdList.DrawPrimitiveIndirect(PT_TriangleList, SurfaceCacheResources.Level[SplatSourceDepthLevel]->ScatterDrawParameters.Buffer, 0);
				}
			}
			else
			{
				TShaderMapRef<TIrradianceCacheSplatVS<false> > VertexShader(View.ShaderMap);
				TShaderMapRef<TIrradianceCacheSplatPS<false> > PixelShader(View.ShaderMap);

				for (int32 SplatSourceDepthLevel = GAOMaxLevel; SplatSourceDepthLevel >= FMath::Max(DepthLevel, GAOMinLevel); SplatSourceDepthLevel--)
				{
					static FGlobalBoundShaderState BoundShaderState;
					
					SetGlobalBoundShaderState(RHICmdList, View.GetFeatureLevel(), BoundShaderState, GScreenVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

					VertexShader->SetParameters(RHICmdList, View, SplatSourceDepthLevel, DestLevelDownsampleFactor, NormalizedOffsetToPixelCenter);
					PixelShader->SetParameters(RHICmdList, View, DistanceFieldNormal, DestLevelDownsampleFactor, Parameters);

					RHICmdList.DrawPrimitiveIndirect(PT_TriangleList, SurfaceCacheResources.Level[SplatSourceDepthLevel]->ScatterDrawParameters.Buffer, 0);
				}
			}
		}
	}
}

void UpdateVisibleObjectBuffers(const FScene* Scene, const FDistanceFieldAOParameters& Parameters, const FViewInfo& View, int32& NumObjects)
{
	//@todo - scene rendering allocator
	TArray<FVector4> ObjectBoundsData;
	TArray<FVector4> ObjectData;
	TArray<FVector4> ObjectData2;

	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_GatherObjectData);

		const FVector ViewOriginForDistanceCulling = View.ViewMatrices.ViewOrigin;
		const int32 NumTexelsOneDimX = GDistanceFieldVolumeTextureAtlas.GetSizeX();
		const int32 NumTexelsOneDimY = GDistanceFieldVolumeTextureAtlas.GetSizeY();
		const int32 NumTexelsOneDimZ = GDistanceFieldVolumeTextureAtlas.GetSizeZ();
		const FVector InvTextureDim(1.0f / NumTexelsOneDimX, 1.0f / NumTexelsOneDimY, 1.0f / NumTexelsOneDimZ);

		ObjectBoundsData.Empty(Scene->Primitives.Num() / 2);
		ObjectData.Empty(FDistanceFieldObjectBuffers::ObjectDataStride * Scene->Primitives.Num() / 2);
		ObjectData2.Empty(FDistanceFieldObjectBuffers::ObjectData2Stride * Scene->Primitives.Num() / 2);

		//@todo - dense data-coherent pass like frustum culling in InitViews
		// FTileIntersectionResources uses 16 bit object indices
		for (int32 PrimitiveIndex = 0; PrimitiveIndex < Scene->Primitives.Num() && NumObjects < MAX_uint16; PrimitiveIndex++)
		{
			const FPrimitiveSceneInfo* PrimitiveSceneInfo = Scene->Primitives[PrimitiveIndex];

			if (PrimitiveSceneInfo->Proxy->CastsDynamicShadow() || PrimitiveSceneInfo->Proxy->CastsStaticShadow())
			{
				FBox LocalVolumeBounds;
				FIntVector BlockMin;
				FIntVector BlockSize;
				bool bBuiltAsIfTwoSided;
				bool bMeshWasPlane;
				PrimitiveSceneInfo->Proxy->GetDistancefieldAtlasData(LocalVolumeBounds, BlockMin, BlockSize, bBuiltAsIfTwoSided, bMeshWasPlane);

				if (BlockMin.X >= 0 && BlockMin.Y >= 0 && BlockMin.Z >= 0)
				{
					FMatrix LocalToWorld = PrimitiveSceneInfo->Proxy->GetLocalToWorld();

					if (bMeshWasPlane)
					{
						FVector LocalScales = LocalToWorld.GetScaleVector();
						FVector AbsLocalScales(FMath::Abs(LocalScales.X), FMath::Abs(LocalScales.Y), FMath::Abs(LocalScales.Z));
						float MidScale = FMath::Min(AbsLocalScales.X, AbsLocalScales.Y);
						float ScaleAdjust = FMath::Sign(LocalScales.Z) * MidScale / AbsLocalScales.Z;
						// The mesh was determined to be a plane flat in Z during the build process, so we can change the Z scale
						// Helps in cases with modular ground pieces with scales of (10, 10, 1) and some triangles just above Z=0
						LocalToWorld.SetAxis(2, LocalToWorld.GetScaledAxis(EAxis::Z) * ScaleAdjust);
					}

					const FMatrix VolumeToWorld = FScaleMatrix(LocalVolumeBounds.GetExtent()) 
						* FTranslationMatrix(LocalVolumeBounds.GetCenter())
						* LocalToWorld;

					const FVector4 ObjectBoundingSphere(VolumeToWorld.GetOrigin(), VolumeToWorld.GetScaleVector().Size());
					const float DistanceToViewSq = (ViewOriginForDistanceCulling - (FVector)ObjectBoundingSphere).SizeSquared();

					if (!GAOCPUFrustumCull 
						|| (DistanceToViewSq < FMath::Square(GAOMaxViewDistance + ObjectBoundingSphere.W)
						&& View.ViewFrustum.IntersectSphere((FVector)ObjectBoundingSphere, ObjectBoundingSphere.W + Parameters.OcclusionMaxDistance)))
					{
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

						const FMatrix WorldToVolume = UniformScaleVolumeToWorld.Inverse();
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
		}
	}

	if (NumObjects > 0)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_UpdateObjectBuffers);

		if (NumObjects > GAOObjectBuffers.Buffers.MaxObjects)
		{
			// Allocate with slack
			GAOObjectBuffers.Buffers.MaxObjects = NumObjects * 5 / 4;
			GAOObjectBuffers.Buffers.Release();
			GAOObjectBuffers.Buffers.Initialize();
		}

		checkSlow(NumObjects < MAX_uint16);

		void* LockedBuffer = RHILockVertexBuffer(GAOObjectBuffers.Buffers.ObjectData.Bounds, 0, GAOObjectBuffers.Buffers.ObjectData.Bounds->GetSize(), RLM_WriteOnly);
		FPlatformMemory::Memcpy(LockedBuffer, ObjectBoundsData.GetData(), ObjectBoundsData.GetTypeSize() * ObjectBoundsData.Num());
		RHIUnlockVertexBuffer(GAOObjectBuffers.Buffers.ObjectData.Bounds);

		LockedBuffer = RHILockVertexBuffer(GAOObjectBuffers.Buffers.ObjectData.Data, 0, GAOObjectBuffers.Buffers.ObjectData.Data->GetSize(), RLM_WriteOnly);
		FPlatformMemory::Memcpy(LockedBuffer, ObjectData.GetData(), ObjectData.GetTypeSize() * ObjectData.Num());
		RHIUnlockVertexBuffer(GAOObjectBuffers.Buffers.ObjectData.Data);

		LockedBuffer = RHILockVertexBuffer(GAOObjectBuffers.Buffers.ObjectData.Data2, 0, GAOObjectBuffers.Buffers.ObjectData.Data2->GetSize(), RLM_WriteOnly);
		FPlatformMemory::Memcpy(LockedBuffer, ObjectData2.GetData(), ObjectData2.GetTypeSize() * ObjectData2.Num());
		RHIUnlockVertexBuffer(GAOObjectBuffers.Buffers.ObjectData.Data2);
	}
}

bool FDeferredShadingSceneRenderer::RenderDistanceFieldAOSurfaceCache(FRHICommandListImmediate& RHICmdList, const FDistanceFieldAOParameters& Parameters, TRefCountPtr<IPooledRenderTarget>& OutDynamicBentNormalAO, bool bApplyToSceneColor)
{
	//@todo - support multiple views
	const FViewInfo& View = Views[0];

	if (GDistanceFieldAO 
		&& FeatureLevel >= ERHIFeatureLevel::SM5
		&& DoesPlatformSupportDistanceFieldAO(View.GetShaderPlatform())
		&& Views.Num() == 1
		// ViewState is used to cache tile intersection resources which have to be sized based on the view
		&& View.State)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_RenderDistanceFieldAOSurfaceCache);
		SCOPED_DRAW_EVENT(RHICmdList, DistanceFieldAO);

		// Update the global distance field atlas
		GDistanceFieldVolumeTextureAtlas.UpdateAllocations();

		if (GDistanceFieldVolumeTextureAtlas.VolumeTextureRHI)
		{
			GenerateBestSpacedVectors();

			// Create surface cache state that will persist across frames if needed
			if (!Scene->SurfaceCacheResources)
			{
				Scene->SurfaceCacheResources = new FSurfaceCacheResources();
			}

			FSurfaceCacheResources& SurfaceCacheResources = *Scene->SurfaceCacheResources;
			FIntPoint DownsampledViewRect = FIntPoint::DivideAndRoundDown(View.ViewRect.Size(), GAODownsampleFactor);
			int32 SlackAmount = 2;
			int32 MaxSurfaceCacheSamples = SlackAmount * DownsampledViewRect.X * DownsampledViewRect.Y / (1 << (2 * GAOMinLevel * GAOPowerOfTwoBetweenLevels));
			SurfaceCacheResources.AllocateFor(GAOMinLevel, GAOMaxLevel, MaxSurfaceCacheSamples);

			uint32 ClearValues[4] = {0};

			if (!SurfaceCacheResources.bClearedResources || !GAOReuseAcrossFrames)
			{
				// Reset the number of active cache records to 0
				for (int32 DepthLevel = GAOMaxLevel; DepthLevel >= GAOMinLevel; DepthLevel--)
				{
					RHICmdList.ClearUAV(SurfaceCacheResources.Level[DepthLevel]->ScatterDrawParameters.UAV, ClearValues);
				}

				RHICmdList.ClearUAV(SurfaceCacheResources.TempResources->ScatterDrawParameters.UAV, ClearValues);

				SurfaceCacheResources.bClearedResources = true;
			}

			int32 NumObjects = 0;

			// Update object buffers with objects that have a valid distance field representation and may affect pixels on the screen
			UpdateVisibleObjectBuffers(Scene, Parameters, View, NumObjects);

			if (NumObjects > 0)
			{
				QUICK_SCOPE_CYCLE_COUNTER(STAT_AOIssueGPUWork);

				// Intersect objects with screen tiles, build lists
				FIntPoint TileListGroupSize = BuildTileObjectLists(RHICmdList, Scene, Views, NumObjects, Parameters);

				TRefCountPtr<IPooledRenderTarget> DistanceFieldNormal;

				{
					const FIntPoint BufferSize = GetBufferSizeForAO();
					FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, PF_FloatRGBA, TexCreate_None, TexCreate_RenderTargetable | TexCreate_UAV, false));
					GRenderTargetPool.FindFreeElement(Desc, DistanceFieldNormal, TEXT("DistanceFieldNormal"));
				}

				// Compute the distance field normal, this is used for surface caching instead of the GBuffer normal because it matches the occluding geometry
				ComputeDistanceFieldNormal(RHICmdList, Views, DistanceFieldNormal->GetRenderTargetItem(), NumObjects, Parameters);

				GRenderTargetPool.VisualizeTexture.SetCheckPoint(RHICmdList, DistanceFieldNormal);

				if (GAOReuseAcrossFrames)
				{
					SCOPED_DRAW_EVENT(RHICmdList, TrimRecords);

					// Copy and trim last frame's surface cache samples
					for (int32 DepthLevel = GAOMaxLevel; DepthLevel >= GAOMinLevel; DepthLevel--)
					{
						if (GetTrimOldRecordsFraction(DepthLevel) > 0)
						{
							{	
								TShaderMapRef<FSetupCopyIndirectArgumentsCS> ComputeShader(View.ShaderMap);

								RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
								ComputeShader->SetParameters(RHICmdList, View, DepthLevel);
								DispatchComputeShader(RHICmdList, *ComputeShader, 1, 1, 1);

								ComputeShader->UnsetParameters(RHICmdList);
							}

							{
								TShaderMapRef<FCopyIrradianceCacheSamplesCS> ComputeShader(View.ShaderMap);

								RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
								ComputeShader->SetParameters(RHICmdList, View, DepthLevel);
								DispatchIndirectComputeShader(RHICmdList, *ComputeShader, SurfaceCacheResources.DispatchParameters.Buffer, 0);

								ComputeShader->UnsetParameters(RHICmdList);
							}

							Swap(SurfaceCacheResources.Level[DepthLevel], SurfaceCacheResources.TempResources);
						}
					}
				}

				// Start from the highest depth, which will consider the fewest potential shading points
				// Each level potentially prevents the next higher resolution level from doing expensive shading work
				// This is the core of the surface cache algorithm
				for (int32 DepthLevel = GAOMaxLevel; DepthLevel >= GAOMinLevel; DepthLevel--)
				{
					int32 DestLevelDownsampleFactor = GAODownsampleFactor * (1 << (DepthLevel * GAOPowerOfTwoBetweenLevels));

					SCOPED_DRAW_EVENTF(RHICmdList, Level, TEXT("Level_%d"), DepthLevel);

					TRefCountPtr<IPooledRenderTarget> DistanceFieldAOIrradianceCacheSplat;

					{
						FIntPoint AOBufferSize = FIntPoint::DivideAndRoundUp(GSceneRenderTargets.GetBufferSizeXY(), DestLevelDownsampleFactor);
						FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(AOBufferSize, PF_FloatRGBA, TexCreate_None, TexCreate_RenderTargetable | TexCreate_UAV, false));
						GRenderTargetPool.FindFreeElement(Desc, DistanceFieldAOIrradianceCacheSplat, TEXT("DistanceFieldAOIrradianceCacheSplat"));
					}

					// Splat / interpolate the surface cache records onto the buffer sized for the current depth level
					RenderIrradianceCacheInterpolation(RHICmdList, Views, DistanceFieldAOIrradianceCacheSplat, DistanceFieldNormal->GetRenderTargetItem(), DepthLevel, DestLevelDownsampleFactor, Parameters, false);

					{
						SCOPED_DRAW_EVENT(RHICmdList, PopulateIrradianceCache);
						SetRenderTarget(RHICmdList, NULL, NULL);

						// Save off the current record count before adding any more
						{	
							TShaderMapRef<FSaveStartIndexCS> ComputeShader(View.ShaderMap);

							RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
							ComputeShader->SetParameters(RHICmdList, View, DepthLevel);
							DispatchComputeShader(RHICmdList, *ComputeShader, 1, 1, 1);

							ComputeShader->UnsetParameters(RHICmdList);
						}

						// Create new records which haven't been shaded yet for shading points which don't have a valid interpolation from existing records
						for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
						{
							const FViewInfo& View = Views[ViewIndex];

							FIntPoint DownsampledViewSize = FIntPoint::DivideAndRoundUp(View.ViewRect.Size(), DestLevelDownsampleFactor);
							uint32 GroupSizeX = FMath::DivideAndRoundUp(DownsampledViewSize.X, GDistanceFieldAOTileSizeX);
							uint32 GroupSizeY = FMath::DivideAndRoundUp(DownsampledViewSize.Y, GDistanceFieldAOTileSizeY);

							TShaderMapRef<FPopulateCacheCS> ComputeShader(View.ShaderMap);

							RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
							ComputeShader->SetParameters(RHICmdList, View, DistanceFieldAOIrradianceCacheSplat->GetRenderTargetItem(), DistanceFieldNormal->GetRenderTargetItem(), DestLevelDownsampleFactor, DepthLevel, TileListGroupSize, Parameters);
							DispatchComputeShader(RHICmdList, *ComputeShader, GroupSizeX, GroupSizeY, 1);

							ComputeShader->UnsetParameters(RHICmdList);
						}
					}

					{
						SCOPED_DRAW_EVENT(RHICmdList, ShadeIrradianceCache);

						{	
							TShaderMapRef<FSetupFinalGatherIndirectArgumentsCS> ComputeShader(View.ShaderMap);

							RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
							ComputeShader->SetParameters(RHICmdList, View, DepthLevel);
							DispatchComputeShader(RHICmdList, *ComputeShader, 1, 1, 1);

							ComputeShader->UnsetParameters(RHICmdList);
						}

						// Compute lighting for the new surface cache records by cone-stepping through the object distance fields
						{
							TShaderMapRef<FFinalGatherCS> ComputeShader(View.ShaderMap);

							RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
							ComputeShader->SetParameters(RHICmdList, View, DistanceFieldAOIrradianceCacheSplat->GetRenderTargetItem(), DistanceFieldNormal->GetRenderTargetItem(), NumObjects, DestLevelDownsampleFactor, DepthLevel, TileListGroupSize, Parameters);
							DispatchIndirectComputeShader(RHICmdList, *ComputeShader, SurfaceCacheResources.DispatchParameters.Buffer, 0);

							ComputeShader->UnsetParameters(RHICmdList);
						}
					}
				}

				TRefCountPtr<IPooledRenderTarget> IrradianceCacheAccumulation;

				{
					FIntPoint BufferSize = GetBufferSizeForAO();
					FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, PF_FloatRGBA, TexCreate_None, TexCreate_RenderTargetable | TexCreate_UAV, false));
					GRenderTargetPool.FindFreeElement(Desc, IrradianceCacheAccumulation, TEXT("DistanceFieldAOIrradianceCacheAccumulation"));
				}

				// Splat the surface cache records onto the opaque pixels, using less strict weighting so the lighting is smoothed in world space
				{
					SCOPED_DRAW_EVENT(RHICmdList, FinalIrradianceCacheSplat);
					RenderIrradianceCacheInterpolation(RHICmdList, Views, IrradianceCacheAccumulation, DistanceFieldNormal->GetRenderTargetItem(), 0, GAODownsampleFactor, Parameters, true);
				}

				GRenderTargetPool.VisualizeTexture.SetCheckPoint(RHICmdList, IrradianceCacheAccumulation);

				// Post process the AO to cover over artifacts
				TRefCountPtr<IPooledRenderTarget> AOOutput;
				PostProcessBentNormalAO(RHICmdList, Parameters, Views, IrradianceCacheAccumulation->GetRenderTargetItem(), DistanceFieldNormal->GetRenderTargetItem(), AOOutput);

				GRenderTargetPool.VisualizeTexture.SetCheckPoint(RHICmdList, AOOutput);

				if (bApplyToSceneColor)
				{
					GSceneRenderTargets.BeginRenderingSceneColor(RHICmdList);
				}
				else
				{
					FPooledRenderTargetDesc Desc = GSceneRenderTargets.GetSceneColor()->GetDesc();
					// Make sure we get a signed format
					Desc.Format = PF_FloatRGBA;
					GRenderTargetPool.FindFreeElement(Desc, OutDynamicBentNormalAO, TEXT("DynamicBentNormalAO"));

					SetRenderTarget(RHICmdList, OutDynamicBentNormalAO->GetRenderTargetItem().TargetableTexture, NULL);
				}

				// Upsample to full resolution, write to output
				UpsampleBentNormalAO(RHICmdList, Views, AOOutput, !bApplyToSceneColor);

				return true;
			}
		}
	}

	return false;
}

class FVisualizeMeshDistanceFieldCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FVisualizeMeshDistanceFieldCS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), GDistanceFieldAOTileSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), GDistanceFieldAOTileSizeY);
		OutEnvironment.SetDefine(TEXT("MAX_OBJECTS_PER_TILE"), GMaxNumObjectsPerTile);
	}

	/** Default constructor. */
	FVisualizeMeshDistanceFieldCS() {}

	/** Initialization constructor. */
	FVisualizeMeshDistanceFieldCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		VisualizeMeshDistanceFields.Bind(Initializer.ParameterMap, TEXT("VisualizeMeshDistanceFields"));
		NumGroups.Bind(Initializer.ParameterMap, TEXT("NumGroups"));
		ObjectParameters.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		AOParameters.Bind(Initializer.ParameterMap);
	}

	void SetParameters(
		FRHICommandList& RHICmdList, 
		const FSceneView& View, 
		FSceneRenderTargetItem& VisualizeMeshDistanceFieldsValue, 
		int32 NumObjects, 
		FVector2D NumGroupsValue,
		const FDistanceFieldAOParameters& Parameters)
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);

		VisualizeMeshDistanceFields.SetTexture(RHICmdList, ShaderRHI, VisualizeMeshDistanceFieldsValue.ShaderResourceTexture, VisualizeMeshDistanceFieldsValue.UAV);

		ObjectParameters.Set(RHICmdList, ShaderRHI, GAOObjectBuffers.Buffers, NumObjects);
		AOParameters.Set(RHICmdList, ShaderRHI, Parameters);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);

		SetShaderValue(RHICmdList, ShaderRHI, NumGroups, NumGroupsValue);
	}

	void UnsetParameters(FRHICommandList& RHICmdList)
	{
		VisualizeMeshDistanceFields.UnsetUAV(RHICmdList, GetComputeShader());
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << VisualizeMeshDistanceFields;
		Ar << NumGroups;
		Ar << ObjectParameters;
		Ar << DeferredParameters;
		Ar << AOParameters;
		return bShaderHasOutdatedParameters;
	}

private:

	FRWShaderParameter VisualizeMeshDistanceFields;
	FShaderParameter NumGroups;
	FDistanceFieldObjectBufferParameters ObjectParameters;
	FDeferredPixelShaderParameters DeferredParameters;
	FAOParameters AOParameters;
};

IMPLEMENT_SHADER_TYPE(,FVisualizeMeshDistanceFieldCS,TEXT("DistanceFieldSurfaceCacheLighting"),TEXT("VisualizeMeshDistanceFieldCS"),SF_Compute);


class FVisualizeDistanceFieldUpsamplePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FVisualizeDistanceFieldUpsamplePS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && DoesPlatformSupportDistanceFieldAO(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);
	}

	/** Default constructor. */
	FVisualizeDistanceFieldUpsamplePS() {}

	/** Initialization constructor. */
	FVisualizeDistanceFieldUpsamplePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		VisualizeDistanceFieldTexture.Bind(Initializer.ParameterMap,TEXT("VisualizeDistanceFieldTexture"));
		VisualizeDistanceFieldSampler.Bind(Initializer.ParameterMap,TEXT("VisualizeDistanceFieldSampler"));
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, TRefCountPtr<IPooledRenderTarget>& VisualizeDistanceField)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);

		SetTextureParameter(RHICmdList, ShaderRHI, VisualizeDistanceFieldTexture, VisualizeDistanceFieldSampler, TStaticSamplerState<SF_Bilinear>::GetRHI(), VisualizeDistanceField->GetRenderTargetItem().ShaderResourceTexture);
	}
	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << VisualizeDistanceFieldTexture;
		Ar << VisualizeDistanceFieldSampler;
		return bShaderHasOutdatedParameters;
	}

private:

	FDeferredPixelShaderParameters DeferredParameters;
	FShaderResourceParameter VisualizeDistanceFieldTexture;
	FShaderResourceParameter VisualizeDistanceFieldSampler;
};

IMPLEMENT_SHADER_TYPE(,FVisualizeDistanceFieldUpsamplePS,TEXT("DistanceFieldSurfaceCacheLighting"),TEXT("VisualizeDistanceFieldUpsamplePS"),SF_Pixel);

void FDeferredShadingSceneRenderer::RenderMeshDistanceFieldVisualization(FRHICommandListImmediate& RHICmdList, const FDistanceFieldAOParameters& Parameters)
{
	//@todo - support multiple views
	const FViewInfo& View = Views[0];

	if (GDistanceFieldAO 
		&& FeatureLevel >= ERHIFeatureLevel::SM5
		&& DoesPlatformSupportDistanceFieldAO(View.GetShaderPlatform())
		&& Views.Num() == 1)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_RenderMeshDistanceFieldVis);
		SCOPED_DRAW_EVENT(RHICmdList, VisualizeMeshDistanceFields);

		// Update the global distance field atlas
		GDistanceFieldVolumeTextureAtlas.UpdateAllocations();

		if (GDistanceFieldVolumeTextureAtlas.VolumeTextureRHI)
		{
			int32 NumObjects = 0;

			// Update object buffers with objects that have a valid distance field representation and may affect pixels on the screen
			UpdateVisibleObjectBuffers(Scene, Parameters, View, NumObjects);

			if (NumObjects > 0)
			{
				QUICK_SCOPE_CYCLE_COUNTER(STAT_AOIssueGPUWork);

				TRefCountPtr<IPooledRenderTarget> VisualizeResultRT;

				{
					const FIntPoint BufferSize = GetBufferSizeForAO();
					FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, PF_FloatRGBA, TexCreate_None, TexCreate_RenderTargetable | TexCreate_UAV, false));
					GRenderTargetPool.FindFreeElement(Desc, VisualizeResultRT, TEXT("VisualizeDistanceField"));
				}

				{
					SetRenderTarget(RHICmdList, NULL, NULL);

					for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
					{
						const FViewInfo& View = Views[ViewIndex];

						uint32 GroupSizeX = FMath::DivideAndRoundUp(View.ViewRect.Size().X / GAODownsampleFactor, GDistanceFieldAOTileSizeX);
						uint32 GroupSizeY = FMath::DivideAndRoundUp(View.ViewRect.Size().Y / GAODownsampleFactor, GDistanceFieldAOTileSizeY);

						{
							SCOPED_DRAW_EVENT(RHICmdList, VisualizeMeshDistanceFieldCS);
							TShaderMapRef<FVisualizeMeshDistanceFieldCS> ComputeShader(View.ShaderMap);

							RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());
							ComputeShader->SetParameters(RHICmdList, View, VisualizeResultRT->GetRenderTargetItem(), NumObjects, FVector2D(GroupSizeX, GroupSizeY), Parameters);
							DispatchComputeShader(RHICmdList, *ComputeShader, GroupSizeX, GroupSizeY, 1);

							ComputeShader->UnsetParameters(RHICmdList);
						}
					}
				}

				{
					GSceneRenderTargets.BeginRenderingSceneColor(RHICmdList);

					for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
					{
						const FViewInfo& View = Views[ViewIndex];

						SCOPED_DRAW_EVENT(RHICmdList, UpsampleAO);

						RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);
						RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
						RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
						RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());

						TShaderMapRef<FPostProcessVS> VertexShader( View.ShaderMap );
						TShaderMapRef<FVisualizeDistanceFieldUpsamplePS> PixelShader( View.ShaderMap );

						static FGlobalBoundShaderState BoundShaderState;

						SetGlobalBoundShaderState(RHICmdList, View.GetFeatureLevel(), BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

						PixelShader->SetParameters(RHICmdList, View, VisualizeResultRT);

						DrawRectangle( 
							RHICmdList,
							0, 0, 
							View.ViewRect.Width(), View.ViewRect.Height(),
							View.ViewRect.Min.X / GAODownsampleFactor, View.ViewRect.Min.Y / GAODownsampleFactor, 
							View.ViewRect.Width() / GAODownsampleFactor, View.ViewRect.Height() / GAODownsampleFactor,
							FIntPoint(View.ViewRect.Width(), View.ViewRect.Height()),
							GetBufferSizeForAO(),
							*VertexShader);
					}
				}
			}
		}
	}
}

template<bool bApplyShadowing>
class TDynamicSkyLightDiffusePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(TDynamicSkyLightDiffusePS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform,OutEnvironment);
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_FACTOR"), GAODownsampleFactor);
		OutEnvironment.SetDefine(TEXT("APPLY_SHADOWING"), (uint32)(bApplyShadowing ? 1 : 0));
	}

	/** Default constructor. */
	TDynamicSkyLightDiffusePS() {}

	/** Initialization constructor. */
	TDynamicSkyLightDiffusePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		DynamicBentNormalAOTexture.Bind(Initializer.ParameterMap, TEXT("BentNormalAOTexture"));
		DynamicBentNormalAOSampler.Bind(Initializer.ParameterMap, TEXT("BentNormalAOSampler"));
		ContrastAndNormalizeMulAdd.Bind(Initializer.ParameterMap, TEXT("ContrastAndNormalizeMulAdd"));
		OcclusionTintAndMinOcclusion.Bind(Initializer.ParameterMap, TEXT("OcclusionTintAndMinOcclusion"));
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, FTextureRHIParamRef DynamicBentNormalAO, const FDistanceFieldAOParameters& Parameters, const FSkyLightSceneProxy* SkyLight)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();
		FGlobalShader::SetParameters(RHICmdList, ShaderRHI, View);
		DeferredParameters.Set(RHICmdList, ShaderRHI, View);

		SetTextureParameter(RHICmdList, ShaderRHI, DynamicBentNormalAOTexture, DynamicBentNormalAOSampler, TStaticSamplerState<SF_Point>::GetRHI(), DynamicBentNormalAO);

		// Scale and bias to remap the contrast curve to [0,1]
		const float Min = 1 / (1 + FMath::Exp(-Parameters.Contrast * (0 * 10 - 5)));
		const float Max = 1 / (1 + FMath::Exp(-Parameters.Contrast * (1 * 10 - 5)));
		const float Mul = 1.0f / (Max - Min);
		const float Add = -Min / (Max - Min);

		SetShaderValue(RHICmdList, ShaderRHI, ContrastAndNormalizeMulAdd, FVector(Parameters.Contrast, Mul, Add));

		FVector4 OcclusionTintAndMinOcclusionValue = FVector4(SkyLight->OcclusionTint);
		OcclusionTintAndMinOcclusionValue.W = SkyLight->MinOcclusion;
		SetShaderValue(RHICmdList, ShaderRHI, OcclusionTintAndMinOcclusion, OcclusionTintAndMinOcclusionValue);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << DynamicBentNormalAOTexture;
		Ar << DynamicBentNormalAOSampler;
		Ar << ContrastAndNormalizeMulAdd;
		Ar << OcclusionTintAndMinOcclusion;
		return bShaderHasOutdatedParameters;
	}

private:

	FDeferredPixelShaderParameters DeferredParameters;
	FShaderResourceParameter DynamicBentNormalAOTexture;
	FShaderResourceParameter DynamicBentNormalAOSampler;
	FShaderParameter ContrastAndNormalizeMulAdd;
	FShaderParameter OcclusionTintAndMinOcclusion;
};

IMPLEMENT_SHADER_TYPE(template<>,TDynamicSkyLightDiffusePS<true>,TEXT("SkyLighting"),TEXT("SkyLightDiffusePS"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>,TDynamicSkyLightDiffusePS<false>,TEXT("SkyLighting"),TEXT("SkyLightDiffusePS"),SF_Pixel);

void FDeferredShadingSceneRenderer::RenderDynamicSkyLighting(FRHICommandListImmediate& RHICmdList, TRefCountPtr<IPooledRenderTarget>& DynamicBentNormalAO)
{
	if (Scene->SkyLight
		&& Scene->SkyLight->ProcessedTexture
		&& !Scene->SkyLight->bWantsStaticShadowing
		&& !Scene->SkyLight->bHasStaticLighting
		&& ViewFamily.EngineShowFlags.SkyLighting
		&& FeatureLevel >= ERHIFeatureLevel::SM4
		&& !IsSimpleDynamicLightingEnabled() 
		&& !ViewFamily.EngineShowFlags.VisualizeLightCulling)
	{
		SCOPED_DRAW_EVENT(RHICmdList, SkyLightDiffuse);

		bool bApplyShadowing = false;

		FDistanceFieldAOParameters Parameters(Scene->SkyLight->OcclusionMaxDistance, Scene->SkyLight->Contrast);

		if (Scene->SkyLight->bCastShadows
			&& ViewFamily.EngineShowFlags.DistanceFieldAO 
			&& !ViewFamily.EngineShowFlags.VisualizeDistanceFieldAO
			&& !ViewFamily.EngineShowFlags.VisualizeMeshDistanceFields)
		{
			bApplyShadowing = RenderDistanceFieldAOSurfaceCache(RHICmdList, Parameters, DynamicBentNormalAO, false);
		}

		GSceneRenderTargets.BeginRenderingSceneColor(RHICmdList);

		for( int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++ )
		{
			const FViewInfo& View = Views[ViewIndex];

			RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);
			RHICmdList.SetRasterizerState(TStaticRasterizerState< FM_Solid, CM_None >::GetRHI());
			RHICmdList.SetDepthStencilState(TStaticDepthStencilState< false, CF_Always >::GetRHI());
			RHICmdList.SetBlendState(TStaticBlendState< CW_RGB, BO_Add, BF_One, BF_One >::GetRHI());

			TShaderMapRef< FPostProcessVS > VertexShader(View.ShaderMap);

			if (bApplyShadowing)
			{
				TShaderMapRef<TDynamicSkyLightDiffusePS<true> > PixelShader(View.ShaderMap);

				static FGlobalBoundShaderState BoundShaderState;
				SetGlobalBoundShaderState(RHICmdList, View.GetFeatureLevel(), BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);
				PixelShader->SetParameters(RHICmdList, View, DynamicBentNormalAO->GetRenderTargetItem().ShaderResourceTexture, Parameters, Scene->SkyLight);
			}
			else
			{
				TShaderMapRef<TDynamicSkyLightDiffusePS<false> > PixelShader(View.ShaderMap);

				static FGlobalBoundShaderState BoundShaderState;
				SetGlobalBoundShaderState(RHICmdList, View.GetFeatureLevel(), BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);
				PixelShader->SetParameters(RHICmdList, View, GWhiteTexture->TextureRHI, Parameters, Scene->SkyLight);
			}

			DrawRectangle( 
				RHICmdList,
				0, 0, 
				View.ViewRect.Width(), View.ViewRect.Height(),
				View.ViewRect.Min.X, View.ViewRect.Min.Y, 
				View.ViewRect.Width(), View.ViewRect.Height(),
				FIntPoint(View.ViewRect.Width(), View.ViewRect.Height()),
				GSceneRenderTargets.GetBufferSizeXY(),
				*VertexShader);
		}
	}
}
