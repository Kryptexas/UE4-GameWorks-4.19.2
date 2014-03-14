// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "Core.h"

/** How far the user has to drag the mouse before we consider the action dragging rather than a click */
const float SlateDragStartDistance = 5.0f;

/** How far the user has to drag the mouse before we consider the action panning rather than a click */
const float SlatePanTriggerDistance = 1.0f;

/** How much to scroll for each click of the mouse wheel (in Slate Screen Units). */
const float WheelScrollAmount = 32;


/** Is an entity visible? */
struct SLATE_API EVisibility
{
	/** Default widget visibility - visible and can interactive with the cursor */
	static const EVisibility Visible;
	/** Not visible and takes up no space in the layout; can never be clicked on because it takes up no space. */
	static const EVisibility Collapsed;
	/** Not visible, but occupies layout space. Not interactive for obvious reasons. */
	static const EVisibility Hidden;
	/** Visible to the user, but only as art. The cursors hit tests will never see this widget. */
	static const EVisibility HitTestInvisible;
	/** Same as HitTestInvisible, but doesn't apply to child widgets. */
	static const EVisibility SelfHitTestInvisible;
	/** Any visibility will do */
	static const EVisibility All;

	bool operator== (const EVisibility& Other) const
	{
		return this->Value == Other.Value;
	}

	bool operator!= (const EVisibility& Other) const
	{
		return this->Value != Other.Value;
	}

	bool AreChildrenHitTestVisible() const
	{
		return 0 != (Value & VISPRIVATE_ChildrenHitTestVisible);
	}

	bool IsHitTestVisible() const
	{
		return 0 != (Value & VISPRIVATE_SelfHitTestVisible);
	}

	bool IsVisible() const
	{
		return 0 != (Value & VIS_Visible);
	}

	FString ToString() const;

	EVisibility()
	: Value( VIS_Visible )
	{
	}

private:

	enum Private
	{
		/** Entity is visible */
		VISPRIVATE_Visible = 0x1 << 0,
		/** Entity is invisible and takes up no space */
		VISPRIVATE_Collapsed = 0x1 << 1,
		/** Entity is invisible, but still takes up space (layout pretends it is visible) */
		VISPRIVATE_Hidden = 0x1 << 2,
		/** Can we click on this widget or is it just non-interactive decoration? */
		VISPRIVATE_SelfHitTestVisible = 0x1 << 3,
		/** Can we click on this widget's child widgets? */
		VISPRIVATE_ChildrenHitTestVisible = 0x1 << 4,


		/** Default widget visibility - visible and can interactive with the cursor */
		VIS_Visible = VISPRIVATE_Visible | VISPRIVATE_SelfHitTestVisible | VISPRIVATE_ChildrenHitTestVisible,
		/** Not visible and takes up no space in the layout; can never be clicked on because it takes up no space. */
		VIS_Collapsed = VISPRIVATE_Collapsed,
		/** Not visible, but occupies layout space. Not interactive for obvious reasons. */
		VIS_Hidden = VISPRIVATE_Hidden,
		/** Visible to the user, but only as art. The cursors hit tests will never see this widget. */
		VIS_HitTestInvisible = VISPRIVATE_Visible,
		/** Same as HitTestInvisible, but doesn't apply to child widgets. */
		VIS_SelfHitTestInvisible = VISPRIVATE_Visible | VISPRIVATE_ChildrenHitTestVisible,


		/** Any visibility will do */
		VIS_All = VISPRIVATE_Visible | VISPRIVATE_Hidden | VISPRIVATE_Collapsed | VISPRIVATE_SelfHitTestVisible | VISPRIVATE_ChildrenHitTestVisible
	};

	EVisibility( Private InValue )
	: Value( InValue )
	{
		
	}

public:
	Private Value;
};

struct FOptionalSize 
{
	FOptionalSize()
	: Size( Unspecified )
	{
	}

	FOptionalSize( const float SpecifiedSize )
	: Size( SpecifiedSize )
	{
	}

	bool IsSet() const
	{
		return Size != Unspecified;
	}

	float Get() const
	{
		return Size;
	}

private:

	SLATE_API static const float Unspecified;
	float Size;
};

namespace EFocusMoveDirection
{
	enum Type
	{
		Next,
		Previous
	};
}

enum EScrollDirection
{
	Scroll_Down,
	Scroll_Up
};

/**
 * Describes a way in which a parent widget allocates available space to its child widgets.
 * When SizeRule is SizeRule_Auto, the widget's DesiredSize will be used as the space required.
 * When SizeRule is SizeRule_AspectRatio, the widget will attempt to maintain the specified aspect ratio.
 * When SizeRule is SizeRule_Stretch, the available space will be distributed proportionately between
 * peer Widgets depending on the Value property. Available space is space remaining after all the
 * peers' SizeRule_Auto requirements have been satisfied.
 *
 * FSizeParam cannot be constructed directly - see FStretch, FAuto, and FAspectRatio
 */
struct FSizeParam
{
	enum ESizeRule
	{
		SizeRule_Auto,
		SizeRule_Stretch,
		SizeRule_AspectRatio
	};
		
	ESizeRule SizeRule;

	/** The actual value this size parameter stores.  Can be driven by a delegate.  Only used for the Stretch mode */
	TAttribute< float > Value;
	
protected:
	FSizeParam( ESizeRule InTypeOfSize, const TAttribute< float >& InValue )
		: SizeRule( InTypeOfSize )
		, Value( InValue )
	{
	}
};


/** Construct a FSizeParam with SizeRule = SizeRule_Stretch */
struct FStretch : public FSizeParam
{
	FStretch( const TAttribute< float >& StretchAmount )
	: FSizeParam( SizeRule_Stretch, StretchAmount )
	{
	}

	FStretch()
	: FSizeParam( SizeRule_Stretch, 1.0f )
	{
	}
};

/** Construct a FSizeParam with SizeRule = SizeRule_Auto */
struct FAuto : public FSizeParam
{
	FAuto()
	: FSizeParam( SizeRule_Auto, 0 )
	{
	}
};

/** Construct a FSizeParam with SizeRule == SizeRule_AspectRatio */
struct FAspectRatio : public FSizeParam
{
	FAspectRatio()
	: FSizeParam( SizeRule_AspectRatio, 1.0f )
	{
	}
};


/** 
 * A rectangle defined by upper-left and lower-right corners
 */
class SLATE_API FSlateRect
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
	{

	}

	FSlateRect( const FVector2D& InStartPos, const FVector2D& InEndPos )
		: Left(InStartPos.X)
		, Top(InStartPos.Y)
		, Right(InEndPos.X)
		, Bottom(InEndPos.Y)
	{

	}
	
	bool IsValid() const
	{
		return Left >= 0 && Right >= 0 && Bottom >=0 && Top >= 0;
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

template <> struct TIsPODType<FSlateRect> { enum { Value = true }; };

class SWidget;
class FArrangedWidget;

/**
 * A Paint geometry is a draw-space rectangle.
 * The DrawPosition and DrawSize are already positioned and
 * scaled for immediate consumption by the draw code.
 *
 * The DrawScale is only applied to the internal aspects of the draw primitives. e.g. Line thickness, 3x3 grid margins, etc.
 */
class SLATE_API FPaintGeometry
{
public:
	FPaintGeometry( FVector2D InDrawPosition, FVector2D InDrawSize, float InDrawScale )
		: DrawPosition( InDrawPosition )
		, DrawSize ( InDrawSize )
		, DrawScale( InDrawScale )
	{
	}

	static const FPaintGeometry& Identity()
	{
		static FPaintGeometry IdentityPaintGeometry
		(
			FVector2D(0,0), // Position: Draw at the origin of draw space
			FVector2D(0,0), // Size    : Size is ignored by splines
			1.0f            // Scale   : Do not do any scaling.
		);

		return IdentityPaintGeometry;
	}

	FSlateRect ToSlateRect() const
	{
		return FSlateRect( DrawPosition.X, DrawPosition.Y, DrawPosition.X + DrawSize.X, DrawPosition.Y + DrawSize.Y );
	}

	/** Render-space position at which we will draw */
	FVector2D DrawPosition;

	/**
	 * The size in render-space that we want to make this element.
	 * This field is ignored by some elements (e.g. spline, lines)
	 */
	FVector2D DrawSize;

	/** Only affects the draw aspects of individual draw elements. e.g. line thickness, 3x3 grid margins */
	float DrawScale;
};

template <> struct TIsPODType<FPaintGeometry> { enum { Value = true }; };

/**
 * Represents the position, size, and absolute position of a Widget in Slate.
 * The absolute location of a geometry is usually screen space or
 * window space depending on where the geometry originated.
 * Geometries are usually paired with a SWidget pointer in order
 * to provide information about a specific widget (see FArrangedWidget).
 * A Geometry's parent is generally thought to be the Geometry of the
 * the corresponding parent widget.
 */
class SLATE_API FGeometry
{

public:
	FGeometry()
	: Position( 0,0 )
	, AbsolutePosition( 0,0 )
	, Size( 0,0 )
	, Scale( 1 )
	{
	}

	/**
	 * Construct a new geometry given the following parameters:
	 * 
	 * @param OffsetFromParent         Local position of this geometry within its parent geometry.
	 * @param ParentAbsolutePosition   The absolute position of the parent geometry containing this geometry.
	 * @param InSize                   The size of this geometry.
	 * @param InScale                  The scale of this geometry with respect to Normal Slate Coordinates.
	 */
	FGeometry(const FVector2D& OffsetFromParent, const FVector2D& ParentAbsolutePosition, const FVector2D& InSize, float InScale )
	: Position( OffsetFromParent )
	, AbsolutePosition( ParentAbsolutePosition + (OffsetFromParent * InScale) )
	, Size( InSize )	
	, Scale( InScale )
	{
	}
	
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
	FSlateRect GetRect() const
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
	FSlateRect GetClippingRect() const
	{
		return FSlateRect( AbsolutePosition.X, AbsolutePosition.Y, AbsolutePosition.X + Size.X * Scale, AbsolutePosition.Y + Size.Y * Scale );
	}
	
	/** @return A String representation of this Geometry */
	FString ToString() const
	{
		return FString::Printf(TEXT("[Pos=%s, Abs=%s, Size=%s]"), *Position.ToString(), *AbsolutePosition.ToString(), *Size.ToString() );
	}

	/** @return the size of the geometry in screen space */
	FVector2D GetDrawSize() const
	{
		return Size * Scale;
	}

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

/**
 * A pair: Widget and its Geometry. Widgets populate an list of WidgetGeometries
 * when they arrange their children. See SWidget::ArrangeChildren.
 */
class FArrangedWidget
{
public:
	FArrangedWidget( TSharedRef<SWidget> InWidget, const FGeometry& InGeometry )
		: Geometry( InGeometry )
		, Widget( InWidget )
	{
	}

	bool operator==( const FArrangedWidget& Other ) const 
	{
		return Widget == Other.Widget;
	}

	FGeometry Geometry;
	TSharedRef<SWidget> Widget;
	
	
	/** @return a String representation of the Widget and corresponding Geometry*/
	FString ToString() const;
};


/**
 * Enumeration for the different methods that a button click can be triggered.  Normally, DownAndUp is appropriate.
 */
namespace EButtonClickMethod
{
	enum Type
	{
		/** User must press the button, then release while over the button to trigger the click.  This is the most common type of button. */
		DownAndUp,

		/** Click will be triggered immediately on mouse down, and mouse will not be captured */
		MouseDown,

		/** Click will always be triggered when mouse button is released over the button, even if the button wasn't pressed down over it */
		MouseUp,
	};
}

/**
 * A convenient representation of a marquee selection
 */
struct FMarqueeRect
{
	/** Where the user began the marquee selection */
	FVector2D StartPoint;
	/** Where the user has dragged to so far */
	FVector2D EndPoint;

	/** Make a default marquee selection */
	FMarqueeRect( FVector2D InStartPoint = FVector2D::ZeroVector )
		: StartPoint( InStartPoint )
		, EndPoint( InStartPoint )
	{
	}

	/**
	 * Update the location to which the user has dragged the marquee selection so far
	 *
	 * @param NewEndPoint   Where the user has dragged so far.
	 */
	void UpdateEndPoint( FVector2D NewEndPoint )
	{
		EndPoint = NewEndPoint;
	}

	/** @return true if this marquee selection is not too small to be considered real */
	bool IsValid() const
	{
		return ! (EndPoint - StartPoint).IsNearlyZero();
	}

	/** @return the upper left point of the selection */
	FVector2D GetUpperLeft() const
	{
		return FVector2D( FMath::Min(StartPoint.X, EndPoint.X), FMath::Min( StartPoint.Y, EndPoint.Y ) );
	}

	/** @return the lower right of the selection */
	FVector2D GetLowerRight() const
	{
		return  FVector2D( FMath::Max(StartPoint.X, EndPoint.X), FMath::Max( StartPoint.Y, EndPoint.Y ) );
	}

	/** The size of the selection */
	FVector2D GetSize() const
	{
		FVector2D SignedSize = EndPoint - StartPoint;
		return FVector2D( FMath::Abs(SignedSize.X), FMath::Abs(SignedSize.Y) );
	}

	/** @return This marquee rectangle as a well-formed SlateRect */
	FSlateRect ToSlateRect() const
	{
		return FSlateRect( FMath::Min(StartPoint.X, EndPoint.X), FMath::Min(StartPoint.Y, EndPoint.Y), FMath::Max(StartPoint.X, EndPoint.X), FMath::Max( StartPoint.Y, EndPoint.Y ) );
	}
};


/**
 * The results of an ArrangeChildren are always returned as an FArrangedChildren.
 * FArrangedChildren supports a filter that is useful for excluding widgets with unwanted
 * visibilities.
 */
class SLATE_API FArrangedChildren
{
	private:
	
	EVisibility VisibilityFilter;
	
	public:

	/**
	 * Construct a new container for arranged children that only accepts children that match the VisibilityFilter.
	 * e.g.
	 *  FArrangedChildren ArrangedChildren( VIS_All ); // Children will be included regardless of visibility
	 *  FArrangedChildren ArrangedChildren( EVisibility::Visible ); // Only visible children will be included
	 *  FArrangedChildren ArrangedChildren( EVisibility::Collapsed | EVisibility::Hidden ); // Only hidden and collapsed children will be included.
	 */
	FArrangedChildren( EVisibility InVisibilityFilter )
	: VisibilityFilter( InVisibilityFilter )
	{
	}

	/** Reverse the order of the arranged children */
	void Reverse()
	{
		int32 LastElementIndex = Array.Num() - 1;
		for (int32 WidgetIndex = 0; WidgetIndex < Array.Num()/2; ++WidgetIndex )
		{
			Array.Swap( WidgetIndex, LastElementIndex - WidgetIndex );
		}
	}

	/**
	 * Add an arranged widget (i.e. widget and its resulting geometry) to the list of Arranged children.
	 *
	 * @param VisibilityOverride   The arrange function may override the visibility of the widget for the purposes
	 *                             of layout or performance (i.e. prevent redundant call to Widget->GetVisibility())
	 * @param InWidgetGeometry     The arranged widget (i.e. widget and its geometry)
	 */
	void AddWidget( EVisibility VisibilityOverride, const FArrangedWidget& InWidgetGeometry );
	
	/**
	 * Add an arranged widget (i.e. widget and its resulting geometry) to the list of Arranged children
	 * based on the the visibility filter and the arranged widget's visibility
	 */
	void AddWidget( const FArrangedWidget& InWidgetGeometry );

	EVisibility GetFilter() const
	{
		return VisibilityFilter;
	}

	bool Accepts( EVisibility InVisibility ) const
	{
		return 0 != (InVisibility.Value & VisibilityFilter.Value);
	}

	// We duplicate parts of TArray's interface here!
	// Inheriting from "public TArray<FArrangedWidget>"
	// saves us this boilerplate, but causes instantiation of
	// many template combinations that we do not want.

	private:
	/** Internal representation of the array widgets */
	TArray<FArrangedWidget> Array;

	public:

	TArray<FArrangedWidget>& GetInternalArray()
	{
		return Array;
	}

	const TArray<FArrangedWidget>& GetInternalArray() const
	{
		return Array;
	}

	int32 Num() const
	{
		return Array.Num();
	}

	const FArrangedWidget& operator()(int32 Index) const
	{
		return Array[Index];
	}

	FArrangedWidget& operator()(int32 Index)
	{
		return Array[Index];
	}

	const FArrangedWidget& Last() const
	{
		return Array.Last();
	}

	FArrangedWidget& Last()
	{
		return Array.Last();
	}

	int32 FindItemIndex(const FArrangedWidget& ItemToFind ) const
	{
		return Array.Find(ItemToFind);
	}

	void Remove( int32 Index, int32 Count=1 )
	{
		Array.RemoveAt(Index, Count);
	}

	void Append( const FArrangedChildren& Source )
	{
		Array.Append( Source.Array );
	}

	void Empty()
	{
		Array.Empty();
	}

};

/**
 * When we have an optional value IsSet() returns true, and GetValue() is meaningful.
 * Otherwise GetValue() is not meaningful.
 */
template<typename OptionalType>
struct TOptional
{
public:
	/** Construct an OptionaType with a valid value. */
	TOptional( OptionalType InValue )
		: bIsSet( true )
		, Value( InValue )
	{
	}

	/** Construct an OptionalType with no value; i.e. NULL */
	TOptional()
		: bIsSet( false )
		, Value()
	{
	}

	/** @return true when the value is meaningulf; false if calling GetValue() is undefined. */
	bool IsSet() const { return bIsSet; }

	/** @return The optional value; undefined when IsSet() returns false. */
	const OptionalType& GetValue() const { return Value; }

	/** @return The optional value when set; DefaultValue otherwise. */
	const OptionalType& Get( const OptionalType& DefaultValue ) const { return IsSet() ? Value : DefaultValue; }

private:
	bool bIsSet;
	OptionalType Value;
};