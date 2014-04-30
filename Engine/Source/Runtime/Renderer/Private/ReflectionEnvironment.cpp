// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Reflection Environment - feature that provides HDR glossy reflections on any surfaces, leveraging precomputation to prefilter cubemaps of the scene
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessing.h"
#include "UniformBuffer.h"
#include "ShaderParameters.h"
#include "ScreenRendering.h"
#include "ScreenSpaceReflections.h"
#include "PostProcessTemporalAA.h"
#include "PostProcessDownsample.h"
#include "ReflectionEnvironment.h"
#include "LightRendering.h"

/** Tile size for the reflection environment compute shader, tweaked for 680 GTX. */
const int32 GReflectionEnvironmentTileSizeX = 16;
const int32 GReflectionEnvironmentTileSizeY = 16;
extern ENGINE_API int32 GReflectionCaptureSize;

static int32 GDiffuseFromCaptures = 0;
static FAutoConsoleVariableRef CVarDiffuseFromCaptures(
	TEXT("r.DiffuseFromCaptures"),
	GDiffuseFromCaptures,
	TEXT("Apply indirect diffuse lighting from captures instead of lightmaps.\n")
	TEXT(" 0 is off (default), 1 is on"),
	ECVF_RenderThreadSafe
	);

// to avoid having direct access from many places
static int GetReflectionEnvironmentCVar()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.ReflectionEnvironment"));
	check(CVar);
	return CVar->GetValueOnAnyThread();
#endif

	// on, default mode
	return 1;
}

bool IsReflectionEnvironmentAvailable()
{
	static const auto AllowStaticLightingVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.AllowStaticLighting"));
	const bool bAllowStaticLighting = (!AllowStaticLightingVar || AllowStaticLightingVar->GetValueOnAnyThread() != 0);

	return (GRHIFeatureLevel >= ERHIFeatureLevel::SM4) && (GetReflectionEnvironmentCVar() != 0) && bAllowStaticLighting;
}

void FReflectionEnvironmentCubemapArray::InitDynamicRHI()
{
	if (GRHIFeatureLevel == ERHIFeatureLevel::SM5)
	{
		const int32 NumReflectionCaptureMips = FMath::CeilLogTwo(GReflectionCaptureSize) + 1;

		ReleaseCubeArray();

		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::CreateCubemapDesc(
			GReflectionCaptureSize,
			//@todo - get rid of the alpha channel (currently stores brightness which is a constant), could use PF_FloatRGB for half memory, would need to implement RHIReadSurface support
			PF_FloatRGBA, 
			TexCreate_None,
			TexCreate_None,
			false, 
			// Cubemap array of 1 produces a regular cubemap, so guarantee it will be allocated as an array
			FMath::Max<uint32>(MaxCubemaps, 2),
			NumReflectionCaptureMips));
	
		// Allocate TextureCubeArray for the scene's reflection captures
		GRenderTargetPool.FindFreeElement(Desc, ReflectionEnvs, TEXT("ReflectionEnvs"));
	}
}

void FReflectionEnvironmentCubemapArray::ReleaseCubeArray()
{
	// it's unlikely we can reuse the TextureCubeArray so when we release it we want to really remove it
	GRenderTargetPool.FreeUnusedResource(ReflectionEnvs);
}

void FReflectionEnvironmentCubemapArray::ReleaseDynamicRHI()
{
	ReleaseCubeArray();
}

void FReflectionEnvironmentCubemapArray::UpdateMaxCubemaps(uint32 InMaxCubemaps)
{
	MaxCubemaps = InMaxCubemaps;

	// Reallocate the cubemap array
	if (IsInitialized())
	{
		UpdateRHI();
	}
	else
	{
		InitResource();
	}
}

struct FReflectionCaptureSortData
{
	uint32 Guid;
	FVector4 PositionAndRadius;
	FVector4 CaptureProperties;
	FMatrix BoxTransform;
	FVector4 BoxScales;
	FTexture* SM4FullHDRCubemap;

	bool operator < (const FReflectionCaptureSortData& Other) const
	{
		if( PositionAndRadius.W != Other.PositionAndRadius.W )
		{
			return PositionAndRadius.W < Other.PositionAndRadius.W;
		}
		else
		{
			return Guid < Other.Guid;
		}
	}
};

/** Per-reflection capture data needed by the shader. */
BEGIN_UNIFORM_BUFFER_STRUCT(FReflectionCaptureData,)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY(FVector4,PositionAndRadius,[GMaxNumReflectionCaptures])
	// R is brightness, G is array index, B is shape
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY(FVector4,CaptureProperties,[GMaxNumReflectionCaptures])
	// Stores the box transform for a box shape, other data is packed for other shapes
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY(FMatrix,BoxTransform,[GMaxNumReflectionCaptures])
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY(FVector4,BoxScales,[GMaxNumReflectionCaptures])
END_UNIFORM_BUFFER_STRUCT(FReflectionCaptureData)

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FReflectionCaptureData,TEXT("ReflectionCapture"));

/** Compute shader that does tiled deferred culling of reflection captures, then sorts and composites them. */
class FReflectionEnvironmentTiledDeferredCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FReflectionEnvironmentTiledDeferredCS,Global)
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), GReflectionEnvironmentTileSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), GReflectionEnvironmentTileSizeY);
		OutEnvironment.SetDefine(TEXT("MAX_CAPTURES"), GMaxNumReflectionCaptures);
		OutEnvironment.SetDefine(TEXT("TILED_DEFERRED_CULL_SHADER"), 1);
		OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	}

	FReflectionEnvironmentTiledDeferredCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		ReflectionEnvironmentColorTexture.Bind(Initializer.ParameterMap,TEXT("ReflectionEnvironmentColorTexture"));
		ReflectionEnvironmentColorSampler.Bind(Initializer.ParameterMap,TEXT("ReflectionEnvironmentColorSampler"));
		ScreenSpaceReflections.Bind(Initializer.ParameterMap, TEXT("ScreenSpaceReflections"));
		InSceneColor.Bind(Initializer.ParameterMap, TEXT("InSceneColor"));
		OutSceneColor.Bind(Initializer.ParameterMap, TEXT("OutSceneColor"));
		NumCaptures.Bind(Initializer.ParameterMap, TEXT("NumCaptures"));
		ViewDimensionsParameter.Bind(Initializer.ParameterMap, TEXT("ViewDimensions"));
		PreIntegratedGF.Bind(Initializer.ParameterMap, TEXT("PreIntegratedGF"));
		PreIntegratedGFSampler.Bind(Initializer.ParameterMap, TEXT("PreIntegratedGFSampler"));
		SkyLightParameters.Bind(Initializer.ParameterMap);
	}

	FReflectionEnvironmentTiledDeferredCS()
	{
	}

	void SetParameters(const FSceneView& View, FTextureRHIParamRef SSRTexture, FUnorderedAccessViewRHIParamRef OutSceneColorUAV)
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		FGlobalShader::SetParameters(ShaderRHI, View);
		DeferredParameters.Set(ShaderRHI, View);

		FScene* Scene = (FScene*)View.Family->Scene;
		FSceneRenderTargetItem& CubemapArray = Scene->ReflectionSceneData.CubemapArray.GetRenderTarget();

		SetTextureParameter(
			ShaderRHI, 
			ReflectionEnvironmentColorTexture, 
			ReflectionEnvironmentColorSampler, 
			TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), 
			// Set NULL when not valid to prevent d3d debug warnings
			Scene->ReflectionSceneData.CubemapArray.GetRenderTarget().IsValid() ? CubemapArray.ShaderResourceTexture : NULL);

		SetTextureParameter( ShaderRHI, ScreenSpaceReflections, SSRTexture );

		SetTextureParameter( ShaderRHI, InSceneColor, GSceneRenderTargets.SceneColor->GetRenderTargetItem().ShaderResourceTexture );
		OutSceneColor.SetTexture(ShaderRHI, NULL, OutSceneColorUAV);

		SetShaderValue(ShaderRHI, ViewDimensionsParameter, View.ViewRect);

		static TArray<FReflectionCaptureSortData> SortData;
		SortData.Reset(Scene->ReflectionSceneData.RegisteredReflectionCaptures.Num());

		check(Scene->ReflectionSceneData.CubemapArray.GetRenderTarget().IsValid());
		const int32 MaxCubemaps = Scene->ReflectionSceneData.CubemapArray.GetMaxCubemaps();

		// Pack only visible reflection captures into the uniform buffer, each with an index to its cubemap array entry
		for (int32 ReflectionProxyIndex = 0; ReflectionProxyIndex < Scene->ReflectionSceneData.RegisteredReflectionCaptures.Num() && SortData.Num() < GMaxNumReflectionCaptures; ReflectionProxyIndex++)
		{
			FReflectionCaptureProxy* CurrentCapture = Scene->ReflectionSceneData.RegisteredReflectionCaptures[ReflectionProxyIndex];
			// Find the cubemap index this component was allocated with
			const FCaptureComponentSceneState* ComponentStatePtr = Scene->ReflectionSceneData.AllocatedReflectionCaptureState.Find(CurrentCapture->Component);

			if (ComponentStatePtr)
			{
				int32 CubemapIndex = ComponentStatePtr->CaptureIndex;
				check(CubemapIndex < MaxCubemaps);

				FReflectionCaptureSortData NewSortEntry;

				NewSortEntry.SM4FullHDRCubemap = NULL;
				NewSortEntry.Guid = CurrentCapture->Guid;
				NewSortEntry.PositionAndRadius = FVector4(CurrentCapture->Position, CurrentCapture->InfluenceRadius);
				float ShapeTypeValue = (float)CurrentCapture->Shape;
				NewSortEntry.CaptureProperties = FVector4(CurrentCapture->Brightness, CubemapIndex, ShapeTypeValue, 0);

				if (CurrentCapture->Shape == EReflectionCaptureShape::Plane)
				{
					NewSortEntry.BoxTransform = FMatrix(
						FPlane(CurrentCapture->ReflectionPlane), 
						FPlane(CurrentCapture->ReflectionXAxisAndYScale), 
						FPlane(0, 0, 0, 0), 
						FPlane(0, 0, 0, 0));

					NewSortEntry.BoxScales = FVector4(0);
				}
				else
				{
					NewSortEntry.BoxTransform = CurrentCapture->BoxTransform;

					NewSortEntry.BoxScales = FVector4(CurrentCapture->BoxScales, CurrentCapture->BoxTransitionDistance);
				}

				SortData.Add(NewSortEntry);
			}
		}

		SortData.Sort();
		FReflectionCaptureData SamplePositionsBuffer;

		for (int32 CaptureIndex = 0; CaptureIndex < SortData.Num(); CaptureIndex++)
		{
			SamplePositionsBuffer.PositionAndRadius[CaptureIndex] = SortData[CaptureIndex].PositionAndRadius;
			SamplePositionsBuffer.CaptureProperties[CaptureIndex] = SortData[CaptureIndex].CaptureProperties;
			SamplePositionsBuffer.BoxTransform[CaptureIndex] = SortData[CaptureIndex].BoxTransform;
			SamplePositionsBuffer.BoxScales[CaptureIndex] = SortData[CaptureIndex].BoxScales;
		}

		SetUniformBufferParameterImmediate(ShaderRHI, GetUniformBufferParameter<FReflectionCaptureData>(), SamplePositionsBuffer);
		SetShaderValue(ShaderRHI, NumCaptures, SortData.Num());

		SetTextureParameter(ShaderRHI, PreIntegratedGF, PreIntegratedGFSampler, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(), GSystemTextures.PreintegratedGF->GetRenderTargetItem().ShaderResourceTexture);
	
		SkyLightParameters.SetParameters(ShaderRHI, Scene, View.Family->EngineShowFlags.SkyLighting);
	}

	void UnsetParameters()
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		OutSceneColor.UnsetUAV(ShaderRHI);
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << ReflectionEnvironmentColorTexture;
		Ar << ReflectionEnvironmentColorSampler;
		Ar << ScreenSpaceReflections;
		Ar << InSceneColor;
		Ar << OutSceneColor;
		Ar << NumCaptures;
		Ar << ViewDimensionsParameter;
		Ar << PreIntegratedGF;
		Ar << PreIntegratedGFSampler;
		Ar << SkyLightParameters;
		return bShaderHasOutdatedParameters;
	}

private:

	FDeferredPixelShaderParameters DeferredParameters;
	FShaderResourceParameter ReflectionEnvironmentColorTexture;
	FShaderResourceParameter ReflectionEnvironmentColorSampler;
	FShaderResourceParameter ScreenSpaceReflections;
	FShaderResourceParameter InSceneColor;
	FRWShaderParameter OutSceneColor;
	FShaderParameter NumCaptures;
	FShaderParameter ViewDimensionsParameter;
	FShaderResourceParameter PreIntegratedGF;
	FShaderResourceParameter PreIntegratedGFSampler;
	FSkyLightReflectionParameters SkyLightParameters;
};

template< uint32 bUseLightmaps >
class TReflectionEnvironmentTiledDeferredCS : public FReflectionEnvironmentTiledDeferredCS
{
	DECLARE_SHADER_TYPE(TReflectionEnvironmentTiledDeferredCS, Global);

	/** Default constructor. */
	TReflectionEnvironmentTiledDeferredCS() {}
public:
	TReflectionEnvironmentTiledDeferredCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	: FReflectionEnvironmentTiledDeferredCS(Initializer)
	{}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FReflectionEnvironmentTiledDeferredCS::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("USE_LIGHTMAPS"), bUseLightmaps);
	}
};

IMPLEMENT_SHADER_TYPE(template<>,TReflectionEnvironmentTiledDeferredCS<0>,TEXT("ReflectionEnvironmentComputeShaders"),TEXT("ReflectionEnvironmentTiledDeferredMain"),SF_Compute);
IMPLEMENT_SHADER_TYPE(template<>,TReflectionEnvironmentTiledDeferredCS<1>,TEXT("ReflectionEnvironmentComputeShaders"),TEXT("ReflectionEnvironmentTiledDeferredMain"),SF_Compute);

template< uint32 bSSR, uint32 bReflectionEnv, uint32 bSkylight >
class FReflectionApplyPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FReflectionApplyPS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("APPLY_SSR"), bSSR);
		OutEnvironment.SetDefine(TEXT("APPLY_REFLECTION_ENV"), bReflectionEnv);
		OutEnvironment.SetDefine(TEXT("APPLY_SKYLIGHT"), bSkylight);
	}

	/** Default constructor. */
	FReflectionApplyPS() {}

public:
	FDeferredPixelShaderParameters DeferredParameters;
	FSkyLightReflectionParameters SkyLightParameters;
	FShaderResourceParameter ReflectionEnvTexture;
	FShaderResourceParameter ReflectionEnvSampler;
	FShaderResourceParameter ScreenSpaceReflectionsTexture;
	FShaderResourceParameter ScreenSpaceReflectionsSampler;
	FShaderResourceParameter PreIntegratedGF;
	FShaderResourceParameter PreIntegratedGFSampler;

	/** Initialization constructor. */
	FReflectionApplyPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		SkyLightParameters.Bind(Initializer.ParameterMap);
		ReflectionEnvTexture.Bind(Initializer.ParameterMap,TEXT("ReflectionEnvTexture"));
		ReflectionEnvSampler.Bind(Initializer.ParameterMap,TEXT("ReflectionEnvSampler"));
		ScreenSpaceReflectionsTexture.Bind(Initializer.ParameterMap,TEXT("ScreenSpaceReflectionsTexture"));
		ScreenSpaceReflectionsSampler.Bind(Initializer.ParameterMap,TEXT("ScreenSpaceReflectionsSampler"));
		PreIntegratedGF.Bind(Initializer.ParameterMap, TEXT("PreIntegratedGF"));
		PreIntegratedGFSampler.Bind(Initializer.ParameterMap, TEXT("PreIntegratedGFSampler"));
	}

	void SetParameters( const FSceneView& View, FTextureRHIParamRef ReflectionEnv, FTextureRHIParamRef ScreenSpaceReflections )
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(ShaderRHI, View);
		DeferredParameters.Set(ShaderRHI, View);
		SkyLightParameters.SetParameters(ShaderRHI, (FScene*)View.Family->Scene, true);
		
		SetTextureParameter( ShaderRHI, ReflectionEnvTexture, ReflectionEnvSampler, TStaticSamplerState<SF_Point>::GetRHI(), ReflectionEnv );
		SetTextureParameter( ShaderRHI, ScreenSpaceReflectionsTexture, ScreenSpaceReflectionsSampler, TStaticSamplerState<SF_Point>::GetRHI(), ScreenSpaceReflections );
		SetTextureParameter( ShaderRHI, PreIntegratedGF, PreIntegratedGFSampler, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(), GSystemTextures.PreintegratedGF->GetRenderTargetItem().ShaderResourceTexture );
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << SkyLightParameters;
		Ar << ReflectionEnvTexture;
		Ar << ReflectionEnvSampler;
		Ar << ScreenSpaceReflectionsTexture;
		Ar << ScreenSpaceReflectionsSampler;
		Ar << PreIntegratedGF;
		Ar << PreIntegratedGFSampler;
		return bShaderHasOutdatedParameters;
	}
};

// Typedef is necessary because the C preprocessor thinks the comma in the template parameter list is a comma in the macro parameter list.
#define IMPLEMENT_REFLECTION_APPLY_PIXELSHADER_TYPE(A, B, C) \
	typedef FReflectionApplyPS<A,B,C> FReflectionApplyPS##A##B##C; \
	IMPLEMENT_SHADER_TYPE(template<>,FReflectionApplyPS##A##B##C,TEXT("ReflectionEnvironmentShaders"),TEXT("ReflectionApplyPS"),SF_Pixel);

IMPLEMENT_REFLECTION_APPLY_PIXELSHADER_TYPE(0,0,0);
IMPLEMENT_REFLECTION_APPLY_PIXELSHADER_TYPE(0,0,1);
IMPLEMENT_REFLECTION_APPLY_PIXELSHADER_TYPE(0,1,0);
IMPLEMENT_REFLECTION_APPLY_PIXELSHADER_TYPE(0,1,1);
IMPLEMENT_REFLECTION_APPLY_PIXELSHADER_TYPE(1,0,0);
IMPLEMENT_REFLECTION_APPLY_PIXELSHADER_TYPE(1,0,1);
IMPLEMENT_REFLECTION_APPLY_PIXELSHADER_TYPE(1,1,0);
IMPLEMENT_REFLECTION_APPLY_PIXELSHADER_TYPE(1,1,1);


class FReflectionCaptureSpecularBouncePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FReflectionCaptureSpecularBouncePS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	/** Default constructor. */
	FReflectionCaptureSpecularBouncePS() {}

public:
	FDeferredPixelShaderParameters DeferredParameters;

	/** Initialization constructor. */
	FReflectionCaptureSpecularBouncePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
	}

	void SetParameters(const FSceneView& View)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(ShaderRHI, View);

		DeferredParameters.Set(ShaderRHI, View);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(,FReflectionCaptureSpecularBouncePS,TEXT("ReflectionEnvironmentShaders"),TEXT("SpecularBouncePS"),SF_Pixel);

template<bool bSphereCapture>
class TStandardDeferredReflectionPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(TStandardDeferredReflectionPS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("SPHERE_CAPTURE"), (uint32)bSphereCapture);
		OutEnvironment.SetDefine(TEXT("BOX_CAPTURE"), (uint32)!bSphereCapture);
	}

	/** Default constructor. */
	TStandardDeferredReflectionPS() {}

	/** Initialization constructor. */
	TStandardDeferredReflectionPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		CapturePositionAndRadius.Bind(Initializer.ParameterMap, TEXT("CapturePositionAndRadius"));
		CaptureProperties.Bind(Initializer.ParameterMap, TEXT("CaptureProperties"));
		CaptureBoxTransform.Bind(Initializer.ParameterMap, TEXT("CaptureBoxTransform"));
		CaptureBoxScales.Bind(Initializer.ParameterMap, TEXT("CaptureBoxScales"));
		ReflectionEnvironmentColorTexture.Bind(Initializer.ParameterMap, TEXT("ReflectionEnvironmentColorTexture"));
		ReflectionEnvironmentColorSampler.Bind(Initializer.ParameterMap, TEXT("ReflectionEnvironmentColorSampler"));
		DeferredParameters.Bind(Initializer.ParameterMap);
	}

	void SetParameters(const FSceneView& View, const FReflectionCaptureSortData& SortData)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();
		FGlobalShader::SetParameters(ShaderRHI, View);

		SetTextureParameter(ShaderRHI, ReflectionEnvironmentColorTexture, ReflectionEnvironmentColorSampler, TStaticSamplerState<SF_Trilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(), SortData.SM4FullHDRCubemap->TextureRHI);

		DeferredParameters.Set(ShaderRHI, View);
		SetShaderValue(ShaderRHI, CapturePositionAndRadius, SortData.PositionAndRadius);
		SetShaderValue(ShaderRHI, CaptureProperties, SortData.CaptureProperties);
		SetShaderValue(ShaderRHI, CaptureBoxTransform, SortData.BoxTransform);
		SetShaderValue(ShaderRHI, CaptureBoxScales, SortData.BoxScales);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << CapturePositionAndRadius;
		Ar << CaptureProperties;
		Ar << CaptureBoxTransform;
		Ar << CaptureBoxScales;
		Ar << ReflectionEnvironmentColorTexture;
		Ar << ReflectionEnvironmentColorSampler;
		Ar << DeferredParameters;
		return bShaderHasOutdatedParameters;
	}

private:

	FShaderParameter CapturePositionAndRadius;
	FShaderParameter CaptureProperties;
	FShaderParameter CaptureBoxTransform;
	FShaderParameter CaptureBoxScales;
	FShaderResourceParameter ReflectionEnvironmentColorTexture;
	FShaderResourceParameter ReflectionEnvironmentColorSampler;
	FDeferredPixelShaderParameters DeferredParameters;
};

IMPLEMENT_SHADER_TYPE(template<>,TStandardDeferredReflectionPS<true>,TEXT("ReflectionEnvironmentShaders"),TEXT("StandardDeferredReflectionPS"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>,TStandardDeferredReflectionPS<false>,TEXT("ReflectionEnvironmentShaders"),TEXT("StandardDeferredReflectionPS"),SF_Pixel);


/** Renders deferred reflections for the scene. */
void FDeferredShadingSceneRenderer::RenderDeferredReflections()
{
	if (IsSimpleDynamicLightingEnabled() || ViewFamily.EngineShowFlags.VisualizeLightCulling)
	{
		return;
	}

	bool bAnyViewIsReflectionCapture = false;
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		const FViewInfo& View = Views[ViewIndex];
		bAnyViewIsReflectionCapture = bAnyViewIsReflectionCapture || View.bIsReflectionCapture;
	}

	const uint32 bReflectionEnv = IsReflectionEnvironmentAvailable()
		&& Scene->ReflectionSceneData.RegisteredReflectionCaptures.Num()
		&& ViewFamily.EngineShowFlags.ReflectionEnvironment;

	const uint32 bSkyLight = Scene->SkyLight
		&& Scene->SkyLight->ProcessedTexture
		&& ViewFamily.EngineShowFlags.SkyLighting;

	const uint32 bUseLightmaps = GDiffuseFromCaptures == 0;

	const bool bReflectionsWithCompute = GRHIFeatureLevel == ERHIFeatureLevel::SM5 && Scene->ReflectionSceneData.CubemapArray.IsValid();

	TRefCountPtr<IPooledRenderTarget>	NewSceneColor;
	FUnorderedAccessViewRHIRef			NewSceneColorUAV;
	if( bReflectionEnv && bReflectionsWithCompute && !bAnyViewIsReflectionCapture )
	{
		GSceneRenderTargets.ResolveSceneColor(FResolveRect(0, 0, ViewFamily.FamilySizeX, ViewFamily.FamilySizeY));

		FPooledRenderTargetDesc Desc = GSceneRenderTargets.SceneColor->GetDesc();
		Desc.Flags |= TexCreate_UAV;

		GRenderTargetPool.FindFreeElement( Desc, NewSceneColor, TEXT("SceneColor") );
		NewSceneColorUAV = RHICreateUnorderedAccessView( NewSceneColor->GetRenderTargetItem().TargetableTexture );
	}

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		const FViewInfo& View = Views[ViewIndex];

		if (bAnyViewIsReflectionCapture)
		{
			// We're currently capturing a reflection capture, output SpecularColor * IndirectDiffuseGI for metals so they are not black in reflections,
			// Since we don't have multiple bounce specular reflections
			GSceneRenderTargets.BeginRenderingSceneColor();

			RHISetViewport( View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f );
			RHISetRasterizerState( TStaticRasterizerState< FM_Solid, CM_None >::GetRHI() );
			RHISetDepthStencilState( TStaticDepthStencilState< false, CF_Always >::GetRHI() );
			RHISetBlendState( TStaticBlendState< CW_RGB, BO_Add, BF_One, BF_One >::GetRHI() );

			TShaderMapRef< FPostProcessVS > VertexShader( GetGlobalShaderMap() );
			TShaderMapRef< FReflectionCaptureSpecularBouncePS > PixelShader( GetGlobalShaderMap() );

			static FGlobalBoundShaderState BoundShaderState;
			SetGlobalBoundShaderState( BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader );

			PixelShader->SetParameters( View );

			DrawRectangle( 
				0, 0, 
				View.ViewRect.Width(), View.ViewRect.Height(),
				0, 0, 
				View.ViewRect.Width(), View.ViewRect.Height(),
				FIntPoint(View.ViewRect.Width(), View.ViewRect.Height()),
				GSceneRenderTargets.GetBufferSizeXY(),
				*VertexShader,
				EDRF_UseTriangleOptimization);
		}
		else
		{
			const uint32 bSSR = DoScreenSpaceReflections(Views[0]);

			bool bRequiresApply = bSSR || bReflectionEnv || bSkyLight;

			TRefCountPtr<IPooledRenderTarget> SSROutput = GSystemTextures.BlackDummy;
			if( bSSR )
			{
				ScreenSpaceReflections( View, SSROutput );
			}

			if( bReflectionEnv )
			{
				// If we are in SM5, use the compute shader gather method
				if (GRHIFeatureLevel == ERHIFeatureLevel::SM5 && Scene->ReflectionSceneData.CubemapArray.IsValid())
				{
					// Render the reflection environment with tiled deferred culling
					SCOPED_DRAW_EVENT(ReflectionEnvironmentGather, DEC_SCENE_ITEMS);
				
					RHISetRenderTarget(NULL, NULL);

					FReflectionEnvironmentTiledDeferredCS* ComputeShader = NULL;
					if( bUseLightmaps )
					{
						ComputeShader = *TShaderMapRef< TReflectionEnvironmentTiledDeferredCS<1> >( GetGlobalShaderMap() );
					}
					else
					{
						ComputeShader = *TShaderMapRef< TReflectionEnvironmentTiledDeferredCS<0> >( GetGlobalShaderMap() );
					}

					RHISetComputeShader(ComputeShader->GetComputeShader());

					ComputeShader->SetParameters(View, SSROutput->GetRenderTargetItem().ShaderResourceTexture, NewSceneColorUAV);
					//ComputeShader->SetParameters(View, DestRenderTarget.UAV);

					uint32 GroupSizeX = (View.ViewRect.Size().X + GReflectionEnvironmentTileSizeX - 1) / GReflectionEnvironmentTileSizeX;
					uint32 GroupSizeY = (View.ViewRect.Size().Y + GReflectionEnvironmentTileSizeY - 1) / GReflectionEnvironmentTileSizeY;
					DispatchComputeShader(ComputeShader, GroupSizeX, GroupSizeY, 1);

					ComputeShader->UnsetParameters();

					const FSceneRenderTargetItem& DestRenderTarget = NewSceneColor->GetRenderTargetItem();
					RHICopyToResolveTarget( DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams() );

					GSceneRenderTargets.SceneColor = NewSceneColor;
					check(GSceneRenderTargets.SceneColor);

					bRequiresApply = false;
				}
				// In SM4 use standard deferred shading to composite reflection capture contribution
				else if (GRHIFeatureLevel == ERHIFeatureLevel::SM4)
				{
					// TODO sort once per view
					static TArray<FReflectionCaptureSortData> SortData;
					SortData.Reset(Scene->ReflectionSceneData.RegisteredReflectionCaptures.Num());

					// Gather visible reflection capture data
					for (int32 ReflectionProxyIndex = 0; ReflectionProxyIndex < Scene->ReflectionSceneData.RegisteredReflectionCaptures.Num() && SortData.Num() < GMaxNumReflectionCaptures; ReflectionProxyIndex++)
					{
						FReflectionCaptureProxy* CurrentCapture = Scene->ReflectionSceneData.RegisteredReflectionCaptures[ReflectionProxyIndex];
						FReflectionCaptureSortData NewSortEntry;

						NewSortEntry.SM4FullHDRCubemap = CurrentCapture->SM4FullHDRCubemap;
						NewSortEntry.Guid = CurrentCapture->Guid;
						NewSortEntry.PositionAndRadius = FVector4(CurrentCapture->Position, CurrentCapture->InfluenceRadius);
						float ShapeTypeValue = (float)CurrentCapture->Shape;
						NewSortEntry.CaptureProperties = FVector4(CurrentCapture->Brightness, 0, ShapeTypeValue, 0);

						if (CurrentCapture->Shape == EReflectionCaptureShape::Plane)
						{
							NewSortEntry.BoxTransform = FMatrix(
								FPlane(CurrentCapture->ReflectionPlane), 
								FPlane(CurrentCapture->ReflectionXAxisAndYScale), 
								FPlane(0, 0, 0, 0), 
								FPlane(0, 0, 0, 0));

							NewSortEntry.BoxScales = FVector4(0);
						}
						else
						{
							NewSortEntry.BoxTransform = CurrentCapture->BoxTransform;

							NewSortEntry.BoxScales = FVector4(CurrentCapture->BoxScales, CurrentCapture->BoxTransitionDistance);
						}

						SortData.Add(NewSortEntry);
					}

					SortData.Sort();


					SCOPED_DRAW_EVENT(StandardDeferredReflectionEnvironment, DEC_SCENE_ITEMS);
				
					RHISetRenderTarget(GSceneRenderTargets.LightAccumulation->GetRenderTargetItem().TargetableTexture, NULL);

					// Clear to no reflection contribution, alpha of 1 indicates full background contribution
					RHIClear(true, FLinearColor(0, 0, 0, 1), false, 0, false, 0, View.ViewRect);

					RHISetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);

					// rgb accumulates reflection contribution front to back, alpha accumulates (1 - alpha0) * (1 - alpha 1)...
					RHISetBlendState(TStaticBlendState<CW_RGBA, BO_Add, BF_DestAlpha, BF_One, BO_Add, BF_Zero, BF_InverseSourceAlpha>::GetRHI());
								 
					for (int32 ReflectionCaptureIndex = 0; ReflectionCaptureIndex < SortData.Num(); ReflectionCaptureIndex++)
					{
						const FReflectionCaptureSortData& ReflectionCapture = SortData[ReflectionCaptureIndex];

						if (ReflectionCapture.SM4FullHDRCubemap)
						{
							const FSphere LightBounds(ReflectionCapture.PositionAndRadius, ReflectionCapture.PositionAndRadius.W);

							TShaderMapRef<TDeferredLightVS<true> > VertexShader(GetGlobalShaderMap());

							// Use the appropriate shader for the capture shape
							if (ReflectionCapture.CaptureProperties.Z == 0)
							{
								TShaderMapRef<TStandardDeferredReflectionPS<true> > PixelShader(GetGlobalShaderMap());

								static FGlobalBoundShaderState BoundShaderState;
								SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

								PixelShader->SetParameters(View, ReflectionCapture);
							}
							else 
							{
								TShaderMapRef<TStandardDeferredReflectionPS<false> > PixelShader(GetGlobalShaderMap());

								static FGlobalBoundShaderState BoundShaderState;
								SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

								PixelShader->SetParameters(View, ReflectionCapture);
							}

							SetBoundingGeometryRasterizerAndDepthState(View, LightBounds);
							VertexShader->SetSimpleLightParameters(View, LightBounds);
							StencilingGeometry::DrawSphere();
						}
					}

					GRenderTargetPool.VisualizeTexture.SetCheckPoint( GSceneRenderTargets.LightAccumulation );
				}
			}
			
			if( bRequiresApply )
			{
				// Apply reflections to screen
				SCOPED_DRAW_EVENT(ReflectionApply, DEC_SCENE_ITEMS);

				GSceneRenderTargets.BeginRenderingSceneColor();

				RHISetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);
				RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
				RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

				if (GetReflectionEnvironmentCVar() == 2)
				{
					RHISetBlendState(TStaticBlendState<>::GetRHI());
				}
				else
				{
					RHISetBlendState(TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One>::GetRHI());
				}

				TShaderMapRef< FPostProcessVS > VertexShader( GetGlobalShaderMap() );

				#define CASE(A,B,C) \
					case ( (A << 2) | (B << 1) | C ): \
					{ \
						TShaderMapRef< FReflectionApplyPS<A,B,C> > PixelShader(GetGlobalShaderMap()); \
						static FGlobalBoundShaderState BoundShaderState; \
						SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader); \
						PixelShader->SetParameters( View, GSceneRenderTargets.GetLightAccumulationTexture(), SSROutput->GetRenderTargetItem().ShaderResourceTexture ); \
					}; \
					break

				switch( (bSSR << 2) | (bReflectionEnv << 1) | bSkyLight )
				{
					CASE(0,0,0);
					CASE(0,0,1);
					CASE(0,1,0);
					CASE(0,1,1);
					CASE(1,0,0);
					CASE(1,0,1);
					CASE(1,1,0);
					CASE(1,1,1);
				}
				#undef CASE

				DrawRectangle( 
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

	NewSceneColorUAV.SafeRelease();
}
