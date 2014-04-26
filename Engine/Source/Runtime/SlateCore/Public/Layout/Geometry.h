// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Geometry.h: Declares the FGeometry class.
=============================================================================*/

#pragma once


class FArrangedWidget;
class SWidget;


/**
 * Represents the position, size, and absolute position of a Widget in Slate.
 * The absolute location of a geometry is usually screen space or
 * window space depending on where the geometry originated.
 * Geometries are usually paired with a SWidget pointer in order
 * to provide information about a specific widget (see FArrangedWidget).
 * A Geometry's parent is generally thought to be the Geometry of the
 * the corresponding parent widget.
 */
class SLATECORE_API FGeometry
{

public:

	/**
	 * Default constructor.
	 */
	FGeometry( )
		: Position(0.0f, 0.0f)
		, AbsolutePosition(0.0f, 0.0f)
		, Size(0.0f, 0.0f)
		, Scale(1.0f)
	{ }

	/**
	 * Construct a new geometry given the following parameters:
	 * 
	 * @param OffsetFromParent         Local position of this geometry within its parent geometry.
	 * @param ParentAbsolutePosition   The absolute position of the parent geometry containing this geometry.
	 * @param InSize                   The size of this geometry.
	 * @param InScale                  The scale of this geometry with respect to Normal Slate Coordinates.
	 */
	FGeometry( const FVector2D& OffsetFromParent, const FVector2D& ParentAbsolutePosition, const FVector2D& InSize, float InScale )
		: Position(OffsetFromParent)
		, AbsolutePosition(ParentAbsolutePosition + (OffsetFromParent * InScale))
		, Size(InSize)	
		, Scale(InScale)
	{ }

public:

	/**
	 * Test geometries for strict equality (i.e. exact float equality).
	 *
	 * @param Other A geometry to compare to.
	 *
	 * @return true if this is equal to other, false otherwise.
	 */
	bool operator==( const FGeometry& Other ) const
	{
		return
			this->Position == Other.Position &&
			this->AbsolutePosition == Other.AbsolutePosition &&
			this->Size == Other.Size &&
			this->Scale == Other.Scale;
	}
	
	/**
	 * Test geometries for strict inequality (i.e. exact float equality negated).
	 *
	 * @param Other A geometry to compare to.
	 *
	 * @return false if this is equal to other, true otherwise.
	 */
	bool operator!=( const FGeometry& Other ) const
	{
		return !( this->operator==(Other) );
	}

public:
	
	/**
	 * Create a child geometry; i.e. a geometry relative to this geometry.
	 *
	 * @param ChildOffset  The offset of the child from this Geometry's position.
	 * @param ChildSize    The size of the child geometry in screen units.
	 * @param InScale      How much to scale this child with respect to its parent.
	 *
	 * @return A Geometry that is offset from this geometry by ChildOffset
	 */
	FGeometry MakeChild( const FVector2D& ChildOffset, const FVector2D& ChildSize, float InScale = 1.0f ) const
	{
		return FGeometry( ChildOffset, this->AbsolutePosition, ChildSize, this->Scale * InScale );
	}
	
	/**
	 * Create a child widget geometry; i.e. a geometry relative to this geometry that
	 * is also associated with a Widget.
	 *
	 * @param InWidget     The widget to associate with the newly created child geometry
	 * @param ChildOffset  The offset of the child from this Geometry's position.
	 * @param ChildSize    The size of the child geometry in screen units.
	 * @param InScale      How much to scale this child with respect to its parent.
	 *
	 * @return A WidgetGeometry that is offset from this geometry by ChildOffset
	 */
	FArrangedWidget MakeChild( const TSharedRef<SWidget>& InWidget, const FVector2D& ChildOffset, const FVector2D& ChildSize, float InScale = 1.0f ) const;

	/**
	 * Make an FPaintGeometry in this FGeometry at the specified Offset.
	 *
	 * @param InOffset   Offset in SlateUnits from this FGeometry's origin.
	 * @param InSize     The size of this paint element in SlateUnits.
	 * @param InScale    Additional scaling to apply to the DrawElements.
	 *
	 * @return a FPaintGeometry derived this FGeometry.
	 */
	FPaintGeometry ToPaintGeometry( FVector2D InOffset, FVector2D InSize, float InScale ) const;

	/** A PaintGeometry that is not scaled additionally with respect to its originating FGeometry */
	FPaintGeometry ToPaintGeometry( FVector2D InOffset, FVector2D InSize ) const
	{
		return ToPaintGeometry( InOffset, InSize, 1.0f );
	}

	/** Convert the FGeometry to an FPaintGeometry with no changes */
	FPaintGeometry ToPaintGeometry() const
	{
		return ToPaintGeometry( FVector2D(0,0), this->Size, 1.0f );
	}

	/** An FPaintGeometry that is offset into the FGeometry from which it originates and occupies all the available room */
	FPaintGeometry ToOffsetPaintGeometry( FVector2D InOffset ) const
	{
		return ToPaintGeometry( InOffset, this->Size, 1.0f );
	}

	/**
	 * Make an FPaintGeometry from this FGeometry by inflating the FGeometry by InflateAmount.
	 */
	FPaintGeometry ToInflatedPaintGeometry( const FVector2D& InflateAmount ) const;

	FPaintGeometry CenteredPaintGeometryOnLeft( const FVector2D& SizeBeingAligned, float InScale ) const;

	FPaintGeometry CenteredPaintGeometryOnRight( const FVector2D& SizeBeingAligned, float InScale ) const;

	FPaintGeometry CenteredPaintGeometryBelow( const FVector2D& SizeBeingAligned, float InScale ) const;


	/** @return true if the provided location is within the bounds of this geometry. */
	const bool IsUnderLocation( const FVector2D& AbsoluteCoordinate ) const
	{
		return
			( AbsolutePosition.X <= AbsoluteCoordinate.X ) && ( ( AbsolutePosition.X + Size.X * Scale ) > AbsoluteCoordinate.X ) &&
			( AbsolutePosition.Y <= AbsoluteCoordinate.Y ) && ( ( AbsolutePosition.Y + Size.Y * Scale ) > AbsoluteCoordinate.Y );
	}

	/** @return Translates AbsoluteCoordinate into the space defined by this Geometry. */
	const FVector2D AbsoluteToLocal( FVector2D AbsoluteCoordinate ) const
	{
		return (AbsoluteCoordinate - this->AbsolutePosition) / Scale;
	}

	/**
	 * Translates local coordinates into absolute coordinates
	 *
	 * @return  Absolute coordinates
	 */
	const FVector2D LocalToAbsolute( FVector2D LocalCoordinate ) const
	{
		return (LocalCoordinate * Scale) + this->AbsolutePosition;
	}
	
	/**
	 * Returns the allocated geometry's relative position and size as a rect
	 *
	 * @return  Allotted geometry relative position and size rectangle
	 */
	FSlateRect GetRect( ) const
	{
		return FSlateRect( Position.X, Position.Y, Position.X + Size.X, Position.Y + Size.Y );
	}

	/**
	 * Returns a clipping rectangle corresponding to the allocated geometry's absolute position and size.
	 * Note that the clipping rectangle starts 1 pixel above and left of the geometry because clipping is not
	 * inclusive on the lower bound.
	 *
	 * @return  Allotted geometry absolute position and size rectangle
	 */
	FSlateRect GetClippingRect( ) const
	{
		return FSlateRect( AbsolutePosition.X, AbsolutePosition.Y, AbsolutePosition.X + Size.X * Scale, AbsolutePosition.Y + Size.Y * Scale );
	}
	
	/** @return A String representation of this Geometry */
	FString ToString( ) const
	{
		return FString::Printf(TEXT("[Pos=%s, Abs=%s, Size=%s]"), *Position.ToString(), *AbsolutePosition.ToString(), *Size.ToString() );
	}

	/** @return the size of the geometry in screen space */
	FVector2D GetDrawSize() const
	{
		return Size * Scale;
	}

public:

	/** Position relative to the parent */
	FVector2D Position;

	/** Position in screen space or (window space during rendering) */
	FVector2D AbsolutePosition;	

	/** The dimensions */
	FVector2D Size;

	/** How much this geometry is scaled: WidgetCoodinates / SlateCoordinates */
	float Scale;
};


template <> struct TIsPODType<FGeometry> { enum { Value = true }; };
