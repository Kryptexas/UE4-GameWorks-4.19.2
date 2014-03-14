// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


class STimeSlider : public ITimeSlider
{
public:

	SLATE_BEGIN_ARGS(STimeSlider)
		: _MirrorLabels( false )
	{}
		/* If we should mirror the labels on the timeline */
		SLATE_ARGUMENT( bool, MirrorLabels )
	SLATE_END_ARGS()


	/**
	 * Construct the widget
	 * 
	 * @param InArgs   A declaration from which to construct the widget
	 */
	void Construct( const FArguments& InArgs, TSharedRef<ITimeSliderController> InTimeSliderController );

protected:
	// SWidget interface
	virtual FVector2D ComputeDesiredSize() const OVERRIDE;
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
private:
	TSharedPtr<ITimeSliderController> TimeSliderController;
	bool bMirrorLabels;
};

