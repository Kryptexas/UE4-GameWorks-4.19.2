// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	SimpleElementShaders.h: Definitions for simple element shaders.
==============================================================================*/

#pragma once

#include "GlobalShader.h"
#include "ShaderParameters.h"

/**
 * A vertex shader for rendering a texture on a simple element.
 */
class ENGINE_API FSimpleElementVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSimpleElementVS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) { return true; }

	FSimpleElementVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);
	FSimpleElementVS() {}

	/*ENGINE_API */void SetParameters(const FMatrix& TransformValue, bool bSwitchVerticalAxis = false);

	virtual bool Serialize(FArchive& Ar);

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment);

private:
	FShaderParameter Transform;
	FShaderParameter SwitchVerticalAxis;
};

/**
 * Simple pixel shader that just reads from a texture
 * This is used for resolves and needs to be as efficient as possible
 */
class FSimpleElementPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSimpleElementPS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) { return true; }

	FSimpleElementPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);
	FSimpleElementPS() {}

	/**
	 * Sets parameters for compositing editor primitives
	 *
	 * @param View			SceneView for view constants when compositing
	 * @param DepthTexture	Depth texture to read from when depth testing for compositing.  If not set no compositing will occur
	 */
	void SetEditorCompositingParameters( const FSceneView* View, FTexture2DRHIRef DepthTexture );

	void SetParameters(const FTexture* TextureValue );

	virtual bool Serialize(FArchive& Ar);

private:
	FShaderResourceParameter InTexture;
	FShaderResourceParameter InTextureSampler;
	FShaderParameter TextureComponentReplicate;
	FShaderParameter TextureComponentReplicateAlpha;
	FShaderResourceParameter SceneDepthTextureNonMS;
	FShaderParameter EditorCompositeDepthTestParameter;
	FShaderParameter ScreenToPixel;
};

/**
 * A pixel shader for rendering a texture on a simple element.
 */
class FSimpleElementGammaPS : public FSimpleElementPS
{
	DECLARE_SHADER_TYPE(FSimpleElementGammaPS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) { return true; }

	FSimpleElementGammaPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);
	FSimpleElementGammaPS() {}

	void SetParameters(const FTexture* Texture,float GammaValue,ESimpleElementBlendMode BlendMode);

	virtual bool Serialize(FArchive& Ar);

private:
	FShaderParameter Gamma;
};

/**
 * A pixel shader for rendering a masked texture on a simple element.
 */
class FSimpleElementMaskedGammaPS : public FSimpleElementGammaPS
{
	DECLARE_SHADER_TYPE(FSimpleElementMaskedGammaPS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) { return true; }

	FSimpleElementMaskedGammaPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);
	FSimpleElementMaskedGammaPS() {}

	void SetParameters(const FTexture* Texture,float Gamma,float ClipRefValue,ESimpleElementBlendMode BlendMode);

	virtual bool Serialize(FArchive& Ar);

private:
	FShaderParameter ClipRef;
};

/**
* A pixel shader for rendering a masked texture using signed distance filed for alpha on a simple element.
*/
class FSimpleElementDistanceFieldGammaPS : public FSimpleElementMaskedGammaPS
{
	DECLARE_SHADER_TYPE(FSimpleElementDistanceFieldGammaPS,Global);
public:

	/**
	* Determine if this shader should be compiled
	*
	* @param Platform - current shader platform being compiled
	* @return true if this shader should be cached for the given platform
	*/
	static bool ShouldCache(EShaderPlatform Platform) 
	{ 
		return true; 
	}

	/**
	* Constructor
	*
	* @param Initializer - shader initialization container
	*/
	FSimpleElementDistanceFieldGammaPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	/**
	* Default constructor
	*/
	FSimpleElementDistanceFieldGammaPS() {}

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
	void SetParameters(
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
		);

	/**
	* Serialize constant paramaters for this shader
	* 
	* @param Ar - archive to serialize to
	* @return true if any of the parameters were outdated
	*/
	virtual bool Serialize(FArchive& Ar);
	
private:
	/** The width to smooth the edge the texture */
	FShaderParameter SmoothWidth;
	/** Toggles drop shadow rendering */
	FShaderParameter EnableShadow;
	/** 2D vector specifying the direction of shadow */
	FShaderParameter ShadowDirection;
	/** Color of the shadowed pixels */
	FShaderParameter ShadowColor;	
	/** The width to smooth the edge the shadow of the texture */
	FShaderParameter ShadowSmoothWidth;
	/** whether to turn on the outline glow */
	FShaderParameter EnableGlow;
	/** base color to use for the glow */
	FShaderParameter GlowColor;
	/** outline glow outer radius */
	FShaderParameter GlowOuterRadius;
	/** outline glow inner radius */
	FShaderParameter GlowInnerRadius;
};

/**
 * A pixel shader for rendering a texture on a simple element.
 */
class FSimpleElementHitProxyPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSimpleElementHitProxyPS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) 
	{ 
		return IsPCPlatform(Platform); 
	}


	FSimpleElementHitProxyPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);
	FSimpleElementHitProxyPS() {}

	void SetParameters(const FTexture* TextureValue);

	virtual bool Serialize(FArchive& Ar);

private:
	FShaderResourceParameter InTexture;
	FShaderResourceParameter InTextureSampler;
};


/**
* A pixel shader for rendering a texture with the ability to weight the colors for each channel.
* The shader also features the ability to view alpha channels and desaturate any red, green or blue channels
* 
*/
class FSimpleElementColorChannelMaskPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSimpleElementColorChannelMaskPS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) 
	{ 
		return IsPCPlatform(Platform); 
	}


	FSimpleElementColorChannelMaskPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer);
	FSimpleElementColorChannelMaskPS() {}

	/**
	* Sets all the constant parameters for this shader
	*
	* @param Texture - 2d tile texture
	* @param ColorWeights - reference value to compare with alpha for killing pixels
	* @param Gamma - if gamma != 1.0 then a pow(color,Gamma) is applied
	*/
	void SetParameters(const FTexture* TextureValue, const FMatrix& ColorWeightsValue, float GammaValue);

	virtual bool Serialize(FArchive& Ar);

private:
	FShaderResourceParameter InTexture;
	FShaderResourceParameter InTextureSampler;
	FShaderParameter ColorWeights; 
	FShaderParameter Gamma;
};
