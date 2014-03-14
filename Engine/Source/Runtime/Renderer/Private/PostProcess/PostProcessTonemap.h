// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessTonemap.h: Post processing tone mapping implementation, can add bloom.
=============================================================================*/

#pragma once

#include "RenderingCompositionGraph.h"


static float GrainHalton( int32 Index, int32 Base )
{
	float Result = 0.0f;
	float InvBase = 1.0f / Base;
	float Fraction = InvBase;
	while( Index > 0 )
	{
		Result += ( Index % Base ) * Fraction;
		Index /= Base;
		Fraction *= InvBase;
	}
	return Result;
}

static void GrainRandomFromFrame(FVector* RESTRICT const Constant, uint32 FrameNumber)
{
	Constant->X = GrainHalton(FrameNumber & 1023, 2);
	Constant->Y = GrainHalton(FrameNumber & 1023, 3);
}


void FilmPostSetConstants(FVector4* RESTRICT const Constants, const uint32 TonemapperType16, const FPostProcessSettings* RESTRICT const FinalPostProcessSettings, bool bMobile);

// derives from TRenderingCompositePassBase<InputCount, OutputCount>
// ePId_Input0: SceneColor
// ePId_Input1: BloomCombined (not needed for bDoGammaOnly)
// ePId_Input2: EyeAdaptation (not needed for bDoGammaOnly)
// ePId_Input3: LUTsCombined (not needed for bDoGammaOnly)
class FRCPassPostProcessTonemap : public TRenderingCompositePassBase<4, 1>
{
public:
	// constructor
	FRCPassPostProcessTonemap(bool bInDoGammaOnly = false);

	// interface FRenderingCompositePass ---------

	virtual void Process(FRenderingCompositePassContext& Context);
	virtual void Release() OVERRIDE { delete this; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;

private:
	void SetShader(const FRenderingCompositePassContext& Context);
	bool bDoGammaOnly;
};

// derives from TRenderingCompositePassBase<InputCount, OutputCount>
// ePId_Input0: SceneColor
// ePId_Input1: BloomCombined (not needed for bDoGammaOnly)
// ePId_Input2: Dof (not needed for bDoGammaOnly)
class FRCPassPostProcessTonemapES2 : public TRenderingCompositePassBase<3, 1>
{
public:
	FRCPassPostProcessTonemapES2(bool bInUsedFramebufferFetch) : bUsedFramebufferFetch(bInUsedFramebufferFetch) { }
	virtual void Process(FRenderingCompositePassContext& Context);
	virtual void Release() OVERRIDE { delete this; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;

private:
	bool bUsedFramebufferFetch;
	void SetShader(const FRenderingCompositePassContext& Context);
};


/** Encapsulates the post processing tone map vertex shader. */
class FPostProcessTonemapVS : public FGlobalShader
{
	// This class is in the header so that Temporal AA can share this vertex shader.
	DECLARE_SHADER_TYPE(FPostProcessTonemapVS,Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	/** Default constructor. */
	FPostProcessTonemapVS(){}

public:
	FPostProcessPassParameters PostprocessParameter;
	FShaderResourceParameter EyeAdaptation;
	FShaderParameter GrainRandomFull;

	/** Initialization constructor. */
	FPostProcessTonemapVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		EyeAdaptation.Bind(Initializer.ParameterMap, TEXT("EyeAdaptation"));
		GrainRandomFull.Bind(Initializer.ParameterMap, TEXT("GrainRandomFull"));
	}

	void SetVS(const FRenderingCompositePassContext& Context)
	{
		const FVertexShaderRHIParamRef ShaderRHI = GetVertexShader();

		FGlobalShader::SetParameters(ShaderRHI, Context.View);

		PostprocessParameter.SetVS(ShaderRHI, Context, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());

		FVector GrainRandomFullValue;
		GrainRandomFromFrame(&GrainRandomFullValue, Context.View.FrameNumber);
		SetShaderValue(ShaderRHI, GrainRandomFull, GrainRandomFullValue);

		if(EyeAdaptation.IsBound())
		{
			IPooledRenderTarget* EyeAdaptationRT = Context.View.GetEyeAdaptation();

			if(EyeAdaptationRT)
			{
				SetTextureParameter(ShaderRHI, EyeAdaptation, EyeAdaptationRT->GetRenderTargetItem().TargetableTexture);
			}
			else
			{
				// some views don't have a state, thumbnail rendering?
				SetTextureParameter(ShaderRHI, EyeAdaptation, GWhiteTexture->TextureRHI);
			}
		}
	}
	
	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << GrainRandomFull << EyeAdaptation;
		return bShaderHasOutdatedParameters;
	}
};
