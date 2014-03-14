// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	NormalMapPreview.h: Definitions for previewing normal maps.
==============================================================================*/

#pragma once

#include "BatchedElements.h"

/**
 * Batched element parameters for previewing normal maps.
 */
class UNREALED_API FNormalMapBatchedElementParameters : public FBatchedElementParameters
{
	/** Binds vertex and pixel shaders for this element */
	virtual void BindShaders_RenderThread(
		const FMatrix& InTransform,
		const float InGamma,
		const FMatrix& ColorWeights,
		const FTexture* Texture) OVERRIDE;
};
