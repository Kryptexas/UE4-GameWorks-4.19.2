// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * A Slate slider control is a linear scale and draggable handle.
 */
class SLATE_API SSlider
	: public SLeafWidget
{
public:

	SLATE_BEGIN_ARGS(SSlider)
		: _IndentHandle(true)
		, _Locked(false)
		, _Orientation(EOrientation::Orient_Horizontal)
		, _SliderBarColor(FLinearColor::White)
		, _SliderHandleColor(FLinearColor::White)
		, _Style(&FCoreStyle::Get().GetWidgetStyle<FSliderStyle>("Slider"))
		, _Value(1.f)
		, _OnMouseCaptureBegin()
		, _OnMouseCaptureEnd()
		, _OnValueChanged()
		{ }

		/** Whether the slidable area should be indented to fit the handle. */
		SLATE_ATTRIBUTE( bool, IndentHandle )

		/** Whether the handle is interactive or fixed. */
		SLATE_ATTRIBUTE( bool, Locked )

		/** The slider's orientation. */
		SLATE_ARGUMENT( EOrientation, Orientation)

		/** The color to draw the slider bar in. */
		SLATE_ATTRIBUTE( FSlateColor, SliderBarColor )

		/** The color to draw the slider handle in. */
		SLATE_ATTRIBUTE( FSlateColor, SliderHandleColor )

		/** The style used to draw the slider. */
		SLATE_STYLE_ARGUMENT( FSliderStyle, Style )

		/** The volume value to display. */
		SLATE_ATTRIBUTE( float, Value )

		/** Invoked when the mouse is pressed and a capture begins. */
		SLATE_EVENT(FSimpleDelegate, OnMouseCaptureBegin)

		/** Invoked when the mouse is released and a capture ends. */
		SLATE_EVENT(FSimpleDelegate, OnMouseCaptureEnd)


		/** Called when the value is changed by slider or typing. */
		SLATE_EVENT( FOnFloatValueChanged, OnValueChanged )

	SLATE_END_ARGS()

	/**
	 * Construct the widget.
	 * 
	 * @param InDeclaration - A declaration from which to construct the widget.
	 */
	void Construct( const SSlider::FArguments& InDeclaration );

	/**
	 * Gets the slider's value.
	 *
	 * @return Slider value.
	 */
	float GetValue() const
	{
		return ValueAttribute.Get();
	}

public:

	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;

	virtual FVector2D ComputeDesiredSize() const;

	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	
	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;

protected:

	/**
	 * Commits the specified slider value.
	 *
	 * @param NewValue - The value to commit.
	 */
	void CommitValue(float NewValue);

	/**
	 * Calculates the new value based on the given absolute coordinates.
	 *
	 * @param MyGeometry - The slider's geometry.
	 * @param AbsolutePosition - The absolute position of the slider.
	 *
	 * @return The new value.
	 */
	float PositionToValue( const FGeometry& MyGeometry, const FVector2D& AbsolutePosition );

private:

	// Holds the image to use when the slider handle is in its disabled state.
	const FSlateBrush* DisabledHandleImage;

	// Holds the image to use when the slider handle is in its normal state.
	const FSlateBrush* NormalHandleImage;

	// Holds a flag indicating whether the slideable area should be indented to fit the handle.
	TAttribute<bool> IndentHandle;

	// Holds a flag indicating whether the slider is locked.
	TAttribute<bool> LockedAttribute;

	// Holds the slider's orientation.
	EOrientation Orientation;

	// Holds the color of the slider bar.
	TAttribute<FSlateColor> SliderBarColor;

	// Holds the color of the slider handle.
	TAttribute<FSlateColor> SliderHandleColor;

	// Holds the slider's current value.
	TAttribute<float> ValueAttribute;

private:

	// Holds a delegate that is executed when the mouse is pressed and a capture begins.
	FSimpleDelegate OnMouseCaptureBegin;

	// Holds a delegate that is executed when the mouse is let up and a capture ends.
	FSimpleDelegate OnMouseCaptureEnd;

	// Holds a delegate that is executed when the slider's value changed.
	FOnFloatValueChanged OnValueChanged;
};
