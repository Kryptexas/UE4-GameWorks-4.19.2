// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Slate.h"

class FSlateElementBatch;
class FSlateShaderResource;
struct FSlateVertex;

class FSlateRenderingPolicy
{
public:
	FSlateRenderingPolicy( float InPixelCenterOffset )
		: PixelCenterOffset( InPixelCenterOffset )
	{}
	virtual ~FSlateRenderingPolicy() {}

	virtual void UpdateBuffers( const FSlateWindowElementList& WindowElementList ) = 0;
	virtual FSlateShaderResource* GetViewportResource( const ISlateViewport* InViewportInterface ) = 0;
	virtual FSlateShaderResourceProxy* GetTextureResource( const FSlateBrush& Brush ) = 0;
	virtual TSharedPtr<class FSlateFontCache>& GetFontCache() = 0;
	float GetPixelCenterOffset() const { return PixelCenterOffset; }
private:
	float PixelCenterOffset;
};
