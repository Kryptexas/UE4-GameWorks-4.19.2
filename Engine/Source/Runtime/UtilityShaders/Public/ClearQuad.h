// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RHIDefinitions.h"

class FRHICommandList;
struct FRWBufferStructured;
struct FRWBuffer;
struct FSceneRenderTargetItem;
class FRHIUnorderedAccessView;

extern UTILITYSHADERS_API const uint32 GMaxSizeUAVDMA;
extern UTILITYSHADERS_API void ClearUAV(FRHICommandList& RHICmdList, ERHIFeatureLevel::Type FeatureLevel, const FRWBufferStructured& StructuredBuffer, uint32 Value);
extern UTILITYSHADERS_API void ClearUAV(FRHICommandList& RHICmdList, ERHIFeatureLevel::Type FeatureLevel, const FRWBuffer& Buffer, uint32 Value);

extern UTILITYSHADERS_API void ClearUAV(FRHICommandList& RHICmdList, ERHIFeatureLevel::Type FeatureLevel, const FSceneRenderTargetItem& RenderTargetItem, const float(&ClearValues)[4]);
extern UTILITYSHADERS_API void ClearUAV(FRHICommandList& RHICmdList, ERHIFeatureLevel::Type FeatureLevel, const FSceneRenderTargetItem& RenderTargetItem, const uint32(&ClearValues)[4]);
extern UTILITYSHADERS_API void ClearUAV(FRHICommandList& RHICmdList, ERHIFeatureLevel::Type FeatureLevel, const FSceneRenderTargetItem& RenderTargetItem,  const FLinearColor& ClearColor);

extern UTILITYSHADERS_API void ClearTexture2DUAV(FRHICommandList& RHICmdList, ERHIFeatureLevel::Type FeatureLevel, FRHIUnorderedAccessView* UAV, int32 Width, int32 Height, const FLinearColor& ClearColor);


extern UTILITYSHADERS_API void DrawClearQuadMRT( FRHICommandList& RHICmdList, ERHIFeatureLevel::Type FeatureLevel, bool bClearColor, int32 NumClearColors, const FLinearColor* ClearColorArray, bool bClearDepth, float Depth, bool bClearStencil, uint32 Stencil );
extern UTILITYSHADERS_API void DrawClearQuadMRT( FRHICommandList& RHICmdList, ERHIFeatureLevel::Type FeatureLevel, bool bClearColor, int32 NumClearColors, const FLinearColor* ClearColorArray, bool bClearDepth, float Depth, bool bClearStencil, uint32 Stencil, FIntPoint ViewSize, FIntRect ExcludeRect );

inline void DrawClearQuad(FRHICommandList& RHICmdList, ERHIFeatureLevel::Type FeatureLevel, bool bClearColor, const FLinearColor& Color, bool bClearDepth, float Depth, bool bClearStencil, uint32 Stencil)
{
	DrawClearQuadMRT(RHICmdList, FeatureLevel, bClearColor, 1, &Color, bClearDepth, Depth, bClearStencil, Stencil);
}

inline void DrawClearQuad(FRHICommandList& RHICmdList, ERHIFeatureLevel::Type FeatureLevel, bool bClearColor, const FLinearColor& Color, bool bClearDepth, float Depth, bool bClearStencil, uint32 Stencil, FIntPoint ViewSize, FIntRect ExcludeRect)
{
	DrawClearQuadMRT(RHICmdList, FeatureLevel, bClearColor, 1, &Color, bClearDepth, Depth, bClearStencil, Stencil, ViewSize, ExcludeRect);
}

inline void DrawClearQuad(FRHICommandList& RHICmdList, ERHIFeatureLevel::Type FeatureLevel, const FLinearColor& Color)
{
	DrawClearQuadMRT(RHICmdList, FeatureLevel, true, 1, &Color, false, 0, false, 0);
}