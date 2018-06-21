//#nv begin #flex
#pragma once

#include "ShaderBaseClasses.h"
#include "ScenePrivate.h"

class IFlexFluidSurfaceRenderer
{
public:
	/**
	* Texture initialization and particle depth rendering and smoothing.
	*/
	virtual void PreRenderOpaque(FRHICommandList& RHICmdList, const TArray<FViewInfo>& Views) = 0;

	/**
	* Render thickness after scene depth buffer is available.
	*/
	virtual void PostRenderOpaque(FRHICommandList& RHICmdList, const TArray<FViewInfo>& Views) = 0;
};

#if WITH_FLEX
extern RENDERER_API class IFlexFluidSurfaceRenderer* GFlexFluidSurfaceRenderer;
#endif
//#nv end
