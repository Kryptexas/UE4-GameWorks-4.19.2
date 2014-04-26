// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SColorSpectrum.h: Declares the SColorSpectrum class.
=============================================================================*/

#pragma once


/**
 * Implements the color spectrum widget.
 */
class SColorSpectrum
	: public SLeafWidget
{
public:

	SLATE_BEGIN_ARGS(SColorSpectrum)
		: _SelectedColor()
		, _OnMouseCaptureBegin()
		, _OnMouseCaptureEnd()
		, _OnValueChanged()
	{ }
	
		/** The current color selected by the user */
		SLATE_ATTRIBUTE(FLinearColor, SelectedColor)
		
		/** Invoked when the mouse is pressed and a capture begins */
		SLATE_EVENT(FSimpleDelegate, OnMouseCaptureBegin)

		/** Invoked when the mouse is released and a capture ends */
		SLATE_EVENT(FSimpleDelegate, OnMouseCaptureEnd)

		/** Invoked when a new value is selected on the color wheel */
		SLATE_EVENT(FOnLinearColorValueChanged, OnValueChanged)

	SLATE_END_ARGS()
	
public:

	/**
	 * Construct this widget.
	 *
	 * @param InArgs - The declaration data for this widget.
	 */
	void Construct( const FArguments& InArgs );

public:

	// Begin overrides

	virtual FVector2D ComputeDesiredSize( ) const;

	virtual FReply OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
	{
		return FReply::Handled();
	}

	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent );
	
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent );

	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent );

	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;

	// End overrides
	
protected:

	/**
	 * Calculates the position of the color selection indicator.
	 *
	 * @return The position relative to the widget.
	 */
	FVector2D CalcRelativeSelectedPosition( ) const;

private:

	// The color wheel image to show.
	const FSlateBrush* Image;
	
	// The current color selected by the user.
	TAttribute< FLinearColor > SelectedColor;

	// The color selector image to show.
	const FSlateBrush* SelectorImage;

private:

	// Holds a delegate that is executed when the mouse is pressed and a capture begins.
	FSimpleDelegate OnMouseCaptureBegin;

	// Holds a delegate that is executed when the mouse is let up and a capture ends.
	FSimpleDelegate OnMouseCaptureEnd;

	// Holds a delegate that is executed when a new value is selected on the color wheel.
	FOnLinearColorValueChanged OnValueChanged;
};
