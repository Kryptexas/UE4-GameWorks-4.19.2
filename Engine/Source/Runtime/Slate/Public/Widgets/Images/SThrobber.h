// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/** A Throbber widget that uses 5 zooming circles in a row.*/
class SLATE_API SThrobber : public SCompoundWidget
{

static const int32 DefaultNumPieces = 3;

public:
	enum EAnimation
	{
		Vertical = 0x1 << 0,
		Horizontal = 0x1 << 1,
		Opacity = 0x1 << 2,
		VerticalAndOpacity = Vertical | Opacity,
		All = Vertical | Horizontal | Opacity,
		None = 0x0
	};

	SLATE_BEGIN_ARGS(SThrobber)	
		: _PieceImage( FCoreStyle::Get().GetBrush( "Throbber.Chunk" ) )
		, _NumPieces( DefaultNumPieces )
		, _Animate( SThrobber::All )
		{}

		/** What each segment of the throbber looks like */
		SLATE_ARGUMENT( const FSlateBrush*, PieceImage )
		/** How many pieces there are */
		SLATE_ARGUMENT( int32, NumPieces )
		/** Which aspects of the throbber to animate */
		SLATE_ARGUMENT( EAnimation, Animate )
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);


private:
	FVector2D GetPieceScale(int32 PieceIndex) const;
	FLinearColor GetPieceColor(int32 PieceIndex) const;
	
	FCurveSequence AnimCurves;
	TArray<FCurveHandle, TInlineAllocator<DefaultNumPieces> > ThrobberCurve;
	EAnimation Animate;
};

/** A throbber widget that orients images in a spinning circle. */
class SLATE_API SCircularThrobber : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCircularThrobber)	
		: _PieceImage( FCoreStyle::Get().GetBrush( "Throbber.CircleChunk" ) )
		, _NumPieces( 6 )
		, _Period( 0.75f )
		, _Radius( 16.f )
		{}

		/** What each segment of the throbber looks like */
		SLATE_ARGUMENT( const FSlateBrush*, PieceImage )
		/** How many pieces there are */
		SLATE_ARGUMENT( int32, NumPieces )
		/** The amount of time in seconds for a full circle */
		SLATE_ARGUMENT( float, Period )
		/** The radius of the circle */
		SLATE_ARGUMENT( float, Radius )

	SLATE_END_ARGS()

	/** Constructs the widget */
	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;
	virtual FVector2D ComputeDesiredSize() const OVERRIDE;

private:
	/** The sequence to drive the spinning animation */
	FCurveSequence Sequence;
	FCurveHandle Curve;

	/** What each segment of the throbber looks like */
	const FSlateBrush* PieceImage;
	/** How many pieces there are */
	int32 NumPieces;
	/** The radius of the circle */
	float Radius;
};
