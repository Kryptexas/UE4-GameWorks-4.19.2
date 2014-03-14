// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessCombineLUTs.cpp: Post processing tone mapping implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "PostProcessCombineLUTs.h"
#include "PostProcessing.h"
#include "ScreenRendering.h"

// false:use 256x16 texture / true:use volume texture (faster, requires geometry shader)
// USE_VOLUME_LUT: needs to be the same for C++ and HLSL
static bool UseVolumeTextureLUT() 
{	
	return GRHIFeatureLevel >= ERHIFeatureLevel::SM4;
}

// including the neutral one at index 0
const uint32 GMaxLUTBlendCount = 5;


struct FColorTransform
{
	FColorTransform()
	{
		Reset();
	}
	
	float			MinValue;
	float			MidValue;
	float			MaxValue;

	void Reset()
	{
		MinValue = 0.0f;
		MidValue = 0.5f;
		MaxValue = 1.0f;
	}
};

/*-----------------------------------------------------------------------------
FColorRemapShaderParameters
-----------------------------------------------------------------------------*/

FColorRemapShaderParameters::FColorRemapShaderParameters(const FShaderParameterMap& ParameterMap)
{
	MappingPolynomial.Bind(ParameterMap, TEXT("MappingPolynomial"));
}

void FColorRemapShaderParameters::Set(const FPixelShaderRHIParamRef ShaderRHI)
{
	FColorTransform ColorTransform;

	{
		static const auto ICVarMin = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.Color.Min"));
		static const auto ICVarMid = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.Color.Mid"));
		static const auto ICVarMax = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.Color.Max"));
		
		check(ICVarMin && ICVarMid && ICVarMax);
		
		ColorTransform.MinValue = FMath::Clamp(ICVarMin->GetValueOnRenderThread(), -10.0f, 10.0f);
		ColorTransform.MidValue = FMath::Clamp(ICVarMid->GetValueOnRenderThread(), -10.0f, 10.0f);
		ColorTransform.MaxValue = FMath::Clamp(ICVarMax->GetValueOnRenderThread(), -10.0f, 10.0f);
	}

	{
		// x is the input value, y the output value
		// RGB = a, b, c where y = a * x*x + b * x + c

		float c = ColorTransform.MinValue;
		float b = 4 * ColorTransform.MidValue - 3 * ColorTransform.MinValue - ColorTransform.MaxValue;
		float a = ColorTransform.MaxValue - ColorTransform.MinValue - b;

		SetShaderValue(ShaderRHI, MappingPolynomial, FVector(a, b, c));
	}
}

FArchive& operator<<(FArchive& Ar, FColorRemapShaderParameters& P)
{
	Ar << P.MappingPolynomial;
	return Ar;
}

/**
* A pixel shader for blending multiple LUT to one
*
* @param BlendCount >0
*/
template<uint32 BlendCount>
class FLUTBlenderPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FLUTBlenderPS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) { return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3); }

	FLUTBlenderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer) 
		, ColorRemapShaderParameters(Initializer.ParameterMap)
	{
		// Suppress static code analysis warnings about a potentially ill-defined loop. BlendCount > 0 is valid.
		CA_SUPPRESS(6294)

		// starts as 1 as 0 is the neutral one
		for(uint32 i = 1; i < BlendCount; ++i)
		{
			FString Name = FString::Printf(TEXT("Texture%d"), i);

			TextureParameter[i].Bind(Initializer.ParameterMap, *Name);
			TextureParameterSampler[i].Bind(Initializer.ParameterMap, *(Name + TEXT("Sampler")));
		}
		WeightsParameter.Bind(Initializer.ParameterMap, TEXT("LUTWeights"));
		ColorScale.Bind(Initializer.ParameterMap,TEXT("ColorScale"));
		OverlayColor.Bind(Initializer.ParameterMap,TEXT("OverlayColor"));
	}
	FLUTBlenderPS() {}

	void SetParameters(const FSceneView& View, FTexture* Texture[BlendCount], float Weights[BlendCount])
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		for(uint32 i = 0; i < BlendCount; ++i)
		{
			// we don't need to set the neutral one
			if(i != 0)
			{
				SetTextureParameter(ShaderRHI, TextureParameter[i], TextureParameterSampler[i], Texture[i]);
			}

			SetShaderValue(ShaderRHI, WeightsParameter, Weights[i], i);
		}

		SetShaderValue(ShaderRHI, ColorScale, View.ColorScale);
		SetShaderValue(ShaderRHI, OverlayColor, View.OverlayColor);
		ColorRemapShaderParameters.Set(ShaderRHI);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("BLENDCOUNT"), BlendCount);
		OutEnvironment.SetDefine(TEXT("USE_VOLUME_LUT"), UseVolumeTextureLUT());
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);

		for(uint32 i = 0; i < BlendCount; ++i)
		{
			Ar << TextureParameter[i];
			Ar << TextureParameterSampler[i];
		}
		Ar << WeightsParameter << ColorScale << OverlayColor;
		Ar << ColorRemapShaderParameters;

		return bShaderHasOutdatedParameters;
	}

private: // ---------------------------------------------------

	// [0] is not used as it's the neutral one we do in the shader
	FShaderResourceParameter TextureParameter[GMaxLUTBlendCount];
	FShaderResourceParameter TextureParameterSampler[GMaxLUTBlendCount];
	FShaderParameter WeightsParameter;
	FShaderParameter ColorScale;
	FShaderParameter OverlayColor;
	FColorRemapShaderParameters ColorRemapShaderParameters;
};


IMPLEMENT_SHADER_TYPE(template<>,FLUTBlenderPS<1>,TEXT("PostProcessCombineLUTs"),TEXT("MainPS"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>,FLUTBlenderPS<2>,TEXT("PostProcessCombineLUTs"),TEXT("MainPS"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>,FLUTBlenderPS<3>,TEXT("PostProcessCombineLUTs"),TEXT("MainPS"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>,FLUTBlenderPS<4>,TEXT("PostProcessCombineLUTs"),TEXT("MainPS"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>,FLUTBlenderPS<5>,TEXT("PostProcessCombineLUTs"),TEXT("MainPS"),SF_Pixel);

void SetLUTBlenderShader(FRenderingCompositePassContext& Context, uint32 BlendCount, FTexture* Texture[], float Weights[], const FVolumeBounds& VolumeBounds)
{
	check(BlendCount > 0);

	FShader* LocalPixelShader = 0;
	FGlobalBoundShaderState* LocalBoundShaderState = 0;
	const FSceneView& View = Context.View;

	// A macro to handle setting the filter shader for a specific number of samples.
#define CASE_COUNT(BlendCount) \
	case BlendCount: \
	{ \
		TShaderMapRef<FLUTBlenderPS<BlendCount> > PixelShader(GetGlobalShaderMap()); \
		static FGlobalBoundShaderState BoundShaderState; \
		LocalBoundShaderState = &BoundShaderState;\
		LocalPixelShader = *PixelShader;\
	}; \
	break;

	switch(BlendCount)
	{
		// starts at 1 as we always have at least the neutral one
		CASE_COUNT(1);
		CASE_COUNT(2);
		CASE_COUNT(3);
		CASE_COUNT(4);
		CASE_COUNT(5);
		//	default:
		//		UE_LOG(LogRenderer, Fatal,TEXT("Invalid number of samples: %u"),BlendCount);
	}
#undef CASE_COUNT
	check(LocalBoundShaderState != NULL);
	if(UseVolumeTextureLUT())
	{
		TShaderMapRef<FWriteToSliceVS> VertexShader(GetGlobalShaderMap());
		TShaderMapRef<FWriteToSliceGS> GeometryShader(GetGlobalShaderMap());

		SetGlobalBoundShaderState(*LocalBoundShaderState, GScreenVertexDeclaration.VertexDeclarationRHI, *VertexShader, LocalPixelShader, *GeometryShader);

		VertexShader->SetParameters(VolumeBounds, VolumeBounds.MaxX - VolumeBounds.MinX);
		GeometryShader->SetParameters(VolumeBounds);
	}
	else
	{
		TShaderMapRef<FPostProcessVS> VertexShader(GetGlobalShaderMap());

		SetGlobalBoundShaderState(*LocalBoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, LocalPixelShader);

		VertexShader->SetParameters(Context);
	}
#define CASE_COUNT(BlendCount) \
	case BlendCount: \
	{ \
	TShaderMapRef<FLUTBlenderPS<BlendCount> > PixelShader(GetGlobalShaderMap()); \
	PixelShader->SetParameters(View, Texture, Weights); \
	}; \
	break;

	switch(BlendCount)
	{
		// starts at 1 as we always have at least the neutral one
		CASE_COUNT(1);
		CASE_COUNT(2);
		CASE_COUNT(3);
		CASE_COUNT(4);
		CASE_COUNT(5);
		//	default:
		//		UE_LOG(LogRenderer, Fatal,TEXT("Invalid number of samples: %u"),BlendCount);
	}
#undef CASE_COUNT
}


uint32 FRCPassPostProcessCombineLUTs::FindIndex(const FFinalPostProcessSettings& Settings, UTexture* Tex) const
{
	for(uint32 i = 0; i < (uint32)Settings.ContributingLUTs.Num(); ++i)
	{
		if(Settings.ContributingLUTs[i].LUTTexture == Tex)
		{
			return i;
		}
	}

	return 0xffffffff;
}

uint32 FRCPassPostProcessCombineLUTs::GenerateFinalTable(const FFinalPostProcessSettings& Settings, FTexture* OutTextures[], float OutWeights[], uint32 MaxCount) const
{
	// find the n strongest contributors, drop small contributors
	// (inefficient implementation for many items but count should be small)

	uint32 LocalCount = 1;

	// add the neutral one (done in the shader) as it should be the first and always there
	OutTextures[0] = 0;
	{
		uint32 NeutralIndex = FindIndex(Settings, 0);

		OutWeights[0] = NeutralIndex == 0xffffffff ? 0.0f : Settings.ContributingLUTs[NeutralIndex].Weight;
	}

	float OutWeightsSum = OutWeights[0];
	for(; LocalCount < MaxCount; ++LocalCount)
	{
		uint32 BestIndex = 0xffffffff;
		// find the one with the strongest weight, add until full
		for(uint32 i = 0; i < (uint32)Settings.ContributingLUTs.Num(); ++i)
		{
			bool AlreadyInArray = false;
			{
				UTexture* LUTTexture = Settings.ContributingLUTs[i].LUTTexture; 
				FTexture* Internal = LUTTexture ? LUTTexture->Resource : 0; 
				for(uint32 e = 0; e < LocalCount; ++e)
				{
					if(Internal == OutTextures[e])
					{
						AlreadyInArray = true;
						break;
					}
				}
			}

			if(AlreadyInArray)
			{
				// we already have this one
				continue;
			}

			if(BestIndex != 0xffffffff
				&& Settings.ContributingLUTs[BestIndex].Weight > Settings.ContributingLUTs[i].Weight)
			{
				// we have a better ones, maybe add next time
				continue;
			}

			BestIndex = i;
		}

		if(BestIndex == 0xffffffff)
		{
			// no more elements to process 
			break;
		}

		float BestWeight = Settings.ContributingLUTs[BestIndex].Weight;

		if(BestWeight < 1.0f / 512.0f)
		{
			// drop small contributor 
			break;
		}

		UTexture* BestLUTTexture = Settings.ContributingLUTs[BestIndex].LUTTexture; 
		FTexture* BestInternal = BestLUTTexture ? BestLUTTexture->Resource : 0; 

		OutTextures[LocalCount] = BestInternal;
		OutWeights[LocalCount] = BestWeight;
		OutWeightsSum += BestWeight;
	}

	// normalize
	if(OutWeightsSum > 0.001f)
	{
		float InvOutWeightsSum = 1.0f / OutWeightsSum;

		for(uint32 i = 0; i < LocalCount; ++i)
		{
			OutWeights[i] *= InvOutWeightsSum;
		}
	}
	else
	{
		// neutral only is fully utilized
		OutWeights[0] = 1.0f;
		LocalCount = 1;
	}

	return LocalCount;
}

void FRCPassPostProcessCombineLUTs::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(PostProcessCombineLUTs, DEC_SCENE_ITEMS);

	FTexture* LocalTextures[GMaxLUTBlendCount];
	float LocalWeights[GMaxLUTBlendCount];

	const FSceneView& View = Context.View;
	const FSceneViewFamily& ViewFamily = *(View.Family);

	uint32 LocalCount = 1;

	// set defaults for no LUT
	LocalTextures[0] = 0;
	LocalWeights[0] = 1.0f;

	if(ViewFamily.EngineShowFlags.ColorGrading)
	{
		LocalCount = GenerateFinalTable(Context.View.FinalPostProcessSettings, LocalTextures, LocalWeights, GMaxLUTBlendCount);
	}

	// for a 3D texture, the viewport is 16x16 (per slice), for a 2D texture, it's unwrapped to 256x16
	FIntPoint DestSize(UseVolumeTextureLUT() ? 16 : 256, 16);

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	// Set the view family's render target/viewport.
	RHISetRenderTarget(DestRenderTarget.TargetableTexture, FTextureRHIRef());	
	Context.SetViewportAndCallRHI(0, 0, 0.0f, DestSize.X, DestSize.Y, 1.0f );

	// set the state
	RHISetBlendState(TStaticBlendState<>::GetRHI());
	RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

	const FVolumeBounds VolumeBounds(16);

	SetLUTBlenderShader(Context, LocalCount, LocalTextures, LocalWeights, VolumeBounds);

	if(UseVolumeTextureLUT())
	{
		// use volume texture 16x16x16
		RasterizeToVolumeTexture(VolumeBounds);
	}
	else
	{
		// use unwrapped 2d texture 256x16
		DrawRectangle( 
			0, 0,						// XY
			16 * 16, 16,				// SizeXY
			0, 0,						// UV
			16 * 16, 16,				// SizeUV
			FIntPoint(16 * 16, 16),		// TargetSize
			FIntPoint(16 * 16, 16),		// TextureSize
			EDRF_UseTriangleOptimization);
	}

	RHICopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessCombineLUTs::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = FPooledRenderTargetDesc::Create2DDesc(FIntPoint(256, 16), PF_B8G8R8A8, TexCreate_None, TexCreate_RenderTargetable | TexCreate_ShaderResource, false);

	if(UseVolumeTextureLUT())
	{
		Ret.Extent = FIntPoint(16, 16);
		Ret.Depth = 16;
	}

	Ret.DebugName = TEXT("CombineLUTs");

	return Ret;
}
