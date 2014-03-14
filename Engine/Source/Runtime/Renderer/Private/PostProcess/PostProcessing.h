// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessing.h: The center for all post processing activities.
=============================================================================*/

#pragma once

#include "RenderingCompositionGraph.h"
#include "PostProcessInput.h"
#include "PostProcessOutput.h"

/** The context used to setup a post-process pass. */
class FPostprocessContext
{
public:

	FPostprocessContext(FRenderingCompositionGraph& InGraph, const FViewInfo& InView);

	FRenderingCompositionGraph& Graph;
	const FViewInfo& View;

	FRenderingCompositePass* SceneColor;
	FRenderingCompositePass* SceneDepth;
	FRenderingCompositePass* GBufferA;

	FRenderingCompositeOutputRef FinalOutput;
};

/** Encapsulates the post processing vertex shader. */
class FPostProcessVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessVS,Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	/** Default constructor. */
	FPostProcessVS() {}

	/** to have a similar interface as all other shaders */
	void SetParameters(const FRenderingCompositePassContext& Context)
	{
		FGlobalShader::SetParameters(GetVertexShader(), Context.View);
	}

	void SetParameters(const FSceneView& View)
	{
		FGlobalShader::SetParameters(GetVertexShader(), View);
	}

public:

	/** Initialization constructor. */
	FPostProcessVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
	}
};


/**
 * The center for all post processing activities.
 */
class FPostProcessing
{
public:
	// @param VelocityRT only valid if motion blur is supported
	void Process(const FViewInfo& View, TRefCountPtr<IPooledRenderTarget>& VelocityRT);

	void ProcessES2( const FViewInfo& View, bool bUsedFramebufferFetch );
};

/** The global used for post processing. */
extern FPostProcessing GPostProcessing;

/**
 * Used to specify the rectangles used by some passes.
 */
namespace EPostProcessRectSource
{
	enum Type
	{
		// The default behavior for most cases. represents the entire view rect.
		GBS_ViewRect,
		// Specifies that the rectangle(s) for the post process operation will use the UIBlurRects container.
		GBS_UIBlurRects
	};
}