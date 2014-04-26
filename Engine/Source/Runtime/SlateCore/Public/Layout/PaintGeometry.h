// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PaintGeometry.h: Declares the FPaintGeometry structure.
=============================================================================*/

#pragma once


/**
 * A Paint geometry is a draw-space rectangle.
 * The DrawPosition and DrawSize are already positioned and
 * scaled for immediate consumption by the draw code.
 *
 * The DrawScale is only applied to the internal aspects of the draw primitives. e.g. Line thickness, 3x3 grid margins, etc.
 */
struct SLATECORE_API FPaintGeometry
{
	/** Render-space position at which we will draw */
	FVector2D DrawPosition;

	/**
	 * The size in render-space that we want to make this element.
	 * This field is ignored by some elements (e.g. spline, lines)
	 */
	FVector2D DrawSize;

	/** Only affects the draw aspects of individual draw elements. e.g. line thickness, 3x3 grid margins */
	float DrawScale;

public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InDrawPosition The draw position.
	 * @param InDrawSize The draw size.
	 * @param InDrawScale The draw scale.
	 */
	FPaintGeometry( FVector2D InDrawPosition, FVector2D InDrawSize, float InDrawScale )
		: DrawPosition(InDrawPosition)
		, DrawSize (InDrawSize)
		, DrawScale(InDrawScale)
	{ }

public:

	/**
	 * Converts this paint geometry to a Slate rectangle.
	 *
	 * @return The Slate rectangle.
	 */
	FSlateRect ToSlateRect( ) const
	{
		return FSlateRect(DrawPosition.X, DrawPosition.Y, DrawPosition.X + DrawSize.X, DrawPosition.Y + DrawSize.Y);
	}

public:

	static const FPaintGeometry& Identity( )
	{
		static FPaintGeometry IdentityPaintGeometry
		(
			FVector2D(0.0f, 0.0f),	// Position: Draw at the origin of draw space
			FVector2D(0.0f, 0.0f),	// Size    : Size is ignored by splines
			1.0f					// Scale   : Do not do any scaling.
		);

		return IdentityPaintGeometry;
	}
};


template<> struct TIsPODType<FPaintGeometry> { enum { Value = true }; };
