// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/** SImage displays a single image at a desired Width and Height. */
class SLATE_API SImage : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS( SImage )
		: _Image( FCoreStyle::Get().GetDefaultBrush() )
		, _ColorAndOpacity( FLinearColor::White )
		, _OnMouseButtonDown()
		{}

		/** Image resource */
		SLATE_ATTRIBUTE( const FSlateBrush*, Image )

		/** Color and opacity */
		SLATE_ATTRIBUTE( FSlateColor, ColorAndOpacity )

		/** Invoked when the mouse is pressed in the widget. */
		SLATE_EVENT( FPointerEventHandler, OnMouseButtonDown )

	SLATE_END_ARGS()

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs );

	// SWidget interface
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FVector2D ComputeDesiredSize() const OVERRIDE;
	// End of SWidget interface

public:

	/** The the color and opacity of this image */
	void SetColorAndOpacity( const TAttribute<FSlateColor>& InColorAndOpacity );
	/** The the color and opacity of this image */
	void SetColorAndOpacity( FLinearColor InColorAndOpacity );


protected:

	/** The FName of the image resource to show */
	TAttribute< const FSlateBrush* > Image;

	/** Color and opacity scale for this image */
	TAttribute<FSlateColor> ColorAndOpacity;

	/** Invoked when the mouse is pressed in the image */
	FPointerEventHandler OnMouseButtonDownHandler;

};
