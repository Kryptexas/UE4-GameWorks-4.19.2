// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RHI.h"
#include "RenderResource.h"
#include "Shader.h"
#include "GlobalShader.h"

/**
 * Pixel shader to convert an AYUV texture to RGBA.
 *
 * This shader expects a single texture consisting of a N x M array of pixels
 * in AYUV format. Each pixel is encoded as four consecutive unsigned chars
 * with the following layout: [V0 U0 Y0 A0][V1 U1 Y1 A1]..
 */
class FRGBAToYUV420CS
	: public FGlobalShader
{
	//DECLARE_EXPORTED_SHADER_TYPE(FRGBAToYUV420CS, Global, UTILITYSHADERS_API);
	DECLARE_SHADER_TYPE(FRGBAToYUV420CS, Global);

public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		//return true;  
		return Platform == EShaderPlatform::SP_PS4;
	}

	FRGBAToYUV420CS() { }

	FRGBAToYUV420CS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		OutTextureRW.Bind(Initializer.ParameterMap, TEXT("OutTexture"), SPF_Mandatory);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(FRHICommandList& RHICmdList, TRefCountPtr<FRHITexture2D> SrcTex, FUnorderedAccessViewRHIParamRef OutUAV, float TargetHeight, float ScaleFactorX, float ScaleFactorY, float TextureYOffset);
	void UnbindBuffers(FRHICommandList& RHICmdList);

protected:
	FShaderResourceParameter OutTextureRW;
};
