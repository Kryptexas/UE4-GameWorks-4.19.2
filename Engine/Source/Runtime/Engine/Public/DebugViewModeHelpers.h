// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/**
 * Definition and helpers for debug view modes
 */

#pragma once

/** 
 * Enumeration for different Quad Overdraw visualization mode.
 */
enum EDebugViewShaderMode
{
	DVSM_None,						// No debug view.
	DVSM_ShaderComplexity,			// Default shader complexity viewmode
	DVSM_ShaderComplexityContainedQuadOverhead,	// Show shader complexity with quad overdraw scaling the PS instruction count.
	DVSM_ShaderComplexityBleedingQuadOverhead,	// Show shader complexity with quad overdraw bleeding the PS instruction count over the quad.
	DVSM_QuadComplexity,			// Show quad overdraw only.
	DVSM_WantedMipsAccuracy,		// Accuraty of the wanted mips computed by the texture streamer.
	DVSM_TexelFactorAccuracy,		// Accuraty of the texel factor computed on each mesh.
	DVSM_TexCoordScaleAccuracy,		// Accuracy of the material texture coordinate scales computed in the texture streaming build.
	DVSM_TexCoordScaleAnalysis,		// Outputs each texture lookup texcoord scale.
	DVSM_MAX
};

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
/** Returns true if the specified shadermode is available for the given shader platform. Called for shader compilation shader compilation. */
extern ENGINE_API bool AllowDebugViewShaderMode(EDebugViewShaderMode ShaderMode, EShaderPlatform Platform);
/** Returns true if the specified shadermode is currently available for the given feature level on the current platform. */
extern ENGINE_API bool AllowDebugViewShaderMode(EDebugViewShaderMode ShaderMode, ERHIFeatureLevel::Type FeatureLevel);
/** Returns true if the vertex shader (and potential hull and domain) should be compiled on the given platform. */
extern ENGINE_API bool AllowDebugViewVSDSHS(EShaderPlatform Platform);
/** Returns true if the vertex shader (and potential hull and domain) should be compiled for the given feature level on the current platform. */
extern ENGINE_API bool AllowDebugViewVSDSHS(ERHIFeatureLevel::Type FeatureLevel);
#else
FORCEINLINE bool AllowDebugViewShaderMode(EDebugViewShaderMode ShaderMode, EShaderPlatform Platform) { return false; }
FORCEINLINE bool AllowDebugViewShaderMode(EDebugViewShaderMode ShaderMode, ERHIFeatureLevel::Type FeatureLevel)  { return false; }
FORCEINLINE bool AllowDebugViewVSDSHS(EShaderPlatform Platform) { return false; }
FORCEINLINE bool AllowDebugViewVSDSHS(ERHIFeatureLevel::Type FeatureLevel) { return false; }
#endif



