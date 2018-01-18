// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
FlexFluidSurfaceRendering.h: Flex fluid surface rendering definitions.
=============================================================================*/

#pragma once

#include "GameWorks/IFlexFluidSurfaceRendering.h"

class FFlexFluidSurfaceRenderer : public IFlexFluidSurfaceRenderer
{
public:
	static FFlexFluidSurfaceRenderer& get()
	{
		static FFlexFluidSurfaceRenderer instance;
		return instance;
	}

	virtual void PreRenderOpaque(FRHICommandList& RHICmdList, const TArray<FViewInfo>& Views) override;
	virtual void PostRenderOpaque(FRHICommandList& RHICmdList, const TArray<FViewInfo>& Views) override;
};
