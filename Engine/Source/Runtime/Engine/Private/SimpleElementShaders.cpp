// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	SimpleElementShaders.h: Definitions for simple element shaders.
==============================================================================*/

#include "EnginePrivate.h"
#include "SimpleElementShaders.h"

/*------------------------------------------------------------------------------
	Simple element vertex shader.
------------------------------------------------------------------------------*/

FSimpleElementVS::FSimpleElementVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
	FGlobalShader(Initializer)
{
	Transform.Bind(Initializer.ParameterMap,TEXT("Transform"), SPF_Mandatory);
	SwitchVerticalAxis.Bind(Initializer.ParameterMap,TEXT("SwitchVerticalAxis"), SPF_Optional);
}

void FSimpleElementVS::SetParameters(const FMatrix& TransformValue, bool bSwitchVerticalAxis)
{
	SetShaderValue(GetVertexShader(),Transform,TransformValue);
	SetShaderValue(GetVertexShader(),SwitchVerticalAxis, bSwitchVerticalAxis ? -1.0f : 1.0f);
}

bool FSimpleElementVS::Serialize(FArchive& Ar)
{
	bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
	Ar << Transform << SwitchVerticalAxis;
	return bShaderHasOutdatedParameters;
}

void FSimpleElementVS::ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
	OutEnvironment.SetDefine(TEXT("ALLOW_SWITCH_VERTICALAXIS"), 1);
}

/*------------------------------------------------------------------------------
	Simple element pixel shaders.
------------------------------------------------------------------------------*/

FSimpleElementPS::FSimpleElementPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
	FGlobalShader(Initializer)
{
	InTexture.Bind(Initializer.ParameterMap,TEXT("InTexture"));
	InTextureSampler.Bind(Initializer.ParameterMap,TEXT("InTextureSampler"));
	TextureComponentReplicate.Bind(Initializer.ParameterMap,TEXT("TextureComponentReplicate"));
	TextureComponentReplicateAlpha.Bind(Initializer.ParameterMap,TEXT("TextureComponentReplicateAlpha"));
	SceneDepthTextureNonMS.Bind(Initializer.ParameterMap,TEXT("SceneDepthTextureNonMS"));
	EditorCompositeDepthTestParameter.Bind(Initializer.ParameterMap,TEXT("bEnableEditorPrimitiveDepthTest"));
	ScreenToPixel.Bind(Initializer.ParameterMap,TEXT("ScreenToPixel"));
}

void FSimpleElementPS::SetEditorCompositingParameters( const FSceneView* View, FTexture2DRHIRef DepthTexture )
{
	if( View )
	{
		FGlobalShader::SetParameters( GetPixelShader(), *View );

		FIntRect DestRect = View->ViewRect;
		FIntPoint ViewportOffset = DestRect.Min;
		FIntPoint ViewportExtent = DestRect.Size();

		FVector4 ScreenPosToPixelValue(
			ViewportExtent.X * 0.5f,
			-ViewportExtent.Y * 0.5f, 
			ViewportExtent.X * 0.5f - 0.5f + ViewportOffset.X,
			ViewportExtent.Y * 0.5f - 0.5f + ViewportOffset.Y);

		SetShaderValue(GetPixelShader(), ScreenToPixel, ScreenPosToPixelValue);

		SetShaderValue(GetPixelShader(),EditorCompositeDepthTestParameter,IsValidRef(DepthTexture) );
	}
	else
	{
		// Unset the view uniform buffer since we don't have a view
		SetUniformBufferParameter(GetPixelShader(), GetUniformBufferParameter<FViewUniformShaderParameters>(), NULL);
		SetShaderValue(GetPixelShader(),EditorCompositeDepthTestParameter,false );
	}

	// Bind the zbuffer as a texture if depth textures are supported
	SetTextureParameter(GetPixelShader(), SceneDepthTextureNonMS, IsValidRef(DepthTexture) ? (FTextureRHIRef)DepthTexture : GWhiteTexture->TextureRHI);
}

void FSimpleElementPS::SetParameters(const FTexture* TextureValue)
{
	SetTextureParameter(GetPixelShader(),InTexture,InTextureSampler,TextureValue);
	
	SetShaderValue(GetPixelShader(),TextureComponentReplicate,TextureValue->bGreyScaleFormat ? FLinearColor(1,0,0,0) : FLinearColor(0,0,0,0));
	SetShaderValue(GetPixelShader(),TextureComponentReplicateAlpha,TextureValue->bGreyScaleFormat ? FLinearColor(1,0,0,0) : FLinearColor(0,0,0,1));
}

bool FSimpleElementPS::Serialize(FArchive& Ar)
{
	bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
	Ar << InTexture;
	Ar << InTextureSampler;
	Ar << TextureComponentReplicate;
	Ar << TextureComponentReplicateAlpha;
	Ar << SceneDepthTextureNonMS;
	Ar << EditorCompositeDepthTestParameter;
	Ar << ScreenToPixel;
	return bShaderHasOutdatedParameters;
}

FSimpleElementGammaPS::FSimpleElementGammaPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
	FSimpleElementPS(Initializer)
{
	Gamma.Bind(Initializer.ParameterMap,TEXT("Gamma"));
}

void FSimpleElementGammaPS::SetParameters(const FTexture* Texture,float GammaValue,ESimpleElementBlendMode BlendMode)
{
	FSimpleElementPS::SetParameters(Texture);
	SetShaderValue(GetPixelShader(),Gamma,GammaValue);
}

bool FSimpleElementGammaPS::Serialize(FArchive& Ar)
{
	bool bShaderHasOutdatedParameters = FSimpleElementPS::Serialize(Ar);
	Ar << Gamma;
	return bShaderHasOutdatedParameters;
}

FSimpleElementMaskedGammaPS::FSimpleElementMaskedGammaPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
	FSimpleElementGammaPS(Initializer)
{
	ClipRef.Bind(Initializer.ParameterMap,TEXT("ClipRef"), SPF_Mandatory);
}

void FSimpleElementMaskedGammaPS::SetParameters(const FTexture* Texture,float Gamma,float ClipRefValue,ESimpleElementBlendMode BlendMode)
{
	FSimpleElementGammaPS::SetParameters(Texture,Gamma,BlendMode);
	SetShaderValue(GetPixelShader(),ClipRef,ClipRefValue);
}

bool FSimpleElementMaskedGammaPS::Serialize(FArchive& Ar)
{
	bool bShaderHasOutdatedParameters = FSimpleElementGammaPS::Serialize(Ar);
	Ar << ClipRef;
	return bShaderHasOutdatedParameters;
}

/**
* Constructor
*
* @param Initializer - shader initialization container
*/
FSimpleElementDistanceFieldGammaPS::FSimpleElementDistanceFieldGammaPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
:	FSimpleElementMaskedGammaPS(Initializer)
{
	SmoothWidth.Bind(Initializer.ParameterMap,TEXT("SmoothWidth"));
	EnableShadow.Bind(Initializer.ParameterMap,TEXT("EnableShadow"));
	ShadowDirection.Bind(Initializer.ParameterMap,TEXT("ShadowDirection"));
	ShadowColor.Bind(Initializer.ParameterMap,TEXT("ShadowColor"));
	ShadowSmoothWidth.Bind(Initializer.ParameterMap,TEXT("ShadowSmoothWidth"));
	EnableGlow.Bind(Initializer.ParameterMap,TEXT("EnableGlow"));
	GlowColor.Bind(Initializer.ParameterMap,TEXT("GlowColor"));
	GlowOuterRadius.Bind(Initializer.ParameterMap,TEXT("GlowOuterRadius"));
	GlowInnerRadius.Bind(Initializer.ParameterMap,TEXT("GlowInnerRadius"));
}

/**
* Sets all the constant parameters for this shader
*
* @param Texture - 2d tile texture
* @param Gamma - if gamma != 1.0 then a pow(color,Gamma) is applied
* @param ClipRef - reference value to compare with alpha for killing pixels
* @param SmoothWidth - The width to smooth the edge the texture
* @param EnableShadow - Toggles drop shadow rendering
* @param ShadowDirection - 2D vector specifying the direction of shadow
* @param ShadowColor - Color of the shadowed pixels
* @param ShadowSmoothWidth - The width to smooth the edge the shadow of the texture
* @param BlendMode - current batched element blend mode being rendered
*/
void FSimpleElementDistanceFieldGammaPS::SetParameters(
	const FTexture* Texture,
	float Gamma,
	float ClipRef,
	float SmoothWidthValue,
	bool EnableShadowValue,
	const FVector2D& ShadowDirectionValue,
	const FLinearColor& ShadowColorValue,
	float ShadowSmoothWidthValue,
	const FDepthFieldGlowInfo& GlowInfo,
	ESimpleElementBlendMode BlendMode
	)
{
	FSimpleElementMaskedGammaPS::SetParameters(Texture,Gamma,ClipRef,BlendMode);
	SetShaderValue(GetPixelShader(),SmoothWidth,SmoothWidthValue);		
	SetPixelShaderBool(GetPixelShader(),EnableShadow,EnableShadowValue);
	if (EnableShadowValue)
	{
		SetShaderValue(GetPixelShader(),ShadowDirection,ShadowDirectionValue);
		SetShaderValue(GetPixelShader(),ShadowColor,ShadowColorValue);
		SetShaderValue(GetPixelShader(),ShadowSmoothWidth,ShadowSmoothWidthValue);
	}
	SetPixelShaderBool(GetPixelShader(),EnableGlow,GlowInfo.bEnableGlow);
	if (GlowInfo.bEnableGlow)
	{
		SetShaderValue(GetPixelShader(),GlowColor,GlowInfo.GlowColor);
		SetShaderValue(GetPixelShader(),GlowOuterRadius,GlowInfo.GlowOuterRadius);
		SetShaderValue(GetPixelShader(),GlowInnerRadius,GlowInfo.GlowInnerRadius);
	}

	// This shader does not use editor compositing
	SetEditorCompositingParameters( NULL, FTexture2DRHIRef() );
}

/**
* Serialize constant paramaters for this shader
* 
* @param Ar - archive to serialize to
* @return true if any of the parameters were outdated
*/
bool FSimpleElementDistanceFieldGammaPS::Serialize(FArchive& Ar)
{
	bool bShaderHasOutdatedParameters = FSimpleElementMaskedGammaPS::Serialize(Ar);
	Ar << SmoothWidth;
	Ar << EnableShadow;
	Ar << ShadowDirection;
	Ar << ShadowColor;	
	Ar << ShadowSmoothWidth;
	Ar << EnableGlow;
	Ar << GlowColor;
	Ar << GlowOuterRadius;
	Ar << GlowInnerRadius;
	return bShaderHasOutdatedParameters;
}
	
FSimpleElementHitProxyPS::FSimpleElementHitProxyPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
	FGlobalShader(Initializer)
{
	InTexture.Bind(Initializer.ParameterMap,TEXT("InTexture"), SPF_Mandatory);
	InTextureSampler.Bind(Initializer.ParameterMap,TEXT("InTextureSampler"));
}

void FSimpleElementHitProxyPS::SetParameters(const FTexture* TextureValue)
{
	SetTextureParameter(GetPixelShader(),InTexture,InTextureSampler,TextureValue);
}

bool FSimpleElementHitProxyPS::Serialize(FArchive& Ar)
{
	bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
	Ar << InTexture;
	Ar << InTextureSampler;
	return bShaderHasOutdatedParameters;
}


FSimpleElementColorChannelMaskPS::FSimpleElementColorChannelMaskPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
: FGlobalShader(Initializer)
{
	InTexture.Bind(Initializer.ParameterMap,TEXT("InTexture"), SPF_Mandatory);
	InTextureSampler.Bind(Initializer.ParameterMap,TEXT("InTextureSampler"));
	ColorWeights.Bind(Initializer.ParameterMap,TEXT("ColorWeights"));
	Gamma.Bind(Initializer.ParameterMap,TEXT("Gamma"));
}

/**
* Sets all the constant parameters for this shader
*
* @param Texture - 2d tile texture
* @param ColorWeights - reference value to compare with alpha for killing pixels
* @param Gamma - if gamma != 1.0 then a pow(color,Gamma) is applied
*/
void FSimpleElementColorChannelMaskPS::SetParameters(const FTexture* TextureValue, const FMatrix& ColorWeightsValue, float GammaValue)
{
	SetTextureParameter(GetPixelShader(),InTexture,InTextureSampler,TextureValue);
	SetShaderValue(GetPixelShader(),ColorWeights,ColorWeightsValue);
	SetShaderValue(GetPixelShader(),Gamma,GammaValue);
}

bool FSimpleElementColorChannelMaskPS::Serialize(FArchive& Ar)
{
	bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
	Ar << InTexture;
	Ar << InTextureSampler;
	Ar << ColorWeights;
	Ar << Gamma;
	return bShaderHasOutdatedParameters;
}

/*------------------------------------------------------------------------------
	Shader implementations.
------------------------------------------------------------------------------*/

IMPLEMENT_SHADER_TYPE(,FSimpleElementVS,TEXT("SimpleElementVertexShader"),TEXT("Main"),SF_Vertex);
IMPLEMENT_SHADER_TYPE(,FSimpleElementPS,TEXT("SimpleElementPixelShader"),TEXT("Main"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(,FSimpleElementGammaPS,TEXT("SimpleElementPixelShader"),TEXT("GammaMain"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(,FSimpleElementMaskedGammaPS,TEXT("SimpleElementPixelShader"),TEXT("GammaMaskedMain"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(,FSimpleElementDistanceFieldGammaPS,TEXT("SimpleElementPixelShader"),TEXT("GammaDistanceFieldMain"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(,FSimpleElementHitProxyPS,TEXT("SimpleElementHitProxyPixelShader"),TEXT("Main"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(,FSimpleElementColorChannelMaskPS,TEXT("SimpleElementColorChannelMaskPixelShader"),TEXT("Main"),SF_Pixel);

