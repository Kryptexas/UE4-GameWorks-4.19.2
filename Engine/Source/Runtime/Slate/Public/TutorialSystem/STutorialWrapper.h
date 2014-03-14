// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Delegate for a named widget being highlighted */
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnWidgetHighlight, const FName&);

class STutorialWrapper : public SBorder
{
public:
	SLATE_BEGIN_ARGS( STutorialWrapper )
		: _Content()
		, _Name()
	{
		_Visibility = EVisibility::SelfHitTestInvisible;
	}

	/** Slot for the wrapped content (optional) */
	SLATE_DEFAULT_SLOT( FArguments, Content )

	/** The name of the widget */
	SLATE_ARGUMENT( FName, Name )

	SLATE_END_ARGS()

	SLATE_API void Construct( const FArguments& InArgs );

	/** Begin SWidget interface */
	SLATE_API virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;
	SLATE_API virtual int32 OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;
	/** End SWidget interface */

	/** 
	 * Get the delegate fired when a named widget wants to draw its highlight
	 * @returns a reference to the delegate
	 */
	SLATE_API static FOnWidgetHighlight& OnWidgetHighlight();

private:

	/** Get the values that the animation drives */
	void GetAnimationValues(float& OutAlphaFactor, float& OutPulseFactor, FLinearColor& OutShadowTint, FLinearColor& OutBorderTint) const;

private:

	/** The name of the widget */
	FName Name;

	/** Flag for whether we are playing animation or not */
	bool bIsPlaying;

	/** Animation curves for displaying border */
	FCurveSequence BorderPulseAnimation;
	FCurveSequence BorderIntroAnimation;

	/** Geometry cached from Tick() */
	FGeometry CachedGeometry;

	/** The delegate fired when a named widget wants to draw its highlight */
	static FOnWidgetHighlight OnWidgetHighlightDelegate;
};