// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RenderingPolicy.h: Declares the FRenderingPolicy class.
=============================================================================*/

#pragma once


class FSlateElementBatch;
class FSlateShaderResource;
struct FSlateVertex;


/**
 * Abstract base class for Slate rendering policies.
 */
class FSlateRenderingPolicy
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InPixelCenterOffset
	 */
	FSlateRenderingPolicy( float InPixelCenterOffset )
		: PixelCenterOffset( InPixelCenterOffset )
	{ }

	/**
	 * Virtual constructor.
	 */
	virtual ~FSlateRenderingPolicy( ) { }

	virtual void UpdateBuffers( const FSlateWindowElementList& WindowElementList ) = 0;

	virtual TSharedPtr<class FSlateFontCache> GetFontCache( ) = 0;

	virtual TSharedRef<class FSlateShaderResourceManager> GetResourceManager() = 0;

	float GetPixelCenterOffset( ) const
	{
		return PixelCenterOffset;
	}

private:
	FSlateRenderingPolicy(const FSlateRenderingPolicy&);
	FSlateRenderingPolicy& operator=(const FSlateRenderingPolicy&);
private:
	float PixelCenterOffset;
};
