// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

// TODO support ExcludeRect
void DrawClearQuadMRT(bool bClearColor,int32 NumClearColors,const FLinearColor* ClearColorArray,bool bClearDepth,float Depth,bool bClearStencil,uint32 Stencil);

inline void DrawClearQuad(bool bClearColor,const FLinearColor& Color,bool bClearDepth,float Depth,bool bClearStencil,uint32 Stencil)
{
	DrawClearQuadMRT(bClearColor, 1, &Color, bClearDepth, Depth, bClearStencil, Stencil);
}