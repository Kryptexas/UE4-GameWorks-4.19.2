// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ScreenRendering.h: D3D render target implementation.
=============================================================================*/

#pragma once

#include "Engine.h"
#include "Shader.h"
#include "ShaderParameters.h"
#include "GlobalShader.h"

struct FScreenVertex
{
	FVector2D Position;
	FVector2D UV;
};

/** The filter vertex declaration resource type. */
class FScreenVertexDeclaration : public FRenderResource
{
public:
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	// Destructor.
	virtual ~FScreenVertexDeclaration() {}

	virtual void InitRHI()
	{
		FVertexDeclarationElementList Elements;
		Elements.Add(FVertexElement(0,STRUCT_OFFSET(FScreenVertex,Position),VET_Float2,0));
		Elements.Add(FVertexElement(0,STRUCT_OFFSET(FScreenVertex,UV),VET_Float2,1));
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseRHI()
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

extern ENGINE_API TGlobalResource<FScreenVertexDeclaration> GScreenVertexDeclaration;

/**
 * A pixel shader for rendering a textured screen element.
 */
class FScreenPS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FScreenPS,Global,ENGINE_API);
public:

	static bool ShouldCache(EShaderPlatform Platform) { return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::ES2); }

	FScreenPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		InTexture.Bind(Initializer.ParameterMap,TEXT("InTexture"), SPF_Mandatory);
		InTextureSampler.Bind(Initializer.ParameterMap,TEXT("InTextureSampler"));
	}
	FScreenPS() {}

	void SetParameters(const FTexture* Texture)
	{
		SetTextureParameter(GetPixelShader(),InTexture,InTextureSampler,Texture);
	}

	void SetParameters(FSamplerStateRHIParamRef SamplerStateRHI, FTextureRHIParamRef TextureRHI)
	{
		SetTextureParameter(GetPixelShader(),InTexture,InTextureSampler,SamplerStateRHI,TextureRHI);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << InTexture;
		Ar << InTextureSampler;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderResourceParameter InTexture;
	FShaderResourceParameter InTextureSampler;
};

/**
 * A vertex shader for rendering a textured screen element.
 */
class FScreenVS : public FGlobalShader
{
	DECLARE_EXPORTED_SHADER_TYPE(FScreenVS,Global,ENGINE_API);
public:

	static bool ShouldCache(EShaderPlatform Platform) { return true; }

	FScreenVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
	}
	FScreenVS() {}


	void SetParameters(const FSceneView& View)
	{
		FGlobalShader::SetParameters(GetVertexShader(), View);
	}


	virtual bool Serialize(FArchive& Ar)
	{
		return FGlobalShader::Serialize(Ar);
	}
};

class FScreenVSForGS : public FScreenVS
{
	DECLARE_EXPORTED_SHADER_TYPE(FScreenVSForGS,Global,ENGINE_API);
public:

	static bool ShouldCache(EShaderPlatform Platform) { return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::ES2); }

	FScreenVSForGS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
	  FScreenVS(Initializer)
	  {
	  }
	FScreenVSForGS() {}
};

/**
 * Draws a texture rectangle on the screen using normalized (-1 to 1) device coordinates.
 */
extern void DrawScreenQuad(  float X0, float Y0, float U0, float V0, float X1, float Y1, float U1, float V1, const FTexture* Texture );

extern void ENGINE_API DrawNormalizedScreenQuad(  float X0, float Y0, float U0, float V0, float X1, float Y1, float U1, float V1, FIntPoint TargetSize, FTexture2DRHIRef TextureRHI );
