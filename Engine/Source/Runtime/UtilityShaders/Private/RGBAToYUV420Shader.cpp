// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "RGBAToYUV420Shader.h"
#include "ShaderParameterUtils.h"

#if defined(HAS_MORPHEUS) && HAS_MORPHEUS

/* FRGBAToYUV420CS shader
 *****************************************************************************/

BEGIN_UNIFORM_BUFFER_STRUCT( RGBAToYUV420UB, )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( float, TargetHeight )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( float, ScaleFactorX )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( float, ScaleFactorY )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( float, TextureYOffset )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_TEXTURE( Texture2D, SrcTex )
END_UNIFORM_BUFFER_STRUCT( RGBAToYUV420UB )

IMPLEMENT_UNIFORM_BUFFER_STRUCT(RGBAToYUV420UB, TEXT("RGBAToYUV420UB"));
IMPLEMENT_SHADER_TYPE(, FRGBAToYUV420CS, TEXT("/Engine/Private/RGBAToYUV420.usf"), TEXT("RGBAToYUV420Main"), SF_Compute);


void FRGBAToYUV420CS::SetParameters(FRHICommandList& RHICmdList, TRefCountPtr<FRHITexture2D> SrcTex, FUnorderedAccessViewRHIParamRef OutUAV, float TargetHeight, float ScaleFactorX, float ScaleFactorY, float TextureYOffset)
{
	RGBAToYUV420UB UB;
	{
		UB.SrcTex = SrcTex;
		UB.TargetHeight = TargetHeight;
		UB.ScaleFactorX = ScaleFactorX;
		UB.ScaleFactorY = ScaleFactorY;
		UB.TextureYOffset = TextureYOffset;
	}
	FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

	TUniformBufferRef<RGBAToYUV420UB> Data = TUniformBufferRef<RGBAToYUV420UB>::CreateUniformBufferImmediate(UB, UniformBuffer_SingleFrame);
	SetUniformBufferParameter(RHICmdList, ComputeShaderRHI, GetUniformBufferParameter<RGBAToYUV420UB>(), Data);

	RHICmdList.SetUAVParameter(ComputeShaderRHI, OutTextureRW.GetBaseIndex(), OutUAV);
}

/**
* Unbinds any buffers that have been bound.
*/
void FRGBAToYUV420CS::UnbindBuffers(FRHICommandList& RHICmdList)
{
	FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
	RHICmdList.SetUAVParameter(ComputeShaderRHI, OutTextureRW.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
}

#endif //defined(HAS_MORPHEUS) && HAS_MORPHEUS
