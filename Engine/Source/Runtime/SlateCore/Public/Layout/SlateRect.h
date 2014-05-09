// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateRect.h: Declares the FSlateRect structure.
=============================================================================*/

#pragma once


/** 
 * A rectangle defined by upper-left and lower-right corners
 */
class SLATECORE_API FSlateRect
{
public:

	float Left;
	float Top;
	float Right;
	float Bottom;

	FSlateRect( float InLeft = -1, float InTop = -1, float InRight = -1, float InBottom = -1 )
		: Left(InLeft)
		, Top(InTop)
		, Right(InRight)
		, Bottom(InBottom)
	{ }

	FSlateRect( const FVector2D& InStartPos, const FVector2D& InEndPos )
		: Left(InStartPos.X)
		, Top(InStartPos.Y)
		, Right(InEndPos.X)
		, Bottom(InEndPos.Y)
	{ }
	
public:

	bool IsValid() const
	{
		return Left >= 0 && Right >= 0 && Bottom >=0 && Top >= 0;
	}


	/**
	 * @return true, if the rectangle is empty. Should be used in conjunction with IntersectionWith.
	 */
	bool IsEmpty() const
	{
		return GetSize().Size() == 0.0f;
	}

	/**
	 * Returns the size of the rectangle.
	 *
	 * @return The size as a vector.
	 */
	FVector2D GetSize() const
	{
		return FVector2D( Right - Left, Bottom - Top );
	}

	/**
	 * Returns the center of the rectangle
	 * 
	 * @return The center point.
	 */
	FVector2D GetCenter() const
	{
		return FVector2D( Left, Top ) + GetSize() * 0.5f;
	}

	/**
	 * Returns the top-left position of the rectangle
	 * 
	 * @return The top-left position.
	 */
	FVector2D GetTopLeft() const
	{
		return FVector2D( Left, Top );
	}

	/**
	 * Return a rectangle that is contracted on each side by the amount specified in each margin.
	 *
	 * @param FMargin The amount to contract the geometry.
	 *
	 * @return An inset rectangle.
	 */
	FSlateRect InsetBy( const struct FMargin& InsetAmount ) const;

	/**
	 * Returns the rect that encompasses both rectangles
	 * 
	 * @param	Other	The other rectangle
	 *
	 * @return	Rectangle that is big enough to fit both rectangles
	 */
	FSlateRect Expand( const FSlateRect& Other ) const
	{
		return FSlateRect( FMath::Min( Left, Other.Left ), FMath::Min( Top, Other.Top ), FMath::Max( Right, Other.Right ), FMath::Max( Bottom, Other.Bottom ) );
	}

	
	FSlateRect IntersectionWith( const FSlateRect& Other ) const 
	{
		FSlateRect Intersected( FMath::Max( this->Left, Other.Left ), FMath::Max(this->Top, Other.Top), FMath::Min( this->Right, Other.Right ), FMath::Min( this->Bottom, Other.Bottom ) );
		if ( (Intersected.Bottom < Intersected.Top) || (Intersected.Right < Intersected.Left) )
		{
			// The intersection has 0 area and should not be rendered at all.
			return FSlateRect(0,0,0,0);
		}
		else
		{
			return Intersected;
		}
	}

	/**
	 * Returns whether or not a point is inside the rectangle
	 * Note: The lower right and bottom points of the rect are not inside the rectangle due to rendering API clipping rules.
	 * 
	 * @param Point	The point to check
	 * @return True if the point is inside the rectangle
	 */
	FORCEINLINE bool ContainsPoint( const FVector2D& Point ) const
	{
		return Point.X >= Left && Point.X < Right && Point.Y >= Top && Point.Y < Bottom;
	}

	bool operator==( const FSlateRect& Other ) const
	{
		return
			Left == Other.Left &&
			Top == Other.Top &&
			Right == Other.Right &&
			Bottom == Other.Bottom;
	}

	bool operator!=( const FSlateRect& Other ) const
	{
		return Left != Other.Left || Top != Other.Top || Right != Other.Right || Bottom != Other.Bottom;
	}

	friend FSlateRect operator+( const FSlateRect& A, const FSlateRect& B )
	{
		return FSlateRect( A.Left + B.Left, A.Top + B.Top, A.Right + B.Right, A.Bottom + B.Bottom );
	}

	friend FSlateRect operator-( const FSlateRect& A, const FSlateRect& B )
	{
		return FSlateRect( A.Left - B.Left, A.Top - B.Top, A.Right - B.Right, A.Bottom - B.Bottom );
	}

	friend FSlateRect operator*( float Scalar, const FSlateRect& Rect )
	{
		return FSlateRect( Rect.Left * Scalar, Rect.Top * Scalar, Rect.Right * Scalar, Rect.Bottom * Scalar );
	}

	/** Do rectangles A and B intersect? */
	static bool DoRectanglesIntersect( const FSlateRect& A, const FSlateRect& B )
	{
		//  Segments A and B do not intersect when:
		//
		//       (left)   A     (right)
		//         o-------------o
		//  o---o        OR         o---o
		//    B                       B
		//
		//
		// We assume the A and B are well-formed rectangles.
		// i.e. (Top,Left) is above and to the left of (Bottom,Right)
		const bool bDoNotOverlap =
			B.Right < A.Left || A.Right < B.Left ||
			B.Bottom < A.Top || A.Bottom < B.Top;

		return ! bDoNotOverlap;
	}

	/** Is rectangle B contained within rectangle A? */
	static bool IsRectangleContained( const FSlateRect& A, const FSlateRect& B )
	{
		return (A.Left <= B.Left) && (A.Right >= B.Right) && (A.Top <= B.Top) && (A.Bottom >= B.Bottom);
	}
};


template<> struct TIsPODType<FSlateRect> { enum { Value = true }; };
