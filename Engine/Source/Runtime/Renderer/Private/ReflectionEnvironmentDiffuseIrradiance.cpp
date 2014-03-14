// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Functionality for computing SH diffuse irradiance from a cubemap
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

extern int32 GDiffuseIrradianceCubemapSize;

FSceneRenderTargetItem& GetEffectiveDiffuseIrradianceRenderTarget(int32 TargetMipIndex)
{
	const int32 ScratchTextureIndex = TargetMipIndex % 2;

	return GSceneRenderTargets.DiffuseIrradianceScratchCubemap[ScratchTextureIndex]->GetRenderTargetItem();
}

FSceneRenderTargetItem& GetEffectiveDiffuseIrradianceSourceTexture(int32 TargetMipIndex)
{
	const int32 ScratchTextureIndex = 1 - TargetMipIndex % 2;

	return GSceneRenderTargets.DiffuseIrradianceScratchCubemap[ScratchTextureIndex]->GetRenderTargetItem();
}

/** Pixel shader used for copying to diffuse irradiance texture. */
class FCopyDiffuseIrradiancePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCopyDiffuseIrradiancePS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	FCopyDiffuseIrradiancePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		CubeFace.Bind(Initializer.ParameterMap,TEXT("CubeFace"));
		SourceMipIndex.Bind(Initializer.ParameterMap,TEXT("SourceMipIndex"));
		SourceTexture.Bind(Initializer.ParameterMap,TEXT("SourceTexture"));
		SourceTextureSampler.Bind(Initializer.ParameterMap,TEXT("SourceTextureSampler"));
		CoefficientMask0.Bind(Initializer.ParameterMap,TEXT("CoefficientMask0"));
		CoefficientMask1.Bind(Initializer.ParameterMap,TEXT("CoefficientMask1"));
		CoefficientMask2.Bind(Initializer.ParameterMap,TEXT("CoefficientMask2"));
		NumSamples.Bind(Initializer.ParameterMap,TEXT("NumSamples"));
	}
	FCopyDiffuseIrradiancePS() {}

	void SetParameters(int32 CubeFaceValue, int32 SourceMipIndexValue, int32 CoefficientIndex, int32 FaceResolution, FTextureRHIRef& SourceTextureValue)
	{
		SetShaderValue(GetPixelShader(), CubeFace, CubeFaceValue);
		SetShaderValue(GetPixelShader(), SourceMipIndex, SourceMipIndexValue);

		SetTextureParameter(
			GetPixelShader(), 
			SourceTexture, 
			SourceTextureSampler, 
			TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), 
			SourceTextureValue);

		const FVector4 Mask0(CoefficientIndex == 0, CoefficientIndex == 1, CoefficientIndex == 2, CoefficientIndex == 3);
		const FVector4 Mask1(CoefficientIndex == 4, CoefficientIndex == 5, CoefficientIndex == 6, CoefficientIndex == 7);
		const float Mask2 = CoefficientIndex == 8;
		SetShaderValue(GetPixelShader(), CoefficientMask0, Mask0);
		SetShaderValue(GetPixelShader(), CoefficientMask1, Mask1);
		SetShaderValue(GetPixelShader(), CoefficientMask2, Mask2);

		SetShaderValue(GetPixelShader(), NumSamples, FaceResolution * FaceResolution * 6);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << CubeFace;
		Ar << SourceMipIndex;
		Ar << SourceTexture;
		Ar << SourceTextureSampler;
		Ar << CoefficientMask0;
		Ar << CoefficientMask1;
		Ar << CoefficientMask2;
		Ar << NumSamples;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter CubeFace;
	FShaderParameter SourceMipIndex;
	FShaderResourceParameter SourceTexture;
	FShaderResourceParameter SourceTextureSampler;
	FShaderParameter CoefficientMask0;
	FShaderParameter CoefficientMask1;
	FShaderParameter CoefficientMask2;
	FShaderParameter NumSamples;
};

IMPLEMENT_SHADER_TYPE(,FCopyDiffuseIrradiancePS,TEXT("ReflectionEnvironmentShaders"),TEXT("DiffuseIrradianceCopyPS"),SF_Pixel)

FGlobalBoundShaderState CopyDiffuseIrradianceShaderState;


/**  */
class FAccumulateDiffuseIrradiancePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FAccumulateDiffuseIrradiancePS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	FAccumulateDiffuseIrradiancePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		CubeFace.Bind(Initializer.ParameterMap,TEXT("CubeFace"));
		SourceMipIndex.Bind(Initializer.ParameterMap,TEXT("SourceMipIndex"));
		SourceTexture.Bind(Initializer.ParameterMap,TEXT("SourceTexture"));
		SourceTextureSampler.Bind(Initializer.ParameterMap,TEXT("SourceTextureSampler"));
		Sample01.Bind(Initializer.ParameterMap,TEXT("Sample01"));
		Sample23.Bind(Initializer.ParameterMap,TEXT("Sample23"));
	}
	FAccumulateDiffuseIrradiancePS() {}

	void SetParameters(int32 CubeFaceValue, int32 NumMips, int32 SourceMipIndexValue, int32 CoefficientIndex, FTextureRHIRef& SourceTextureValue)
	{
		SetShaderValue(GetPixelShader(), CubeFace, CubeFaceValue);
		SetShaderValue(GetPixelShader(), SourceMipIndex, SourceMipIndexValue);

		SetTextureParameter(
			GetPixelShader(), 
			SourceTexture, 
			SourceTextureSampler, 
			TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), 
			SourceTextureValue);

		const int32 MipSize = 1 << (NumMips - SourceMipIndexValue - 1);
		const float HalfSourceTexelSize = .5f / MipSize;
		const FVector4 Sample01Value(-HalfSourceTexelSize, -HalfSourceTexelSize, HalfSourceTexelSize, -HalfSourceTexelSize);
		const FVector4 Sample23Value(-HalfSourceTexelSize, HalfSourceTexelSize, HalfSourceTexelSize, HalfSourceTexelSize);
		SetShaderValue(GetPixelShader(), Sample01, Sample01Value);
		SetShaderValue(GetPixelShader(), Sample23, Sample23Value);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << CubeFace;
		Ar << SourceMipIndex;
		Ar << SourceTexture;
		Ar << SourceTextureSampler;
		Ar << Sample01;
		Ar << Sample23;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter CubeFace;
	FShaderParameter SourceMipIndex;
	FShaderResourceParameter SourceTexture;
	FShaderResourceParameter SourceTextureSampler;
	FShaderParameter Sample01;
	FShaderParameter Sample23;
};

IMPLEMENT_SHADER_TYPE(,FAccumulateDiffuseIrradiancePS,TEXT("ReflectionEnvironmentShaders"),TEXT("DiffuseIrradianceAccumulatePS"),SF_Pixel)

FGlobalBoundShaderState DiffuseIrradianceAccumulateShaderState;


/**  */
class FAccumulateCubeFacesPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FAccumulateCubeFacesPS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	FAccumulateCubeFacesPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		SourceMipIndex.Bind(Initializer.ParameterMap,TEXT("SourceMipIndex"));
		SourceTexture.Bind(Initializer.ParameterMap,TEXT("SourceTexture"));
		SourceTextureSampler.Bind(Initializer.ParameterMap,TEXT("SourceTextureSampler"));
	}
	FAccumulateCubeFacesPS() {}

	void SetParameters(int32 SourceMipIndexValue, FTextureRHIRef& SourceTextureValue)
	{
		SetShaderValue(GetPixelShader(), SourceMipIndex, SourceMipIndexValue);

		SetTextureParameter(
			GetPixelShader(), 
			SourceTexture, 
			SourceTextureSampler, 
			TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), 
			SourceTextureValue);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << SourceMipIndex;
		Ar << SourceTexture;
		Ar << SourceTextureSampler;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter SourceMipIndex;
	FShaderResourceParameter SourceTexture;
	FShaderResourceParameter SourceTextureSampler;
};

IMPLEMENT_SHADER_TYPE(,FAccumulateCubeFacesPS,TEXT("ReflectionEnvironmentShaders"),TEXT("AccumulateCubeFacesPS"),SF_Pixel)

FGlobalBoundShaderState AccumulateCubeFacesBoundShaderState;

void ComputeDiffuseIrradiance(FTextureRHIRef LightingSource, int32 LightingSourceMipIndex, FSHVectorRGB3* OutIrradianceEnvironmentMap)
{
	for (int32 CoefficientIndex = 0; CoefficientIndex < FSHVector3::MaxSHBasis; CoefficientIndex++)
	{
		// Copy the starting mip from the lighting texture, apply texel area weighting and appropriate SH coefficient
		{
			const int32 MipIndex = 0;
			const int32 MipSize = GDiffuseIrradianceCubemapSize;
			FSceneRenderTargetItem& EffectiveRT = GetEffectiveDiffuseIrradianceRenderTarget(MipIndex);
			RHISetRenderTarget(EffectiveRT.TargetableTexture, 0, NULL);

			const FIntRect ViewRect(0, 0, MipSize, MipSize);
			RHISetViewport(0, 0, 0.0f, MipSize, MipSize, 1.0f);
			RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
			RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
			RHISetBlendState(TStaticBlendState<>::GetRHI());

			TShaderMapRef<FScreenVSForGS> VertexShader(GetGlobalShaderMap());
			TShaderMapRef<FDownsampleGS> GeometryShader(GetGlobalShaderMap());
			TShaderMapRef<FCopyDiffuseIrradiancePS> PixelShader(GetGlobalShaderMap());

			SetGlobalBoundShaderState(CopyDiffuseIrradianceShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, *GeometryShader);

			// Draw each face with a geometry shader that routes to the correct render target slice
			for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
			{
				GeometryShader->SetParameters(CubeFace);

				PixelShader->SetParameters(CubeFace, LightingSourceMipIndex, CoefficientIndex, MipSize, LightingSource);

				DrawRectangle( 
					ViewRect.Min.X, ViewRect.Min.Y, 
					ViewRect.Width(), ViewRect.Height(),
					ViewRect.Min.X, ViewRect.Min.Y, 
					ViewRect.Width(), ViewRect.Height(),
					FIntPoint(ViewRect.Width(), ViewRect.Height()),
					FIntPoint(MipSize, MipSize));

				RHICopyToResolveTarget(EffectiveRT.TargetableTexture, EffectiveRT.ShaderResourceTexture, true, FResolveParams(FResolveRect(), (ECubeFace)CubeFace, MipIndex));
			}
		}

		const int32 NumMips = FMath::CeilLogTwo(GDiffuseIrradianceCubemapSize) + 1;

		{
			// Accumulate all the texel values through downsampling to 1x1 mip
			for (int32 MipIndex = 1; MipIndex < NumMips; MipIndex++)
			{
				const int32 SourceMipIndex = FMath::Max(MipIndex - 1, 0);
				const int32 MipSize = 1 << (NumMips - MipIndex - 1);

				FSceneRenderTargetItem& EffectiveRT = GetEffectiveDiffuseIrradianceRenderTarget(MipIndex);
				FSceneRenderTargetItem& EffectiveSource = GetEffectiveDiffuseIrradianceSourceTexture(MipIndex);
				check(EffectiveRT.TargetableTexture != EffectiveSource.ShaderResourceTexture);

				RHISetRenderTarget(EffectiveRT.TargetableTexture, MipIndex, NULL);

				const FIntRect ViewRect(0, 0, MipSize, MipSize);
				RHISetViewport(0, 0, 0.0f, MipSize, MipSize, 1.0f);
				RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
				RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
				RHISetBlendState(TStaticBlendState<>::GetRHI());

				TShaderMapRef<FScreenVSForGS> VertexShader(GetGlobalShaderMap());
				TShaderMapRef<FDownsampleGS> GeometryShader(GetGlobalShaderMap());
				TShaderMapRef<FAccumulateDiffuseIrradiancePS> PixelShader(GetGlobalShaderMap());

				SetGlobalBoundShaderState(DiffuseIrradianceAccumulateShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, *GeometryShader);

				// Draw each face with a geometry shader that routes to the correct render target slice
				for (int32 CubeFace = 0; CubeFace < CubeFace_MAX; CubeFace++)
				{
					GeometryShader->SetParameters(CubeFace);

					PixelShader->SetParameters(CubeFace, NumMips, SourceMipIndex, CoefficientIndex, EffectiveSource.ShaderResourceTexture);

					DrawRectangle( 
						ViewRect.Min.X, ViewRect.Min.Y, 
						ViewRect.Width(), ViewRect.Height(),
						ViewRect.Min.X, ViewRect.Min.Y, 
						ViewRect.Width(), ViewRect.Height(),
						FIntPoint(ViewRect.Width(), ViewRect.Height()),
						FIntPoint(MipSize, MipSize));

					RHICopyToResolveTarget(EffectiveRT.TargetableTexture, EffectiveRT.ShaderResourceTexture, true, FResolveParams(FResolveRect(), (ECubeFace)CubeFace, MipIndex));
				}
			}
		}

		{
			// Gather the cubemap face results and normalize, copy this coefficient to GSceneRenderTargets.SkySHIrradianceMap
			FSceneRenderTargetItem& EffectiveRT = GSceneRenderTargets.SkySHIrradianceMap->GetRenderTargetItem();
			RHISetRenderTarget(EffectiveRT.TargetableTexture, 0, NULL);

			const FIntRect ViewRect(CoefficientIndex, 0, CoefficientIndex + 1, 1);
			RHISetViewport(0, 0, 0.0f, FSHVector3::MaxSHBasis, 1, 1.0f);
			RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
			RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
			RHISetBlendState(TStaticBlendState<>::GetRHI());

			TShaderMapRef<FScreenVS> VertexShader(GetGlobalShaderMap());
			TShaderMapRef<FAccumulateCubeFacesPS> PixelShader(GetGlobalShaderMap());
			SetGlobalBoundShaderState(AccumulateCubeFacesBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

			const int32 SourceMipIndex = NumMips - 1;
			const int32 MipSize = 1;
			FSceneRenderTargetItem& EffectiveSource = GetEffectiveDiffuseIrradianceRenderTarget(SourceMipIndex);
			PixelShader->SetParameters(SourceMipIndex, EffectiveSource.ShaderResourceTexture);

			DrawRectangle( 
				ViewRect.Min.X, ViewRect.Min.Y, 
				ViewRect.Width(), ViewRect.Height(),
				0, 0, 
				MipSize, MipSize,
				FIntPoint(FSHVector3::MaxSHBasis, 1),
				FIntPoint(MipSize, MipSize));

			RHICopyToResolveTarget(EffectiveRT.TargetableTexture, EffectiveRT.ShaderResourceTexture, true, FResolveParams());
		}
	}

	{
		// Read back the completed SH environment map
		FSceneRenderTargetItem& EffectiveRT = GSceneRenderTargets.SkySHIrradianceMap->GetRenderTargetItem();
		check(EffectiveRT.ShaderResourceTexture->GetFormat() == PF_FloatRGBA);

		TArray<FFloat16Color> SurfaceData;
		RHIReadSurfaceFloatData(EffectiveRT.ShaderResourceTexture, FIntRect(0, 0, FSHVector3::MaxSHBasis, 1), SurfaceData, CubeFace_PosX, 0, 0);
		check(SurfaceData.Num() == FSHVector3::MaxSHBasis);

		for (int32 CoefficientIndex = 0; CoefficientIndex < FSHVector3::MaxSHBasis; CoefficientIndex++)
		{
			const FLinearColor CoefficientValue(SurfaceData[CoefficientIndex]);
			OutIrradianceEnvironmentMap->R.V[CoefficientIndex] = CoefficientValue.R;
			OutIrradianceEnvironmentMap->G.V[CoefficientIndex] = CoefficientValue.G;
			OutIrradianceEnvironmentMap->B.V[CoefficientIndex] = CoefficientValue.B;
		}
	}
}

