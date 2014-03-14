// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Functionality for capturing the scene into reflection capture cubemaps, and prefiltering
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessing.h"
#include "UniformBuffer.h"
#include "ShaderParameters.h"
#include "ScreenRendering.h"
#include "EngineDecalClasses.h"
#include "ReflectionEnvironment.h"
#include "ReflectionEnvironmentCapture.h"

/** Near plane to use when capturing the scene. */
float GReflectionCaptureNearPlane = 5;

int32 GSupersampleCaptureFactor = 1;

/** 
 * Mip map used by a Roughness of 0, counting down from the lowest resolution mip (MipCount - 1).  
 * This has been tweaked along with ReflectionCaptureRoughnessMipScale to make good use of the resolution in each mip, especially the highest resolution mips.
 * This value is duplicated in ReflectionEnvironmentShared.usf!
 */
float ReflectionCaptureRoughestMip = 1;

/** 
 * Scales the log2 of Roughness when computing which mip to use for a given roughness.
 * Larger values make the higher resolution mips sharper.
 * This has been tweaked along with ReflectionCaptureRoughnessMipScale to make good use of the resolution in each mip, especially the highest resolution mips.
 * This value is duplicated in ReflectionEnvironmentShared.usf!
 */
float ReflectionCaptureRoughnessMipScale = 1.2f;

int32 GDiffuseIrradianceCubemapSize = 32;

void OnUpdateReflectionCaptures( UWorld* InWorld )
{
	InWorld->UpdateAllReflectionCaptures();
}

FAutoConsoleCommandWithWorld CaptureConsoleCommand(
	TEXT("r.ReflectionCapture"),
	TEXT("Updates all reflection captures"),
	FConsoleCommandWithWorldDelegate::CreateStatic(OnUpdateReflectionCaptures)
	);


/** Encapsulates render target picking logic for cubemap mip generation. */
FSceneRenderTargetItem& GetEffectiveRenderTarget(bool bDownsamplePass, int32 TargetMipIndex)
{
	int32 ScratchTextureIndex = TargetMipIndex % 2;

	if (!bDownsamplePass)
	{
		ScratchTextureIndex = 1 - ScratchTextureIndex;
	}

	return GSceneRenderTargets.ReflectionColorScratchCubemap[ScratchTextureIndex]->GetRenderTargetItem();
}

/** Encapsulates source texture picking logic for cubemap mip generation. */
FSceneRenderTargetItem& GetEffectiveSourceTexture(bool bDownsamplePass, int32 TargetMipIndex)
{
	int32 ScratchTextureIndex = TargetMipIndex % 2;

	if (bDownsamplePass)
	{
		ScratchTextureIndex = 1 - ScratchTextureIndex;
	}

	return GSceneRenderTargets.ReflectionColorScratchCubemap[ScratchTextureIndex]->GetRenderTargetItem();
}

IMPLEMENT_SHADER_TYPE(,FDownsampleGS,TEXT("ReflectionEnvironmentShaders"),TEXT("DownsampleGS"),SF_Geometry);

const int32 NumFilterSamples = 1024;

/** Generates a sample on a cone with probability equal to the value of the GGX specular distribution. */
FVector GenerateImportanceSampleGGX(float Roughness, int32 Index, FVector2D* Samples)
{
	float m = Roughness * Roughness;
	float m2 = m * m;

	float E1 = Samples[Index].X;
	float E2 = Samples[Index].Y;

	// Only implemented for GGX
	float Phi = 2 * PI * E1;
	float CosPhi = FMath::Cos(Phi);
	float SinPhi = FMath::Sin(Phi);
	float CosTheta = FMath::Sqrt((1 - E2) / (1 + (m2 - 1) * E2));
	float SinTheta = FMath::Sqrt(1 - CosTheta * CosTheta);

	FVector HalfVector(SinTheta * FMath::Cos(Phi), SinTheta * FMath::Sin(Phi), CosTheta);
	FVector View(0, 0, 1);
	return 2 * (View | HalfVector) * HalfVector - View;
}

/** Uniform buffer for cubemap filtering data. */
BEGIN_UNIFORM_BUFFER_STRUCT(FCubeFilterData,)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY(FVector4,Samples,[NumFilterSamples])
END_UNIFORM_BUFFER_STRUCT(FCubeFilterData)

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FCubeFilterData,TEXT("FilterData"));

/** Pixel shader used for downsampling a mip or filtering a mip. */
class FDownsamplePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDownsamplePS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("NUM_FILTER_SAMPLES"), NumFilterSamples);
	}

	FDownsamplePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		CubeFace.Bind(Initializer.ParameterMap,TEXT("CubeFace"));
		SourceMipIndex.Bind(Initializer.ParameterMap,TEXT("SourceMipIndex"));
		SourceTexture.Bind(Initializer.ParameterMap,TEXT("SourceTexture"));
		SourceTextureSampler.Bind(Initializer.ParameterMap,TEXT("SourceTextureSampler"));
		AverageBrightnessTexture.Bind(Initializer.ParameterMap,TEXT("AverageBrightnessTexture"));
		AverageBrightnessSampler.Bind(Initializer.ParameterMap,TEXT("AverageBrightnessSampler"));
	}
	FDownsamplePS() {}

	void SetParameters(int32 CubeFaceValue, int32 SourceMipIndexValue, const FVector4* FilterSamples, FSceneRenderTargetItem& SourceTextureValue)
	{
		SetShaderValue(GetPixelShader(), CubeFace, CubeFaceValue);
		SetShaderValue(GetPixelShader(), SourceMipIndex, SourceMipIndexValue);

		FCubeFilterData FilterData;

		// Copy the samples
		// Can't use memcpy due to padding in the uniform buffer
		for (int32 i = 0; i < NumFilterSamples; i++)
		{
			FilterData.Samples[i] = FilterSamples[i];
		}

		SetUniformBufferParameterImmediate(GetPixelShader(), GetUniformBufferParameter<FCubeFilterData>(), FilterData);

		SetTextureParameter(
			GetPixelShader(), 
			SourceTexture, 
			SourceTextureSampler, 
			TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), 
			SourceTextureValue.ShaderResourceTexture);

		SetTextureParameter(
			GetPixelShader(), 
			AverageBrightnessTexture, 
			AverageBrightnessSampler, 
			TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), 
			GSceneRenderTargets.ReflectionBrightness->GetRenderTargetItem().ShaderResourceTexture);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << CubeFace;
		Ar << SourceMipIndex;
		Ar << SourceTexture;
		Ar << SourceTextureSampler;
		Ar << AverageBrightnessTexture;
		Ar << AverageBrightnessSampler;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter CubeFace;
	FShaderParameter SourceMipIndex;
	FShaderResourceParameter SourceTexture;
	FShaderResourceParameter SourceTextureSampler;
	FShaderResourceParameter AverageBrightnessTexture;
	FShaderResourceParameter AverageBrightnessSampler;
};

/** Used to generate permutations of FDownsamplePS with different defines. */
template<bool bDownsample>
class TDownsamplePS : public FDownsamplePS
{
	DECLARE_SHADER_TYPE(TDownsamplePS,Global);
public:

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FDownsamplePS::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("DOWNSAMPLE_PASS"), (uint32)(bDownsample ? 1 : 0));
	}

	TDownsamplePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FDownsamplePS(Initializer)
	{}

	TDownsamplePS()
	{}
};

#define IMPLEMENT_REFLECTION_FILTER_PIXELSHADER_TYPE(bDownsample, MainFunction) \
	typedef TDownsamplePS<bDownsample> TDownsamplePS##bDownsample; \
	IMPLEMENT_SHADER_TYPE(template<>,TDownsamplePS##bDownsample,TEXT("ReflectionEnvironmentShaders"),MainFunction,SF_Pixel);


IMPLEMENT_REFLECTION_FILTER_PIXELSHADER_TYPE(true, TEXT("DownsamplePS"));
IMPLEMENT_REFLECTION_FILTER_PIXELSHADER_TYPE(false, TEXT("BlurPS"));

FGlobalBoundShaderState DownsampleBoundShaderState;
FGlobalBoundShaderState BlurBoundShaderState;

/** Computes the average brightness of a 1x1 mip of a cubemap. */
class FComputeBrightnessPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FComputeBrightnessPS,Global)
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	FComputeBrightnessPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		ReflectionEnvironmentColorTexture.Bind(Initializer.ParameterMap,TEXT("ReflectionEnvironmentColorTexture"));
		ReflectionEnvironmentColorSampler.Bind(Initializer.ParameterMap,TEXT("ReflectionEnvironmentColorSampler"));
		NumCaptureArrayMips.Bind(Initializer.ParameterMap, TEXT("NumCaptureArrayMips"));
	}

	FComputeBrightnessPS()
	{
	}

	void SetParameters()
	{
		const int32 EffectiveTopMipSize = GReflectionCaptureSize;
		const int32 NumMips = FMath::CeilLogTwo(EffectiveTopMipSize) + 1;
		// Read from the smallest mip that was downsampled to
		FSceneRenderTargetItem& Cubemap = GetEffectiveRenderTarget(true, NumMips - 1);

		if (Cubemap.IsValid())
		{
			SetTextureParameter(
				GetPixelShader(), 
				ReflectionEnvironmentColorTexture, 
				ReflectionEnvironmentColorSampler, 
				TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), 
				Cubemap.ShaderResourceTexture);
		}

		SetShaderValue(GetPixelShader(), NumCaptureArrayMips, FMath::CeilLogTwo(GReflectionCaptureSize) + 1);
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << ReflectionEnvironmentColorTexture;
		Ar << ReflectionEnvironmentColorSampler;
		Ar << NumCaptureArrayMips;
		return bShaderHasOutdatedParameters;
	}

private:

	FShaderResourceParameter ReflectionEnvironmentColorTexture;
	FShaderResourceParameter ReflectionEnvironmentColorSampler;
	FShaderParameter NumCaptureArrayMips;
};

IMPLEMENT_SHADER_TYPE(,FComputeBrightnessPS,TEXT("ReflectionEnvironmentShaders"),TEXT("ComputeBrightnessMain"),SF_Pixel);

/** Computes the average brightness of the given reflection capture and stores it in the scene. */
void ComputeAverageBrightness()
{
	RHISetRenderTarget(GSceneRenderTargets.ReflectionBrightness->GetRenderTargetItem().TargetableTexture, NULL);
	RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
	RHISetBlendState(TStaticBlendState<>::GetRHI());

	TShaderMapRef<FPostProcessVS> VertexShader(GetGlobalShaderMap());
	TShaderMapRef<FComputeBrightnessPS> PixelShader(GetGlobalShaderMap());

	static FGlobalBoundShaderState BoundShaderState;
	SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	PixelShader->SetParameters();

	DrawRectangle( 
		0, 0, 
		1, 1,
		0, 0, 
		1, 1,
		FIntPoint(1, 1),
		FIntPoint(1, 1));
}

FVector4 FilterConeImportanceSamples[MAX_TEXTURE_MIP_COUNT][NumFilterSamples];

/** Generates mips for glossiness and filters the cubemap for a given reflection. */
void FilterReflectionEnvironment(FSHVectorRGB3* OutIrradianceEnvironmentMap)
{
	const int32 EffectiveTopMipSize = GReflectionCaptureSize;
	const int32 NumMips = FMath::CeilLogTwo(EffectiveTopMipSize) + 1;

	static bool bGeneratedSamples = false;

	if (!bGeneratedSamples)
	{
		bGeneratedSamples = true;
		FRandomStream RandomStream(12345);
		const int32 NumSamplesOneDimension = FMath::Sqrt(NumFilterSamples);
		FVector2D StratifiedUniformRandoms[NumFilterSamples];

		for (int32 i = 0; i < NumFilterSamples; i++)
		{
			StratifiedUniformRandoms[i] = FVector2D(
				(i % NumSamplesOneDimension + RandomStream.GetFraction()) / (float)NumSamplesOneDimension, 
				(i / NumSamplesOneDimension + RandomStream.GetFraction()) / (float)NumSamplesOneDimension);
		}

		for (int32 MipIndex = 0; MipIndex < NumMips; MipIndex++)
		{
			// Note: this must match the logic in ComputeReflectionCaptureMipFromRoughness!
			float Roughness = FMath::Pow(2, (MipIndex - NumMips + 1 + ReflectionCaptureRoughestMip) / ReflectionCaptureRoughnessMipScale);

			for (int32 SampleIndex = 0; SampleIndex < NumFilterSamples; SampleIndex++)
			{
				FilterConeImportanceSamples[MipIndex][SampleIndex] = FVector4(GenerateImportanceSampleGGX(Roughness, SampleIndex, StratifiedUniformRandoms), 0);		
			}
		}
	}

	int32 DiffuseConvolutionSourceMip = INDEX_NONE;
	FSceneRenderTargetItem* DiffuseConvolutionSource = NULL;

	{	
		SCOPED_DRAW_EVENT(DownsampleCubeMips, DEC_SCENE_ITEMS);
		// Downsample all the mips, each one reads from the mip above it
		for (int32 MipIndex = 1; MipIndex < NumMips; MipIndex++)
		{
			SCOPED_DRAW_EVENT(DownsampleCubeMip, DEC_SCENE_ITEMS);
			const int32 SourceMipIndex = FMath::Max(MipIndex - 1, 0);
			const int32 MipSize = 1 << (NumMips - MipIndex - 1);

			FSceneRenderTargetItem& EffectiveRT = GetEffectiveRenderTarget(true, MipIndex);
			FSceneRenderTargetItem& EffectiveSource = GetEffectiveSourceTexture(true, MipIndex);
			check(EffectiveRT.TargetableTexture != EffectiveSource.ShaderResourceTexture);

			if (GSupportsGSRenderTargetLayerSwitchingToMips)
			{
				RHISetRenderTarget(EffectiveRT.TargetableTexture, MipIndex, NULL);

				const FIntRect ViewRect(0, 0, MipSize, MipSize);
				RHISetViewport(0, 0, 0.0f, MipSize, MipSize, 1.0f);
				RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
				RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
				RHISetBlendState(TStaticBlendState<>::GetRHI());

				TShaderMapRef<FScreenVSForGS> VertexShader(GetGlobalShaderMap());
				TShaderMapRef<FDownsampleGS> GeometryShader(GetGlobalShaderMap());
				TShaderMapRef<TDownsamplePS<true> > PixelShader(GetGlobalShaderMap());

				SetGlobalBoundShaderState(DownsampleBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, *GeometryShader);

				// Draw each face with a geometry shader that routes to the correct render target slice
				for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
				{
					SCOPED_DRAW_EVENT(DownsampleCubeFace, DEC_SCENE_ITEMS);
					GeometryShader->SetParameters(CubeFace);

					PixelShader->SetParameters(CubeFace, SourceMipIndex, FilterConeImportanceSamples[SourceMipIndex], EffectiveSource);

					DrawRectangle( 
						ViewRect.Min.X, ViewRect.Min.Y, 
						ViewRect.Width(), ViewRect.Height(),
						ViewRect.Min.X, ViewRect.Min.Y, 
						ViewRect.Width(), ViewRect.Height(),
						FIntPoint(ViewRect.Width(), ViewRect.Height()),
						FIntPoint(MipSize, MipSize),
						EDRF_UseTriangleOptimization);

					RHICopyToResolveTarget(EffectiveRT.TargetableTexture, EffectiveRT.ShaderResourceTexture, true, FResolveParams(FResolveRect(), (ECubeFace)CubeFace, MipIndex));
				}
			}
			else
			{
				for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
				{
					RHISetRenderTarget(EffectiveRT.TargetableTexture, MipIndex, CubeFace, NULL);

					const FIntRect ViewRect(0, 0, MipSize, MipSize);
					RHISetViewport(0, 0, 0.0f, MipSize, MipSize, 1.0f);
					RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
					RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
					RHISetBlendState(TStaticBlendState<>::GetRHI());

					TShaderMapRef<FScreenVSForGS> VertexShader(GetGlobalShaderMap());
					TShaderMapRef<TDownsamplePS<true> > PixelShader(GetGlobalShaderMap());

					SetGlobalBoundShaderState(DownsampleBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

					PixelShader->SetParameters(CubeFace, SourceMipIndex, FilterConeImportanceSamples[SourceMipIndex], EffectiveSource);

					DrawRectangle( 
						ViewRect.Min.X, ViewRect.Min.Y, 
						ViewRect.Width(), ViewRect.Height(),
						ViewRect.Min.X, ViewRect.Min.Y, 
						ViewRect.Width(), ViewRect.Height(),
						FIntPoint(ViewRect.Width(), ViewRect.Height()),
						FIntPoint(MipSize, MipSize),
						EDRF_UseTriangleOptimization);

					RHICopyToResolveTarget(EffectiveRT.TargetableTexture, EffectiveRT.ShaderResourceTexture, true, FResolveParams(FResolveRect(), (ECubeFace)CubeFace, MipIndex));
				}
			}

			if (MipSize == GDiffuseIrradianceCubemapSize)
			{
				DiffuseConvolutionSourceMip = MipIndex;
				DiffuseConvolutionSource = &EffectiveRT;
			}
		}
	}

	if (OutIrradianceEnvironmentMap)
	{
		SCOPED_DRAW_EVENT(ComputeDiffuseIrradiance, DEC_SCENE_ITEMS);
		check(DiffuseConvolutionSource != NULL);
		ComputeDiffuseIrradiance(DiffuseConvolutionSource->ShaderResourceTexture, DiffuseConvolutionSourceMip, OutIrradianceEnvironmentMap);
	}
	
	ComputeAverageBrightness();

	{	
		SCOPED_DRAW_EVENT(FilterCubeMap, DEC_SCENE_ITEMS);
		// Filter all the mips, each one reads from whichever scratch render target holds the downsampled contents, and writes to the destination cubemap
		for (int32 MipIndex = 0; MipIndex < NumMips; MipIndex++)
		{
			SCOPED_DRAW_EVENT(FilterCubeMip, DEC_SCENE_ITEMS);
			FSceneRenderTargetItem& EffectiveRT = GetEffectiveRenderTarget(false, MipIndex);
			FSceneRenderTargetItem& EffectiveSource = GetEffectiveSourceTexture(false, MipIndex);
			check(EffectiveRT.TargetableTexture != EffectiveSource.ShaderResourceTexture);
			const int32 MipSize = 1 << (NumMips - MipIndex - 1);

			if (GSupportsGSRenderTargetLayerSwitchingToMips)
			{
				RHISetRenderTarget(EffectiveRT.TargetableTexture, MipIndex, NULL);

				const FIntRect ViewRect(0, 0, MipSize, MipSize);
				RHISetViewport(0, 0, 0.0f, MipSize, MipSize, 1.0f);
				RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
				RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
				RHISetBlendState(TStaticBlendState<>::GetRHI());

				TShaderMapRef<FScreenVSForGS> VertexShader(GetGlobalShaderMap());
				TShaderMapRef<FDownsampleGS> GeometryShader(GetGlobalShaderMap());
				TShaderMapRef<TDownsamplePS<false> > CaptureCubemapArrayPixelShader(GetGlobalShaderMap());
				FDownsamplePS* PixelShader = (FDownsamplePS*)*CaptureCubemapArrayPixelShader;

				SetGlobalBoundShaderState(BlurBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, PixelShader, *GeometryShader);

				for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
				{
					SCOPED_DRAW_EVENT(FilterCubeFace, DEC_SCENE_ITEMS);
					GeometryShader->SetParameters(CubeFace);

					PixelShader->SetParameters(CubeFace, MipIndex, FilterConeImportanceSamples[MipIndex], EffectiveSource);

					DrawRectangle( 
						ViewRect.Min.X, ViewRect.Min.Y, 
						ViewRect.Width(), ViewRect.Height(),
						ViewRect.Min.X, ViewRect.Min.Y, 
						ViewRect.Width(), ViewRect.Height(),
						FIntPoint(ViewRect.Width(), ViewRect.Height()),
						FIntPoint(MipSize, MipSize),
						EDRF_UseTriangleOptimization);

					RHICopyToResolveTarget(EffectiveRT.TargetableTexture, EffectiveRT.ShaderResourceTexture, true, FResolveParams(FResolveRect(), (ECubeFace)CubeFace, MipIndex));
				}
			}
			else
			{
				for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
				{
					RHISetRenderTarget(EffectiveRT.TargetableTexture, MipIndex, CubeFace, NULL);

					const FIntRect ViewRect(0, 0, MipSize, MipSize);
					RHISetViewport(0, 0, 0.0f, MipSize, MipSize, 1.0f);
					RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
					RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
					RHISetBlendState(TStaticBlendState<>::GetRHI());

					TShaderMapRef<FScreenVSForGS> VertexShader(GetGlobalShaderMap());
					TShaderMapRef<TDownsamplePS<false> > CaptureCubemapArrayPixelShader(GetGlobalShaderMap());
					FDownsamplePS* PixelShader = (FDownsamplePS*)*CaptureCubemapArrayPixelShader;

					SetGlobalBoundShaderState(BlurBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, PixelShader);

					PixelShader->SetParameters(CubeFace, MipIndex, FilterConeImportanceSamples[MipIndex], EffectiveSource);

					DrawRectangle( 
						ViewRect.Min.X, ViewRect.Min.Y, 
						ViewRect.Width(), ViewRect.Height(),
						ViewRect.Min.X, ViewRect.Min.Y, 
						ViewRect.Width(), ViewRect.Height(),
						FIntPoint(ViewRect.Width(), ViewRect.Height()),
						FIntPoint(MipSize, MipSize),
						EDRF_UseTriangleOptimization);

					RHICopyToResolveTarget(EffectiveRT.TargetableTexture, EffectiveRT.ShaderResourceTexture, true, FResolveParams(FResolveRect(), (ECubeFace)CubeFace, MipIndex));
				}
			}
		}
	}
}

/** Vertex shader used when writing to a cubemap. */
class FCopyToCubeFaceVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCopyToCubeFaceVS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	FCopyToCubeFaceVS()	{}
	FCopyToCubeFaceVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
	}

	void SetParameters(const FViewInfo& View)
	{
		FGlobalShader::SetParameters(GetVertexShader(),View);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(,FCopyToCubeFaceVS,TEXT("ReflectionEnvironmentShaders"),TEXT("CopyToCubeFaceVS"),SF_Vertex);

/** Geometry shader used when writing to a cubemap face.  Supports writing to a cubemap array as well. */
class FRouteToCubeFaceGS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FRouteToCubeFaceGS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	FRouteToCubeFaceGS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		CubeFace.Bind(Initializer.ParameterMap,TEXT("CubeFace"));
	}
	FRouteToCubeFaceGS() {}

	void SetParameters(uint32 CubeFaceValue)
	{
		SetShaderValue(GetGeometryShader(), CubeFace, CubeFaceValue);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << CubeFace;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter CubeFace;
};

IMPLEMENT_SHADER_TYPE(,FRouteToCubeFaceGS,TEXT("ReflectionEnvironmentShaders"),TEXT("RouteToCubeFaceGS"),SF_Geometry);

/** Pixel shader used when copying scene color from a scene render into a face of a reflection capture cubemap. */
class FCopySceneColorToCubeFacePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCopySceneColorToCubeFacePS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	FCopySceneColorToCubeFacePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		InTexture.Bind(Initializer.ParameterMap,TEXT("InTexture"));
		InTextureSampler.Bind(Initializer.ParameterMap,TEXT("InTextureSampler"));
		SkyLightCaptureParameters.Bind(Initializer.ParameterMap,TEXT("SkyLightCaptureParameters"));
	}
	FCopySceneColorToCubeFacePS() {}

	void SetParameters(const FViewInfo& View, bool bCapturingForSkyLight, bool bLowerHemisphereIsBlack)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(ShaderRHI, View);
		DeferredParameters.Set(ShaderRHI, View);

		SetTextureParameter(
			ShaderRHI, 
			InTexture, 
			InTextureSampler, 
			TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), 
			GSceneRenderTargets.SceneColor->GetRenderTargetItem().ShaderResourceTexture);		

		FVector SkyLightParametersValue(1, 0, bLowerHemisphereIsBlack ? 1.0f : 0.0f);
		FScene* Scene = (FScene*)View.Family->Scene;

		if (Scene->SkyLight && !bCapturingForSkyLight)
		{
			SkyLightParametersValue = FVector(0, Scene->SkyLight->SkyDistanceThreshold, 0);
		}

		SetShaderValue(ShaderRHI, SkyLightCaptureParameters, SkyLightParametersValue);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << InTexture;
		Ar << InTextureSampler;
		Ar << SkyLightCaptureParameters;
		return bShaderHasOutdatedParameters;
	}

private:
	FDeferredPixelShaderParameters DeferredParameters;	
	FShaderResourceParameter InTexture;
	FShaderResourceParameter InTextureSampler;
	FShaderParameter SkyLightCaptureParameters;
};

IMPLEMENT_SHADER_TYPE(,FCopySceneColorToCubeFacePS,TEXT("ReflectionEnvironmentShaders"),TEXT("CopySceneColorToCubeFaceColorPS"),SF_Pixel);

FGlobalBoundShaderState CopyColorCubemapBoundShaderState;

/** Pixel shader used when copying a cubemap into a face of a reflection capture cubemap. */
class FCopyCubemapToCubeFacePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCopyCubemapToCubeFacePS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	FCopyCubemapToCubeFacePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		CubeFace.Bind(Initializer.ParameterMap,TEXT("CubeFace"));
		SourceTexture.Bind(Initializer.ParameterMap,TEXT("SourceTexture"));
		SourceTextureSampler.Bind(Initializer.ParameterMap,TEXT("SourceTextureSampler"));
		SkyLightCaptureParameters.Bind(Initializer.ParameterMap,TEXT("SkyLightCaptureParameters"));
	}
	FCopyCubemapToCubeFacePS() {}

	void SetParameters(const FTexture* SourceCubemap, uint32 CubeFaceValue, bool bIsSkyLight, bool bLowerHemisphereIsBlack)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		SetShaderValue(ShaderRHI, CubeFace, CubeFaceValue);

		SetTextureParameter(
			ShaderRHI, 
			SourceTexture, 
			SourceTextureSampler, 
			SourceCubemap);

		SetShaderValue(ShaderRHI, SkyLightCaptureParameters, FVector(bIsSkyLight ? 1.0f : 0.0f, 0.0f, bLowerHemisphereIsBlack ? 1.0f : 0.0f));
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << CubeFace;
		Ar << SourceTexture;
		Ar << SourceTextureSampler;
		Ar << SkyLightCaptureParameters;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter CubeFace;
	FShaderResourceParameter SourceTexture;
	FShaderResourceParameter SourceTextureSampler;
	FShaderParameter SkyLightCaptureParameters;
};

IMPLEMENT_SHADER_TYPE(,FCopyCubemapToCubeFacePS,TEXT("ReflectionEnvironmentShaders"),TEXT("CopyCubemapToCubeFaceColorPS"),SF_Pixel);

FGlobalBoundShaderState CopyFromCubemapToCubemapBoundShaderState;

int32 FindOrAllocateCubemapIndex(FScene* Scene, const UReflectionCaptureComponent* Component)
{
	int32 CaptureIndex = -1;

	// Try to find an existing capture index for this component
	FCaptureComponentSceneState* CaptureSceneStatePtr = Scene->ReflectionSceneData.AllocatedReflectionCaptureState.Find(Component);

	if (CaptureSceneStatePtr)
	{
		CaptureIndex = CaptureSceneStatePtr->CaptureIndex;
	}
	else
	{
		// Reuse a freed index if possible
		for (int32 PotentialIndex = 0; PotentialIndex < Scene->ReflectionSceneData.AllocatedReflectionCaptureState.Num(); PotentialIndex++)
		{
			if (!Scene->ReflectionSceneData.AllocatedReflectionCaptureState.FindKey(FCaptureComponentSceneState(PotentialIndex)))
			{
				CaptureIndex = PotentialIndex;
				break;
			}
		}

		// Allocate a new index if needed
		if (CaptureIndex == -1)
		{
			CaptureIndex = Scene->ReflectionSceneData.AllocatedReflectionCaptureState.Num();
		}

		Scene->ReflectionSceneData.AllocatedReflectionCaptureState.Add(Component, FCaptureComponentSceneState(CaptureIndex));

		check(CaptureIndex < GMaxNumReflectionCaptures);
	}

	check(CaptureIndex >= 0);
	return CaptureIndex;
}

/** Captures the scene for a reflection capture by rendering the scene multiple times and copying into a cubemap texture. */
void CaptureSceneToScratchCubemap(FSceneRenderer* SceneRenderer, ECubeFace CubeFace, bool bCapturingForSkyLight, bool bLowerHemisphereIsBlack)
{
	FMemMark MemStackMark(FMemStack::Get());

	// update any resources that needed a deferred update
	FDeferredUpdateResource::UpdateResources();
	
	{
		SCOPED_DRAW_EVENT(CubeMapCapture, DEC_SCENE_ITEMS);

		// Render the scene normally for one face of the cubemap
		SceneRenderer->Render();

		const int32 EffectiveSize = GReflectionCaptureSize;
		FSceneRenderTargetItem& EffectiveColorRT =  GSceneRenderTargets.ReflectionColorScratchCubemap[0]->GetRenderTargetItem();

		{
			SCOPED_DRAW_EVENT(CubeMapCopyScene, DEC_SCENE_ITEMS);

			if (GSupportsGSRenderTargetLayerSwitchingToMips)
			{
				// Copy the captured scene into the cubemap face
				RHISetRenderTarget(EffectiveColorRT.TargetableTexture, NULL);

				const FIntRect ViewRect(0, 0, EffectiveSize, EffectiveSize);
				RHISetViewport(0, 0, 0.0f, EffectiveSize, EffectiveSize, 1.0f);
				RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
				RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
				RHISetBlendState(TStaticBlendState<>::GetRHI());

				TShaderMapRef<FCopyToCubeFaceVS> VertexShader(GetGlobalShaderMap());
				TShaderMapRef<FRouteToCubeFaceGS> GeometryShader(GetGlobalShaderMap());

				TShaderMapRef<FCopySceneColorToCubeFacePS> PixelShader(GetGlobalShaderMap());
				SetGlobalBoundShaderState(CopyColorCubemapBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, *GeometryShader);

				PixelShader->SetParameters(SceneRenderer->Views[0], bCapturingForSkyLight, bLowerHemisphereIsBlack);
				VertexShader->SetParameters(SceneRenderer->Views[0]);
				GeometryShader->SetParameters((uint32)CubeFace);

				DrawRectangle( 
					ViewRect.Min.X, ViewRect.Min.Y, 
					ViewRect.Width(), ViewRect.Height(),
					ViewRect.Min.X, ViewRect.Min.Y, 
					ViewRect.Width() * GSupersampleCaptureFactor, ViewRect.Height() * GSupersampleCaptureFactor,
					FIntPoint(ViewRect.Width(), ViewRect.Height()),
					GSceneRenderTargets.GetBufferSizeXY(),
					EDRF_UseTriangleOptimization);

				RHICopyToResolveTarget(EffectiveColorRT.TargetableTexture, EffectiveColorRT.ShaderResourceTexture, true, FResolveParams(FResolveRect()));
			}
			else
			{
				// Copy the captured scene into the cubemap face
				RHISetRenderTarget(EffectiveColorRT.TargetableTexture, 0, CubeFace, NULL);

				const FIntRect ViewRect(0, 0, EffectiveSize, EffectiveSize);
				RHISetViewport(0, 0, 0.0f, EffectiveSize, EffectiveSize, 1.0f);
				RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
				RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
				RHISetBlendState(TStaticBlendState<>::GetRHI());

				TShaderMapRef<FCopyToCubeFaceVS> VertexShader(GetGlobalShaderMap());
				TShaderMapRef<FCopySceneColorToCubeFacePS> PixelShader(GetGlobalShaderMap());
				SetGlobalBoundShaderState(CopyColorCubemapBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

				PixelShader->SetParameters(SceneRenderer->Views[0], bCapturingForSkyLight, bLowerHemisphereIsBlack);
				VertexShader->SetParameters(SceneRenderer->Views[0]);

				DrawRectangle( 
					ViewRect.Min.X, ViewRect.Min.Y, 
					ViewRect.Width(), ViewRect.Height(),
					ViewRect.Min.X, ViewRect.Min.Y, 
					ViewRect.Width() * GSupersampleCaptureFactor, ViewRect.Height() * GSupersampleCaptureFactor,
					FIntPoint(ViewRect.Width(), ViewRect.Height()),
					GSceneRenderTargets.GetBufferSizeXY(),
					EDRF_UseTriangleOptimization);

				RHICopyToResolveTarget(EffectiveColorRT.TargetableTexture, EffectiveColorRT.ShaderResourceTexture, true, FResolveParams(FResolveRect(), CubeFace));
			}
		}
	}

	delete SceneRenderer;
}

void CopyCubemapToScratchCubemap(UTextureCube* SourceCubemap, bool bIsSkyLight, bool bLowerHemisphereIsBlack)
{
	check(SourceCubemap);
	const int32 EffectiveSize = GReflectionCaptureSize;
	FSceneRenderTargetItem& EffectiveColorRT =  GSceneRenderTargets.ReflectionColorScratchCubemap[0]->GetRenderTargetItem();

	if (GSupportsGSRenderTargetLayerSwitchingToMips)
	{
		// Copy the captured scene into the cubemap face
		RHISetRenderTarget(EffectiveColorRT.TargetableTexture, NULL);

		const FTexture* SourceCubemapResource = SourceCubemap->Resource;
		const FIntPoint SourceDimensions(SourceCubemapResource->GetSizeX(), SourceCubemapResource->GetSizeY());
		const FIntRect ViewRect(0, 0, EffectiveSize, EffectiveSize);
		RHISetViewport(0, 0, 0.0f, EffectiveSize, EffectiveSize, 1.0f);
		RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
		RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
		RHISetBlendState(TStaticBlendState<>::GetRHI());

		TShaderMapRef<FScreenVSForGS> VertexShader(GetGlobalShaderMap());
		TShaderMapRef<FDownsampleGS> GeometryShader(GetGlobalShaderMap());
		TShaderMapRef<FCopyCubemapToCubeFacePS> PixelShader(GetGlobalShaderMap());

		for (uint32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
		{
			SetGlobalBoundShaderState(CopyFromCubemapToCubemapBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, *GeometryShader);
			GeometryShader->SetParameters(CubeFace);
			PixelShader->SetParameters(SourceCubemapResource, CubeFace, bIsSkyLight, bLowerHemisphereIsBlack);

			DrawRectangle( 
				ViewRect.Min.X, ViewRect.Min.Y, 
				ViewRect.Width(), ViewRect.Height(),
				0, 0, 
				SourceDimensions.X, SourceDimensions.Y,
				FIntPoint(ViewRect.Width(), ViewRect.Height()),
				SourceDimensions);
		}

		RHICopyToResolveTarget(EffectiveColorRT.TargetableTexture, EffectiveColorRT.ShaderResourceTexture, true, FResolveParams(FResolveRect()));
	}
	else
	{
		for (uint32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
		{
			// Copy the captured scene into the cubemap face
			RHISetRenderTarget(EffectiveColorRT.TargetableTexture, 0, CubeFace, NULL);

			const FTexture* SourceCubemapResource = SourceCubemap->Resource;
			const FIntPoint SourceDimensions(SourceCubemapResource->GetSizeX(), SourceCubemapResource->GetSizeY());
			const FIntRect ViewRect(0, 0, EffectiveSize, EffectiveSize);
			RHISetViewport(0, 0, 0.0f, EffectiveSize, EffectiveSize, 1.0f);
			RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
			RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
			RHISetBlendState(TStaticBlendState<>::GetRHI());

			TShaderMapRef<FScreenVSForGS> VertexShader(GetGlobalShaderMap());
			TShaderMapRef<FCopyCubemapToCubeFacePS> PixelShader(GetGlobalShaderMap());

			SetGlobalBoundShaderState(CopyFromCubemapToCubemapBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);
			PixelShader->SetParameters(SourceCubemapResource, CubeFace, bIsSkyLight, bLowerHemisphereIsBlack);

			DrawRectangle( 
				ViewRect.Min.X, ViewRect.Min.Y, 
				ViewRect.Width(), ViewRect.Height(),
				0, 0, 
				SourceDimensions.X, SourceDimensions.Y,
				FIntPoint(ViewRect.Width(), ViewRect.Height()),
				SourceDimensions);

			RHICopyToResolveTarget(EffectiveColorRT.TargetableTexture, EffectiveColorRT.ShaderResourceTexture, true, FResolveParams(FResolveRect(), (ECubeFace)CubeFace));
		}
	}
}

/** 
 * Allocates reflection captures in the scene's reflection cubemap array and updates them by recapturing the scene.
 * Existing captures will only be updated.  Must be called from the game thread.
 */
void FScene::AllocateReflectionCaptures(const TArray<UReflectionCaptureComponent*>& NewCaptures)
{
	if (NewCaptures.Num() > 0)
	{
		if (GRHIFeatureLevel == ERHIFeatureLevel::SM5)
		{
			for (int32 CaptureIndex = 0; CaptureIndex < NewCaptures.Num(); CaptureIndex++)
			{
				bool bAlreadyExists = false;

				// Try to find an existing allocation
				for (TSparseArray<UReflectionCaptureComponent*>::TIterator It(ReflectionSceneData.AllocatedReflectionCapturesGameThread); It; ++It)
				{
					UReflectionCaptureComponent* OtherComponent = *It;

					if (OtherComponent == NewCaptures[CaptureIndex])
					{
						bAlreadyExists = true;
					}
				}

				// Add the capture to the allocated list
				if (!bAlreadyExists && ReflectionSceneData.AllocatedReflectionCapturesGameThread.Num() < GMaxNumReflectionCaptures)
				{
					ReflectionSceneData.AllocatedReflectionCapturesGameThread.Add(NewCaptures[CaptureIndex]);
				}
			}

			// Request the exact amount needed by default
			int32 DesiredMaxCubemaps = ReflectionSceneData.AllocatedReflectionCapturesGameThread.Num();
			const float MaxCubemapsRoundUpBase = 1.5f;

			// If this is not the first time the scene has allocated the cubemap array, include slack to reduce reallocations
			if (ReflectionSceneData.MaxAllocatedReflectionCubemapsGameThread > 0)
			{
				float Exponent = FMath::LogX(MaxCubemapsRoundUpBase, ReflectionSceneData.AllocatedReflectionCapturesGameThread.Num());

				// Round up to the next integer exponent to provide stability and reduce reallocations
				DesiredMaxCubemaps = FMath::Pow(MaxCubemapsRoundUpBase, FMath::Trunc(Exponent) + 1);
			}

			DesiredMaxCubemaps = FMath::Min(DesiredMaxCubemaps, GMaxNumReflectionCaptures);

			if (DesiredMaxCubemaps != ReflectionSceneData.MaxAllocatedReflectionCubemapsGameThread)
			{
				ReflectionSceneData.MaxAllocatedReflectionCubemapsGameThread = DesiredMaxCubemaps;

				ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER( 
					ResizeArrayCommand,
					FScene*, Scene, this,
					uint32, MaxSize, ReflectionSceneData.MaxAllocatedReflectionCubemapsGameThread,
				{
					// Update the scene's cubemap array, which will reallocate it, so we no longer have the contents of existing entries
					Scene->ReflectionSceneData.CubemapArray.UpdateMaxCubemaps(MaxSize);
				});

				// Recapture all reflection captures now that we have reallocated the cubemap array
				UpdateAllReflectionCaptures();
			}
			else
			{
				// No reallocation of the cubemap array was needed, just update the captures that were requested

				for (TSparseArray<UReflectionCaptureComponent*>::TIterator It(ReflectionSceneData.AllocatedReflectionCapturesGameThread); It; ++It)
				{
					UReflectionCaptureComponent* CurrentComponent = *It;

					if (NewCaptures.Contains(CurrentComponent))
					{
						UpdateReflectionCaptureContents(CurrentComponent);
					}
				}
			}
		}
		else if (GRHIFeatureLevel == ERHIFeatureLevel::SM4)
		{
			for (int32 ComponentIndex = 0; ComponentIndex < NewCaptures.Num(); ComponentIndex++)
			{
				UReflectionCaptureComponent* CurrentComponent = NewCaptures[ComponentIndex];
				UpdateReflectionCaptureContents(CurrentComponent);
			}
		}

		for (int32 CaptureIndex = 0; CaptureIndex < NewCaptures.Num(); CaptureIndex++)
		{
			UReflectionCaptureComponent* Component = NewCaptures[CaptureIndex];

			Component->SetCaptureCompleted();

			if (Component->SceneProxy)
			{
				// Update the transform of the reflection capture
				// This is not done earlier by the reflection capture when it detects that it is dirty,
				// To ensure that the RT sees both the new transform and the new contents on the same frame.
				Component->SendRenderTransform_Concurrent();
			}
		}
	}
}

/** Updates the contents of all reflection captures in the scene.  Must be called from the game thread. */
void FScene::UpdateAllReflectionCaptures()
{
	if (IsReflectionEnvironmentAvailable())
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER( 
			CaptureCommand,
			FScene*, Scene, this,
		{
			Scene->ReflectionSceneData.AllocatedReflectionCaptureState.Empty();
		});

		const int32 UpdateDivisor = FMath::Max(ReflectionSceneData.AllocatedReflectionCapturesGameThread.Num() / 20, 1);
		const bool bDisplayStatus = ReflectionSceneData.AllocatedReflectionCapturesGameThread.Num() > 50;

		if (bDisplayStatus)
		{
			const FText Status = NSLOCTEXT("Engine", "BeginReflectionCapturesTask", "Updating Reflection Captures...");
			GWarn->BeginSlowTask( Status, true );
			GWarn->StatusUpdate(0, ReflectionSceneData.AllocatedReflectionCapturesGameThread.Num(), Status);
		}

		int32 CaptureIndex = 0;

		for (TSparseArray<UReflectionCaptureComponent*>::TIterator It(ReflectionSceneData.AllocatedReflectionCapturesGameThread); It; ++It)
		{
			// Update progress occasionally
			if (bDisplayStatus && CaptureIndex % UpdateDivisor == 0)
			{
				GWarn->UpdateProgress(CaptureIndex, ReflectionSceneData.AllocatedReflectionCapturesGameThread.Num());
			}

			CaptureIndex++;
			UReflectionCaptureComponent* CurrentComponent = *It;
			UpdateReflectionCaptureContents(CurrentComponent);
		}

		if (bDisplayStatus)
		{
			GWarn->EndSlowTask();
		}
	}
}

void GetReflectionCaptureData_RenderingThread(FScene* Scene, const UReflectionCaptureComponent* Component, FReflectionCaptureFullHDRDerivedData* OutDerivedData)
{
	const FCaptureComponentSceneState* ComponentStatePtr = Scene->ReflectionSceneData.AllocatedReflectionCaptureState.Find(Component);

	if (ComponentStatePtr)
	{
		const int32 CaptureIndex = ComponentStatePtr->CaptureIndex;
		const int32 EffectiveTopMipSize = GReflectionCaptureSize;
		const int32 NumMips = FMath::CeilLogTwo(EffectiveTopMipSize) + 1;

		TArray<uint8> CaptureData;
		int32 CaptureDataSize = 0;

		for (int32 MipIndex = 0; MipIndex < NumMips; MipIndex++)
		{
			const int32 MipSize = 1 << (NumMips - MipIndex - 1);

			for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
			{
				CaptureDataSize += MipSize * MipSize * sizeof(FFloat16Color);
			}
		}

		CaptureData.Empty(CaptureDataSize);
		CaptureData.AddZeroed(CaptureDataSize);
		int32 MipBaseIndex = 0;

		for (int32 MipIndex = 0; MipIndex < NumMips; MipIndex++)
		{
			FSceneRenderTargetItem& EffectiveDest = Scene->ReflectionSceneData.CubemapArray.GetRenderTarget();
			check(EffectiveDest.ShaderResourceTexture->GetFormat() == PF_FloatRGBA);
			const int32 MipSize = 1 << (NumMips - MipIndex - 1);
			const int32 CubeFaceBytes = MipSize * MipSize * sizeof(FFloat16Color);

			for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
			{
				TArray<FFloat16Color> SurfaceData;
				// Read each mip face
				//@todo - do this without blocking the GPU so many times
				//@todo - pool the temporary textures in RHIReadSurfaceFloatData instead of always creating new ones
				RHIReadSurfaceFloatData(EffectiveDest.ShaderResourceTexture, FIntRect(0, 0, MipSize, MipSize), SurfaceData, (ECubeFace)CubeFace, CaptureIndex, MipIndex);
				const int32 DestIndex = MipBaseIndex + CubeFace * CubeFaceBytes;
				uint8* FaceData = &CaptureData[DestIndex];
				check(SurfaceData.Num() * SurfaceData.GetTypeSize() == CubeFaceBytes);
				FMemory::Memcpy(FaceData, SurfaceData.GetData(), CubeFaceBytes);
			}

			MipBaseIndex += CubeFaceBytes * CubeFace_MAX;
		}

		OutDerivedData->InitializeFromUncompressedData(CaptureData);
	}
}

void FScene::GetReflectionCaptureData(UReflectionCaptureComponent* Component, FReflectionCaptureFullHDRDerivedData& OutDerivedData) 
{
	check(GRHIFeatureLevel == ERHIFeatureLevel::SM5);

	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		GetReflectionDataCommand,
		FScene*,Scene,this,
		const UReflectionCaptureComponent*,Component,Component,
		FReflectionCaptureFullHDRDerivedData*,OutDerivedData,&OutDerivedData,
	{
		GetReflectionCaptureData_RenderingThread(Scene, Component, OutDerivedData);
	});

	// Necessary since the RT is writing to OutDerivedData directly
	FlushRenderingCommands();
}

void UploadReflectionCapture_RenderingThread(FScene* Scene, const FReflectionCaptureFullHDRDerivedData* DerivedData, const UReflectionCaptureComponent* CaptureComponent)
{
	const int32 EffectiveTopMipSize = GReflectionCaptureSize;
	const int32 NumMips = FMath::CeilLogTwo(EffectiveTopMipSize) + 1;

	const int32 CaptureIndex = FindOrAllocateCubemapIndex(Scene, CaptureComponent);
	FTextureCubeRHIRef& CubeMapArray = (FTextureCubeRHIRef&)Scene->ReflectionSceneData.CubemapArray.GetRenderTarget().ShaderResourceTexture;
	check(CubeMapArray->GetFormat() == PF_FloatRGBA);

	TArray<uint8> CubemapData;
	DerivedData->GetUncompressedData(CubemapData);
	int32 MipBaseIndex = 0;

	for (int32 MipIndex = 0; MipIndex < NumMips; MipIndex++)
	{
		const int32 MipSize = 1 << (NumMips - MipIndex - 1);
		const int32 CubeFaceBytes = MipSize * MipSize * sizeof(FFloat16Color);

		for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
		{
			uint32 DestStride = 0;
			uint8* DestBuffer = (uint8*)RHILockTextureCubeFace(CubeMapArray, CubeFace, CaptureIndex, MipIndex, RLM_WriteOnly, DestStride, false);

			// Handle DestStride by copying each row
			for (int32 Y = 0; Y < MipSize; Y++)
			{
				FFloat16Color* DestPtr = (FFloat16Color*)((uint8*)DestBuffer + Y * DestStride);
				const int32 SourceIndex = MipBaseIndex + CubeFace * CubeFaceBytes + Y * MipSize * sizeof(FFloat16Color);
				const uint8* SourcePtr = &CubemapData[SourceIndex];
				FMemory::Memcpy(DestPtr, SourcePtr, MipSize * sizeof(FFloat16Color));
			}

			RHIUnlockTextureCubeFace(CubeMapArray, CubeFace, CaptureIndex, MipIndex, false);
		}

		MipBaseIndex += CubeFaceBytes * CubeFace_MAX;
	}
}

void ClearScratchCubemaps()
{
	// Clear scratch render targets to a consistent but noticeable value
	// This makes debugging capture issues much easier, otherwise the random contents from previous captures is shown

	const int32 NumMips = FMath::CeilLogTwo(GReflectionCaptureSize) + 1;
	FSceneRenderTargetItem& RT0 = GSceneRenderTargets.ReflectionColorScratchCubemap[0]->GetRenderTargetItem();

	for (int32 MipIndex = 0; MipIndex < NumMips; MipIndex++)
	{
		if (GSupportsGSRenderTargetLayerSwitchingToMips)
		{
			RHISetRenderTarget(RT0.TargetableTexture, MipIndex, NULL);
			RHIClear(true, FLinearColor(0, 10000, 0, 0), false, 0, false, 0, FIntRect());
		}
		else
		{
			for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
			{
				RHISetRenderTarget(RT0.TargetableTexture, MipIndex, CubeFace, NULL);
				RHIClear(true, FLinearColor(0, 10000, 0, 0), false, 0, false, 0, FIntRect());
			}
		}
	}

	FSceneRenderTargetItem& RT1 = GSceneRenderTargets.ReflectionColorScratchCubemap[1]->GetRenderTargetItem();

	for (int32 MipIndex = 0; MipIndex < NumMips; MipIndex++)
	{
		if (GSupportsGSRenderTargetLayerSwitchingToMips)
		{
			RHISetRenderTarget(RT1.TargetableTexture, MipIndex, NULL);
			RHIClear(true, FLinearColor(0, 10000, 0, 0), false, 0, false, 0, FIntRect());
		}
		else
		{
			for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
			{
				RHISetRenderTarget(RT1.TargetableTexture, MipIndex, CubeFace, NULL);
				RHIClear(true, FLinearColor(0, 10000, 0, 0), false, 0, false, 0, FIntRect());
			}
		}
	}
}

/** Creates a transformation for a cubemap face, following the D3D cubemap layout. */
FMatrix CalcCubeFaceViewMatrix(ECubeFace Face, FVector WorldLocation)
{
	FMatrix Result(FMatrix::Identity);

	static const FVector XAxis(1.f,0.f,0.f);
	static const FVector YAxis(0.f,1.f,0.f);
	static const FVector ZAxis(0.f,0.f,1.f);

	// vectors we will need for our basis
	FVector vUp(YAxis);
	FVector vDir;

	switch( Face )
	{
	case CubeFace_PosX:
		vDir = XAxis;
		break;
	case CubeFace_NegX:
		vDir = -XAxis;
		break;
	case CubeFace_PosY:
		vUp = -ZAxis;
		vDir = YAxis;
		break;
	case CubeFace_NegY:
		vUp = ZAxis;
		vDir = -YAxis;
		break;
	case CubeFace_PosZ:
		vDir = ZAxis;
		break;
	case CubeFace_NegZ:
		vDir = -ZAxis;
		break;
	}

	// derive right vector
	FVector vRight( vUp ^ vDir );
	// create matrix from the 3 axes
	Result = FBasisVectorMatrix( vRight, vUp, vDir, -WorldLocation );	

	return Result;
}

/** 
 * Render target class required for rendering the scene.
 * This doesn't actually allocate a render target as we read from scene color to get HDR results directly.
 */
class FCaptureRenderTarget : public FRenderResource, public FRenderTarget
{
public:

	FCaptureRenderTarget() :
		Size(GReflectionCaptureSize)
	{}

	virtual const FTexture2DRHIRef& GetRenderTargetTexture() const 
	{
		static FTexture2DRHIRef DummyTexture;
		return DummyTexture;
	}

	virtual FIntPoint GetSizeXY() const { return FIntPoint(Size, Size); }
	virtual float GetDisplayGamma() const { return 1.0f; }

private:

	int32 Size;
};

TGlobalResource<FCaptureRenderTarget> GReflectionCaptureRenderTarget;

void CaptureSceneIntoScratchCubemap(FScene* Scene, FVector CapturePosition, bool bCapturingForSkyLight, float SkyLightNearPlane, bool bLowerHemisphereIsBlack)
{
	for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
	{
		// Alert the RHI that we're rendering a new frame
		// Not really a new frame, but it will allow pooling mechanisms to update, like the uniform buffer pool
		ENQUEUE_UNIQUE_RENDER_COMMAND(
			BeginFrame,
		{
			GFrameNumberRenderThread++;
			RHIBeginFrame();
		})

		FSceneViewFamilyContext ViewFamily( FSceneViewFamily::ConstructionValues(
			&GReflectionCaptureRenderTarget,
			Scene,
			FEngineShowFlags(ESFIM_Game))
			.SetWorldTimes( 0.0f, 0.0f, 0.0f )				
			.SetResolveScene(false) );

		// Disable features that are not desired when capturing the scene
		ViewFamily.EngineShowFlags.PostProcessing = 0;
		ViewFamily.EngineShowFlags.MotionBlur = 0;
		ViewFamily.EngineShowFlags.SetOnScreenDebug(false);
		ViewFamily.EngineShowFlags.HMDDistortion = 0;
		// Exclude particles and light functions as they are usually dynamic, and can't be captured well
		ViewFamily.EngineShowFlags.Particles = 0;
		ViewFamily.EngineShowFlags.LightFunctions = 0;
		ViewFamily.EngineShowFlags.CompositeEditorPrimitives = 0;
		// These are highly dynamic and can't be captured effectively
		ViewFamily.EngineShowFlags.LightShafts = 0;

		// Don't apply sky lighting diffuse when capturing the sky light source, or we would have feedback
		ViewFamily.EngineShowFlags.SkyLighting = !bCapturingForSkyLight;

		FSceneViewInitOptions ViewInitOptions;
		ViewInitOptions.ViewFamily = &ViewFamily;
		ViewInitOptions.BackgroundColor = FLinearColor::Black;
		ViewInitOptions.OverlayColor = FLinearColor::Black;
		ViewInitOptions.SetViewRectangle(FIntRect(0, 0, GReflectionCaptureSize * GSupersampleCaptureFactor, GReflectionCaptureSize * GSupersampleCaptureFactor));

		const float NearPlane = bCapturingForSkyLight ? SkyLightNearPlane : GReflectionCaptureNearPlane;

		// Projection matrix based on the fov, near / far clip settings
		// Each face always uses a 90 degree field of view
		ViewInitOptions.ProjectionMatrix = FReversedZPerspectiveMatrix(
			90.0f * (float)PI / 360.0f,
			(float)GReflectionCaptureSize * GSupersampleCaptureFactor,
			(float)GReflectionCaptureSize * GSupersampleCaptureFactor,
			NearPlane
			);

		ViewInitOptions.ViewMatrix = CalcCubeFaceViewMatrix((ECubeFace)CubeFace, CapturePosition);

		FSceneView* View = new FSceneView(ViewInitOptions);

		// Force all surfaces diffuse
		View->RoughnessOverrideParameter = FVector2D( 1.0f, 0.0f );

		View->bIsReflectionCapture = true;
		View->StartFinalPostprocessSettings(CapturePosition);
		View->EndFinalPostprocessSettings();

		ViewFamily.Views.Add(View);

		FSceneRenderer* SceneRenderer = FSceneRenderer::CreateSceneRenderer(&ViewFamily, NULL);

		ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER( 
			CaptureCommand,
			FSceneRenderer*, SceneRenderer, SceneRenderer,
			ECubeFace, CubeFace, (ECubeFace)CubeFace,
			bool, bCapturingForSkyLight, bCapturingForSkyLight,
			bool, bLowerHemisphereIsBlack, bLowerHemisphereIsBlack,
		{
			CaptureSceneToScratchCubemap(SceneRenderer, CubeFace, bCapturingForSkyLight, bLowerHemisphereIsBlack);

			RHIEndFrame();
		});
	}
}

void CopyToSceneArray(FScene* Scene, FReflectionCaptureProxy* ReflectionProxy)
{
	const int32 EffectiveTopMipSize = GReflectionCaptureSize;
	const int32 NumMips = FMath::CeilLogTwo(EffectiveTopMipSize) + 1;

	const int32 CaptureIndex = FindOrAllocateCubemapIndex(Scene, ReflectionProxy->Component);

	// GPU copy back to the scene's texture array, which is not a render target
	for (int32 MipIndex = 0; MipIndex < NumMips; MipIndex++)
	{
		// The source for this copy is the dest from the filtering pass
		FSceneRenderTargetItem& EffectiveSource = GetEffectiveRenderTarget(false, MipIndex);
		FSceneRenderTargetItem& EffectiveDest = Scene->ReflectionSceneData.CubemapArray.GetRenderTarget();

		for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
		{
			RHICopyToResolveTarget(EffectiveSource.ShaderResourceTexture, EffectiveDest.ShaderResourceTexture, true, FResolveParams(FResolveRect(), (ECubeFace)CubeFace, MipIndex, 0, CaptureIndex));
		}
	}
}

void CopyToComponentTexture(FScene* Scene, FReflectionCaptureProxy* ReflectionProxy)
{
	check(ReflectionProxy->SM4FullHDRCubemap);
	const int32 EffectiveTopMipSize = GReflectionCaptureSize;
	const int32 NumMips = FMath::CeilLogTwo(EffectiveTopMipSize) + 1;

	// GPU copy back to the component's cubemap texture, which is not a render target
	for (int32 MipIndex = 0; MipIndex < NumMips; MipIndex++)
	{
		// The source for this copy is the dest from the filtering pass
		FSceneRenderTargetItem& EffectiveSource = GetEffectiveRenderTarget(false, MipIndex);

		for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
		{
			RHICopyToResolveTarget(EffectiveSource.ShaderResourceTexture, ReflectionProxy->SM4FullHDRCubemap->TextureRHI, true, FResolveParams(FResolveRect(), (ECubeFace)CubeFace, MipIndex, 0, 0));
		}
	}
}

/** 
 * Updates the contents of the given reflection capture by rendering the scene. 
 * This must be called on the game thread.
 */
void FScene::UpdateReflectionCaptureContents(UReflectionCaptureComponent* CaptureComponent)
{
	if (IsReflectionEnvironmentAvailable())
	{
		const FReflectionCaptureFullHDRDerivedData* DerivedData = CaptureComponent->GetCachedFullHDRDerivedData();

		// Upload existing derived data if it exists, instead of capturing
		if (DerivedData && DerivedData->CompressedCapturedData.Num() > 0)
		{
			// For other feature levels the reflection textures are stored on the component instead of in a scene-wide texture array
			if (GRHIFeatureLevel == ERHIFeatureLevel::SM5)
			{
				ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER( 
					UploadCaptureCommand,
					FScene*, Scene, this,
					const FReflectionCaptureFullHDRDerivedData*, DerivedData, DerivedData,
					const UReflectionCaptureComponent*, CaptureComponent, CaptureComponent,
				{
					UploadReflectionCapture_RenderingThread(Scene, DerivedData, CaptureComponent);
				});
			}
		}
		else
		{
			ENQUEUE_UNIQUE_RENDER_COMMAND( 
				ClearCommand,
			{
				ClearScratchCubemaps();
			});

			CaptureSceneIntoScratchCubemap(this, CaptureComponent->GetComponentLocation(), false, 0, false);

			ENQUEUE_UNIQUE_RENDER_COMMAND( 
				FilterCommand,
			{
				FilterReflectionEnvironment(NULL);
			});

			// Create a proxy to represent the reflection capture to the rendering thread
			// The rendering thread will be responsible for deleting this when done with the filtering operation
			// We can't use the component's SceneProxy here because the component may not be registered with the scene
			FReflectionCaptureProxy* ReflectionProxy = new FReflectionCaptureProxy(CaptureComponent);

			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER( 
				CopyCommand,
				FScene*, Scene, this,
				FReflectionCaptureProxy*, ReflectionProxy, ReflectionProxy,
			{
				if (GRHIFeatureLevel == ERHIFeatureLevel::SM5)
				{
					CopyToSceneArray(Scene, ReflectionProxy);
				}
				else if (GRHIFeatureLevel == ERHIFeatureLevel::SM4)
				{
					CopyToComponentTexture(Scene, ReflectionProxy);
				}

				// Clean up the proxy now that the rendering thread is done with it
				delete ReflectionProxy;
			});
		}
	}
}

void CopyToSkyTexture(FScene* Scene, FSkyLightSceneProxy* SkyProxy)
{
	const int32 EffectiveTopMipSize = SkyProxy->ProcessedTexture->GetSizeX();
	const int32 NumMips = FMath::CeilLogTwo(EffectiveTopMipSize) + 1;

	// GPU copy back to the skylight's texture, which is not a render target
	for (int32 MipIndex = 0; MipIndex < NumMips; MipIndex++)
	{
		// The source for this copy is the dest from the filtering pass
		FSceneRenderTargetItem& EffectiveSource = GetEffectiveRenderTarget(false, MipIndex);

		for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
		{
			RHICopyToResolveTarget(EffectiveSource.ShaderResourceTexture, SkyProxy->ProcessedTexture->TextureRHI, true, FResolveParams(FResolveRect(), (ECubeFace)CubeFace, MipIndex, 0, 0));
		}
	}
}

void FScene::UpdateSkyCaptureContents(USkyLightComponent* CaptureComponent)
{
	if (IsReflectionEnvironmentAvailable())
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND( 
			ClearCommand,
		{
			//@todo realtime skylight updates - skip this
			ClearScratchCubemaps();
		});

		if (CaptureComponent->SourceType == SLS_CapturedScene)
		{
			CaptureSceneIntoScratchCubemap(this, CaptureComponent->GetComponentLocation(), true, CaptureComponent->SkyDistanceThreshold, CaptureComponent->bLowerHemisphereIsBlack);
		}
		else if (CaptureComponent->SourceType == SLS_SpecifiedCubemap)
		{
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER( 
				CopyCubemapCommand,
				UTextureCube*, SourceTexture, CaptureComponent->Cubemap,
				bool, bLowerHemisphereIsBlack, CaptureComponent->bLowerHemisphereIsBlack,
			{
				CopyCubemapToScratchCubemap(SourceTexture, true, bLowerHemisphereIsBlack);
			});
		}
		else
		{
			check(0);
		}

		FSHVectorRGB3 IrradianceEnvironmentMap;

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER( 
			FilterCommand,
			FSHVectorRGB3*, IrradianceEnvironmentMap, &IrradianceEnvironmentMap,
		{
			FilterReflectionEnvironment(IrradianceEnvironmentMap);
		});

		// Wait until the SH coefficients have been written out by the RT
		//@todo - remove the need for this
		FlushRenderingCommands();

		CaptureComponent->SetIrradianceEnvironmentMap(IrradianceEnvironmentMap);

		FSkyLightSceneProxy* SkyProxy = new FSkyLightSceneProxy(CaptureComponent);

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER( 
			CopyCommand,
			FScene*, Scene, this,
			FSkyLightSceneProxy*, SkyProxy, SkyProxy,
		{
			CopyToSkyTexture(Scene, SkyProxy);

			// Clean up the proxy now that the rendering thread is done with it
			delete SkyProxy;
		});

		CaptureComponent->MarkRenderStateDirty();
	}
}
