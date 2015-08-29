// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class USequencerSettings;

/** Enum specifying how to interpolate to a new view range */
enum class EViewRangeInterpolation
{
	/** Use an externally defined animated interpolation */
	Animated,
	/** Set the view range immediately */
	Immediate,
};

DECLARE_DELEGATE_TwoParams( FOnScrubPositionChanged, float, bool )
DECLARE_DELEGATE_TwoParams( FOnViewRangeChanged, TRange<float>, EViewRangeInterpolation )

/** Structure used to wrap up a range, and an optional animation target */
struct FAnimatedRange : public TRange<float>
{
	/** Default Construction */
	FAnimatedRange() : TRange() {}
	/** Construction from a lower and upper bound */
	FAnimatedRange( float LowerBound, float UpperBound ) : TRange( LowerBound, UpperBound ) {}
	/** Copy-construction from simple range */
	FAnimatedRange( const TRange<float>& InRange ) : TRange(InRange) {}

	/** Helper function to wrap an attribute to an animated range with a non-animated one */
	static TAttribute<TRange<float>> WrapAttribute( const TAttribute<FAnimatedRange>& InAttribute )
	{
		typedef TAttribute<TRange<float>> Attr;
		return Attr::Create(Attr::FGetter::CreateLambda([=](){ return InAttribute.Get(); }));
	}

	/** Helper function to wrap an attribute to a non-animated range with an animated one */
	static TAttribute<FAnimatedRange> WrapAttribute( const TAttribute<TRange<float>>& InAttribute )
	{
		typedef TAttribute<FAnimatedRange> Attr;
		return Attr::Create(Attr::FGetter::CreateLambda([=](){ return InAttribute.Get(); }));
	}

	/** Get the current animation target, or the whole view range when not animating */
	const TRange<float>& GetAnimationTarget() const
	{
		return AnimationTarget.IsSet() ? AnimationTarget.GetValue() : *this;
	}
	
	/** The animation target, if animating */
	TOptional<TRange<float>> AnimationTarget;
};

struct FTimeSliderArgs
{
	FTimeSliderArgs()
		: ScrubPosition(0)
		, ViewRange( FAnimatedRange(0.0f, 5.0f) )
		, ClampMin(-70000.0f)
		, ClampMax(70000.0f)
		, OnScrubPositionChanged()
		, OnBeginScrubberMovement()
		, OnEndScrubberMovement()
		, OnViewRangeChanged()
		, AllowZoom(true)
		, Settings(nullptr)
	{}

	/** The scrub position */
	TAttribute<float> ScrubPosition;
	/** View time range */
	TAttribute< FAnimatedRange > ViewRange;
	/** Optional min output to clamp to */
	TAttribute<TOptional<float> > ClampMin;
	/** Optional max output to clamp to */
	TAttribute<TOptional<float> > ClampMax;
	/** Called when the scrub position changes */
	FOnScrubPositionChanged OnScrubPositionChanged;
	/** Called right before the scrubber begins to move */
	FSimpleDelegate OnBeginScrubberMovement;
	/** Called right after the scrubber handle is released by the user */
	FSimpleDelegate OnEndScrubberMovement;
	/** Called when the view range changes */
	FOnViewRangeChanged OnViewRangeChanged;
	/** If we are allowed to zoom */
	bool AllowZoom;
	/** User-supplied settings object */
	USequencerSettings* Settings;
};

class ITimeSliderController
{
public:
	virtual ~ITimeSliderController(){}
	virtual int32 OnPaintTimeSlider( bool bMirrorLabels, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const = 0;
	virtual FReply OnMouseButtonDown( TSharedRef<SWidget> WidgetOwner, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) = 0;
	virtual FReply OnMouseButtonUp( TSharedRef<SWidget> WidgetOwner, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) = 0;
	virtual FReply OnMouseMove( TSharedRef<SWidget> WidgetOwner, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) = 0;
	virtual FReply OnMouseWheel( TSharedRef<SWidget> WidgetOwner, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) = 0;
};

/**
 * Base class for a widget that scrubs time or frames
 */
class ITimeSlider : public SCompoundWidget
{
public:
};