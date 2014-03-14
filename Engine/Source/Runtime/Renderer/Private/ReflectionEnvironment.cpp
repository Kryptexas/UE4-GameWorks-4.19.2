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
#include "EngineDecalClasses.h"
#include "ScreenSpaceReflections.h"
#include "PostProcessTemporalAA.h"
#include "PostProcessDownsample.h"
#include "ReflectionEnvironment.h"
#include "LightRendering.h"

/** Tile size for the reflection environment compute shader, tweaked for 680 GTX. */
const int32 GReflectionEnvironmentTileSizeX = 16;
const int32 GReflectionEnvironmentTileSizeY = 16;
extern ENGINE_API int32 GReflectionCaptureSize;

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

class FSkyLightReflectionParameters
{
public:

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		SkyLightCubemap.Bind(ParameterMap, TEXT("SkyLightCubemap"));
		SkyLightCubemapSampler.Bind(ParameterMap, TEXT("SkyLightCubemapSampler"));
		SkyLightParameters.Bind(ParameterMap, TEXT("SkyLightParameters"));
	}

	template<typename TParamRef>
	void SetParameters(const TParamRef& ShaderRHI, FScene* Scene, bool bApplySkyLight)
	{
		FTexture* SkyLightTextureResource = GBlackTextureCube;
		float ApplySkyLightMask = 0;

		if (Scene->SkyLight 
			&& Scene->SkyLight->ProcessedTexture
			&& bApplySkyLight)
		{
			SkyLightTextureResource = Scene->SkyLight->ProcessedTexture;
			ApplySkyLightMask = 1;
		}

		SetTextureParameter(ShaderRHI, SkyLightCubemap, SkyLightCubemapSampler, SkyLightTextureResource);

		float SkyMipCount = 1;

		if (SkyLightTextureResource)
		{
			int32 CubemapWidth = SkyLightTextureResource->GetSizeX();
			SkyMipCount = FMath::Log2(CubemapWidth) + 1.0f;
		}

		const FVector2D SkyParametersValue(SkyMipCount - 1.0f, ApplySkyLightMask);
		SetShaderValue(ShaderRHI, SkyLightParameters, SkyParametersValue);
	}

	friend FArchive& operator<<(FArchive& Ar,FSkyLightReflectionParameters& P)
	{
		Ar << P.SkyLightCubemap << P.SkyLightCubemapSampler << P.SkyLightParameters;
		return Ar;
	}

	FShaderResourceParameter SkyLightCubemap;
	FShaderResourceParameter SkyLightCubemapSampler;
	FShaderParameter SkyLightParameters;
};

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
		LightAccumulation.Bind(Initializer.ParameterMap, TEXT("LightAccumulation"));
		NumCaptures.Bind(Initializer.ParameterMap, TEXT("NumCaptures"));
		ViewDimensionsParameter.Bind(Initializer.ParameterMap, TEXT("ViewDimensions"));
		PreIntegratedGF.Bind(Initializer.ParameterMap, TEXT("PreIntegratedGF"));
		PreIntegratedGFSampler.Bind(Initializer.ParameterMap, TEXT("PreIntegratedGFSampler"));
		SkyLightParameters.Bind(Initializer.ParameterMap);
	}

	FReflectionEnvironmentTiledDeferredCS()
	{
	}

	void SetParameters(const FSceneView& View)
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

		LightAccumulation.SetTexture(ShaderRHI, GSceneRenderTargets.GetLightAccumulationTexture(), GSceneRenderTargets.LightAccumulationUAV);

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

		LightAccumulation.UnsetUAV(ShaderRHI);
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << ReflectionEnvironmentColorTexture;
		Ar << ReflectionEnvironmentColorSampler;
		Ar << LightAccumulation;
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
	FRWShaderParameter LightAccumulation;
	FShaderParameter NumCaptures;
	FShaderParameter ViewDimensionsParameter;
	FShaderResourceParameter PreIntegratedGF;
	FShaderResourceParameter PreIntegratedGFSampler;
	FSkyLightReflectionParameters SkyLightParameters;
};

IMPLEMENT_SHADER_TYPE(,FReflectionEnvironmentTiledDeferredCS,TEXT("ReflectionEnvironmentComputeShaders"),TEXT("ReflectionEnvironmentTiledDeferredMain"),SF_Compute);


class FReflectionApplyPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FReflectionApplyPS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	/** Default constructor. */
	FReflectionApplyPS() {}

public:
	FDeferredPixelShaderParameters DeferredParameters;
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
		
		SetTextureParameter( ShaderRHI, ReflectionEnvTexture, ReflectionEnvSampler, TStaticSamplerState<SF_Point>::GetRHI(), ReflectionEnv );
		SetTextureParameter( ShaderRHI, ScreenSpaceReflectionsTexture, ScreenSpaceReflectionsSampler, TStaticSamplerState<SF_Point>::GetRHI(), ScreenSpaceReflections );
		SetTextureParameter( ShaderRHI, PreIntegratedGF, PreIntegratedGFSampler, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(), GSystemTextures.PreintegratedGF->GetRenderTargetItem().ShaderResourceTexture );
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << ReflectionEnvTexture;
		Ar << ReflectionEnvSampler;
		Ar << ScreenSpaceReflectionsTexture;
		Ar << ScreenSpaceReflectionsSampler;
		Ar << PreIntegratedGF;
		Ar << PreIntegratedGFSampler;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(,FReflectionApplyPS,TEXT("ReflectionEnvironmentShaders"),TEXT("ReflectionApplyPS"),SF_Pixel);


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
		PreIntegratedGF.Bind(Initializer.ParameterMap, TEXT("PreIntegratedGF"));
		PreIntegratedGFSampler.Bind(Initializer.ParameterMap, TEXT("PreIntegratedGFSampler"));
		DeferredParameters.Bind(Initializer.ParameterMap);
	}

	void SetParameters(const FSceneView& View, const FReflectionCaptureSortData& SortData)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();
		FGlobalShader::SetParameters(ShaderRHI, View);

		SetTextureParameter(ShaderRHI, PreIntegratedGF, PreIntegratedGFSampler, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(), GSystemTextures.PreintegratedGF->GetRenderTargetItem().ShaderResourceTexture);
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
		Ar << PreIntegratedGF;
		Ar << PreIntegratedGFSampler;
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
	FShaderResourceParameter PreIntegratedGF;
	FShaderResourceParameter PreIntegratedGFSampler;
	FDeferredPixelShaderParameters DeferredParameters;
};

IMPLEMENT_SHADER_TYPE(template<>,TStandardDeferredReflectionPS<true>,TEXT("ReflectionEnvironmentShaders"),TEXT("StandardDeferredReflectionPS"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>,TStandardDeferredReflectionPS<false>,TEXT("ReflectionEnvironmentShaders"),TEXT("StandardDeferredReflectionPS"),SF_Pixel);

template<bool bApplySSR>
class TSkyLightSpecularPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(TSkyLightSpecularPS, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("APPLY_SSR"), (uint32)bApplySSR);
	}

	/** Default constructor. */
	TSkyLightSpecularPS() {}

	/** Initialization constructor. */
	TSkyLightSpecularPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PreIntegratedGF.Bind(Initializer.ParameterMap, TEXT("PreIntegratedGF"));
		PreIntegratedGFSampler.Bind(Initializer.ParameterMap, TEXT("PreIntegratedGFSampler"));
		SkyLightParameters.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		ScreenSpaceReflectionsTexture.Bind(Initializer.ParameterMap,TEXT("ScreenSpaceReflectionsTexture"));
		ScreenSpaceReflectionsSampler.Bind(Initializer.ParameterMap,TEXT("ScreenSpaceReflectionsSampler"));
	}

	void SetParameters(const FSceneView& View, FTextureRHIParamRef ScreenSpaceReflections)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(ShaderRHI, View);
		
		SetTextureParameter( ShaderRHI, PreIntegratedGF, PreIntegratedGFSampler, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(), GSystemTextures.PreintegratedGF->GetRenderTargetItem().ShaderResourceTexture );

		SkyLightParameters.SetParameters(ShaderRHI, (FScene*)View.Family->Scene, true);
		DeferredParameters.Set(ShaderRHI, View);

		SetTextureParameter( ShaderRHI, ScreenSpaceReflectionsTexture, ScreenSpaceReflectionsSampler, TStaticSamplerState<SF_Point>::GetRHI(), ScreenSpaceReflections );
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PreIntegratedGF;
		Ar << PreIntegratedGFSampler;
		Ar << SkyLightParameters;
		Ar << DeferredParameters;
		Ar << ScreenSpaceReflectionsTexture;
		Ar << ScreenSpaceReflectionsSampler;
		return bShaderHasOutdatedParameters;
	}

private:

	FShaderResourceParameter PreIntegratedGF;
	FShaderResourceParameter PreIntegratedGFSampler;
	FSkyLightReflectionParameters SkyLightParameters;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderResourceParameter ScreenSpaceReflectionsTexture;
	FShaderResourceParameter ScreenSpaceReflectionsSampler;
};

IMPLEMENT_SHADER_TYPE(template<>,TSkyLightSpecularPS<true>,TEXT("ReflectionEnvironmentShaders"),TEXT("SkyLightSpecularPS"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>,TSkyLightSpecularPS<false>,TEXT("ReflectionEnvironmentShaders"),TEXT("SkyLightSpecularPS"),SF_Pixel);

FGlobalBoundShaderState SkySpecularBoundShaderState;

/** Renders deferred reflections for the scene. */
void FDeferredShadingSceneRenderer::RenderDeferredReflections()
{
	if (!IsSimpleDynamicLightingEnabled() && !ViewFamily.EngineShowFlags.VisualizeLightCulling)
	{
		// todo: support multiple views
		const bool bSSR = DoScreenSpaceReflections(Views[0]);

		if (IsReflectionEnvironmentAvailable()
			&& Scene->ReflectionSceneData.RegisteredReflectionCaptures.Num()
			&& ViewFamily.EngineShowFlags.ReflectionEnvironment)
		{
			bool bAnyViewIsReflectionCapture = false;

			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				const FViewInfo& View = Views[ViewIndex];
				bAnyViewIsReflectionCapture = bAnyViewIsReflectionCapture || View.bIsReflectionCapture;
			}

			if (bAnyViewIsReflectionCapture)
			{
				// We're currently capturing a reflection capture, output SpecularColor * IndirectDiffuseGI for metals so they are not black in reflections,
				// Since we don't have multiple bounce specular reflections
				GSceneRenderTargets.BeginRenderingSceneColor();

				for( int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++ )
				{
					const FViewInfo& View = Views[ViewIndex];

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
						EDRF_UseTriangleOptimization);
				}
			}
			else
			{
				// If we are in SM5, use the compute shader gather method
				if (GRHIFeatureLevel == ERHIFeatureLevel::SM5
					&& Scene->ReflectionSceneData.CubemapArray.IsValid())
				{
					// Render the reflection environment with tiled deferred culling
					SCOPED_DRAW_EVENT(ReflectionEnvironmentGather, DEC_SCENE_ITEMS);

					RHISetRenderTarget(NULL, NULL);

					for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
					{
						const FViewInfo& View = Views[ViewIndex];

						TShaderMapRef< FReflectionEnvironmentTiledDeferredCS> ComputeShader(GetGlobalShaderMap());
						RHISetComputeShader(ComputeShader->GetComputeShader());

						ComputeShader->SetParameters(View);

						uint32 GroupSizeX = (View.ViewRect.Size().X + GReflectionEnvironmentTileSizeX - 1) / GReflectionEnvironmentTileSizeX;
						uint32 GroupSizeY = (View.ViewRect.Size().Y + GReflectionEnvironmentTileSizeY - 1) / GReflectionEnvironmentTileSizeY;
						DispatchComputeShader(*ComputeShader, GroupSizeX, GroupSizeY, 1);

						ComputeShader->UnsetParameters();
					}
				}
				// In SM4 use standard deferred shading to composite reflection capture contribution
				else if (GRHIFeatureLevel == ERHIFeatureLevel::SM4)
				{
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

					{
						RHISetRenderTarget(GSceneRenderTargets.LightAccumulation->GetRenderTargetItem().TargetableTexture, NULL);

						// Clear to no reflection contribution, alpha of 1 indicates full background contribution
						RHIClear(true, FLinearColor(0, 0, 0, 1), false, 0, false, 0, FIntRect());

						for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
						{
							FViewInfo& View = Views[ViewIndex];

							{
								SCOPED_DRAW_EVENT(StandardDeferredReflectionEnvironment, DEC_SCENE_ITEMS);
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
							}

							if (Scene->SkyLight
								&& Scene->SkyLight->ProcessedTexture
								&& ViewFamily.EngineShowFlags.SkyLighting)
							{
								SCOPED_DRAW_EVENT(SkyLightSpecularComposite, DEC_SCENE_ITEMS);

								RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
								RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
								RHISetBlendState(TStaticBlendState<CW_RGBA, BO_Add, BF_DestAlpha, BF_One, BO_Add, BF_Zero, BF_InverseSourceAlpha>::GetRHI());

								// Composite skylight specular, masked by accumulated reflection capture coverage
								TShaderMapRef< FPostProcessVS >	 VertexShader( GetGlobalShaderMap() );
								// Don't apply SSR in this sky light specular pass, that will happen in the reflection apply
								TShaderMapRef< TSkyLightSpecularPS<false> > PixelShader( GetGlobalShaderMap() );

								static FGlobalBoundShaderState BoundShaderState;
								SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

								PixelShader->SetParameters(View, NULL);

								DrawRectangle( 
									0, 0, 
									View.ViewRect.Width(), View.ViewRect.Height(),
									View.ViewRect.Min.X, View.ViewRect.Min.Y, 
									View.ViewRect.Width(), View.ViewRect.Height(),
									FIntPoint(View.ViewRect.Width(), View.ViewRect.Height()),
									GSceneRenderTargets.GetBufferSizeXY());
							}
						}
					}
				}

				if( bSSR )
				{
					GSceneRenderTargets.ResolveSceneColor(FResolveRect(0, 0, ViewFamily.FamilySizeX, ViewFamily.FamilySizeY));
				}

				for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
				{
					const FViewInfo& View = Views[ViewIndex];

					FTextureRHIParamRef SSRTexture = GSystemTextures.BlackDummy->GetRenderTargetItem().ShaderResourceTexture;

					TRefCountPtr<IPooledRenderTarget> SSROutput;
					if( bSSR )
					{
						SCOPED_DRAW_EVENT(ScreenSpaceReflections, DEC_SCENE_ITEMS);

						ScreenSpaceReflections( View, SSROutput );
						SSRTexture = SSROutput->GetRenderTargetItem().ShaderResourceTexture;
					}

					if (View.Family->EngineShowFlags.ReflectionEnvironment)
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

						TShaderMapRef< FPostProcessVS >		VertexShader( GetGlobalShaderMap() );
						TShaderMapRef< FReflectionApplyPS >	PixelShader( GetGlobalShaderMap() );

						static FGlobalBoundShaderState BoundShaderState;
						SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

						PixelShader->SetParameters( View, GSceneRenderTargets.GetLightAccumulationTexture(), SSRTexture );

						DrawRectangle( 
							0, 0, 
							View.ViewRect.Width(), View.ViewRect.Height(),
							View.ViewRect.Min.X, View.ViewRect.Min.Y, 
							View.ViewRect.Width(), View.ViewRect.Height(),
							FIntPoint(View.ViewRect.Width(), View.ViewRect.Height()),
							GSceneRenderTargets.GetBufferSizeXY());
					}
				}
			}
		}
		else if (Scene->SkyLight
			&& Scene->SkyLight->ProcessedTexture
			&& ViewFamily.EngineShowFlags.SkyLighting
			&& GRHIFeatureLevel >= ERHIFeatureLevel::SM4)
		{
			// Composite sky light specular by itself if the reflection environment pass is not going to be run
			SCOPED_DRAW_EVENT(SkyLightSpecularApply, DEC_SCENE_ITEMS);

			if( bSSR )
			{
				GSceneRenderTargets.ResolveSceneColor(FResolveRect(0, 0, ViewFamily.FamilySizeX, ViewFamily.FamilySizeY));
			}

			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				const FViewInfo& View = Views[ViewIndex];

				FTextureRHIParamRef SSRTexture = GSystemTextures.BlackDummy->GetRenderTargetItem().ShaderResourceTexture;

				TRefCountPtr<IPooledRenderTarget> SSROutput;
				if( bSSR )
				{
					SCOPED_DRAW_EVENT(ScreenSpaceReflections, DEC_SCENE_ITEMS);

					ScreenSpaceReflections( View, SSROutput );
					SSRTexture = SSROutput->GetRenderTargetItem().ShaderResourceTexture;
				}

				GSceneRenderTargets.BeginRenderingSceneColor();

				RHISetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);
				RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
				RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
				RHISetBlendState(TStaticBlendState<CW_RGB, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One>::GetRHI());

				TShaderMapRef< FPostProcessVS >	 VertexShader( GetGlobalShaderMap() );
				TShaderMapRef< TSkyLightSpecularPS<true> > PixelShader( GetGlobalShaderMap() );

				SetGlobalBoundShaderState(SkySpecularBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

				PixelShader->SetParameters(View, SSRTexture);

				DrawRectangle( 
					0, 0, 
					View.ViewRect.Width(), View.ViewRect.Height(),
					View.ViewRect.Min.X, View.ViewRect.Min.Y, 
					View.ViewRect.Width(), View.ViewRect.Height(),
					FIntPoint(View.ViewRect.Width(), View.ViewRect.Height()),
					GSceneRenderTargets.GetBufferSizeXY());
			}
		}
		else
		{
			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				const FViewInfo& View = Views[ViewIndex];

				FTextureRHIParamRef SSRTexture = GSystemTextures.BlackDummy->GetRenderTargetItem().ShaderResourceTexture;

				TRefCountPtr<IPooledRenderTarget> SSROutput;
				if( bSSR )
				{
					SCOPED_DRAW_EVENT(ScreenSpaceReflections, DEC_SCENE_ITEMS);

					ScreenSpaceReflections( View, SSROutput );
					SSRTexture = SSROutput->GetRenderTargetItem().ShaderResourceTexture;

					{
						// Apply reflections to screen
						SCOPED_DRAW_EVENT(ReflectionApply, DEC_SCENE_ITEMS);

						GSceneRenderTargets.BeginRenderingSceneColor();

						RHISetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);
						RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
						RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

						RHISetBlendState(TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One>::GetRHI());

						TShaderMapRef< FPostProcessVS >		VertexShader( GetGlobalShaderMap() );
						TShaderMapRef< FReflectionApplyPS >	PixelShader( GetGlobalShaderMap() );

						static FGlobalBoundShaderState BoundShaderState;
						SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

						PixelShader->SetParameters( View, GSystemTextures.BlackDummy->GetRenderTargetItem().ShaderResourceTexture, SSRTexture );

						DrawRectangle( 
							0, 0, 
							View.ViewRect.Width(), View.ViewRect.Height(),
							View.ViewRect.Min.X, View.ViewRect.Min.Y, 
							View.ViewRect.Width(), View.ViewRect.Height(),
							FIntPoint(View.ViewRect.Width(), View.ViewRect.Height()),
							GSceneRenderTargets.GetBufferSizeXY());
					}
				}
			}
		}
	}
}
