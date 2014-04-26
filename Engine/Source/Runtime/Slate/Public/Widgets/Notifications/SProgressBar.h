// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** A progress bar widget.*/
class SLATE_API SProgressBar : public SLeafWidget
{

public:
	SLATE_BEGIN_ARGS(SProgressBar)
		: _Style( &FCoreStyle::Get().GetWidgetStyle<FProgressBarStyle>("ProgressBar") )
		, _Percent( TOptional<float>() )
		, _FillColorAndOpacity( FLinearColor::White )
		, _BorderPadding( FVector2D(1,0) )
		{}

		/** Style used for the progress bar */
		SLATE_STYLE_ARGUMENT( FProgressBarStyle, Style )

		/** Used to determine the fill position of the progress bar ranging 0..1 */
		SLATE_ATTRIBUTE( TOptional<float>, Percent )

		/** Fill Color and Opacity */
		SLATE_ATTRIBUTE( FSlateColor, FillColorAndOpacity )

		/** Border Padding around fill bar */
		SLATE_ATTRIBUTE( FVector2D, BorderPadding )

	SLATE_END_ARGS()

	/**
	 * Construct the widget
	 * 
	 * @param InArgs   A declaration from which to construct the widget
	 */
	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;

	FVector2D ComputeDesiredSize() const;

private:

	/** The text displayed over the progress bar */
	TAttribute< TOptional<float> > Percent;

	/** Background image to use for the progress bar */
	const FSlateBrush* BackgroundImage;

	/** Foreground image to use for the progress bar */
	const FSlateBrush* FillImage;

	/** Image to use for marquee mode */
	const FSlateBrush* MarqueeImage;

	/** Fill Color and Opacity */
	TAttribute<FSlateColor> FillColorAndOpacity;

	/** Border Padding */
	TAttribute<FVector2D> BorderPadding;

	/** Curve sequence to drive progress bar animation */
	FCurveSequence CurveSequence;

};

