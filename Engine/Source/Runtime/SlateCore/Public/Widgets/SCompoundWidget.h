// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SCompoundWidget.h: Declares the SCompoundWidget class.
=============================================================================*/

#pragma once


/**
 * Implements a compound widget.
 *
 * A CompoundWidget is the base from which most non-primitive widgets should be built.
 * CompoundWidgets have a protected member named ChildSlot.
 */
class SLATECORE_API SCompoundWidget
	: public SWidget
{
public:

	/**
	 * Returns the size scaling factor for this widget.
	 *
	 * @return Size scaling factor.
	 */
	const FVector2D GetContentScale() const
	{
		return ContentScale.Get();
	}

	/**
	 * Sets the content scale for this widget.
	 *
	 * @param InContentScale Content scaling factor.
	 */
	void SetContentScale( const TAttribute< FVector2D >& InContentScale )
	{
		ContentScale = InContentScale;
	}

	/**
	 * Gets the widget's color.
	 */
	FLinearColor GetColorAndOpacity() const
	{
		return ColorAndOpacity.Get();
	}

	/**
	 * Sets the widget's color.
	 *
	 * @param InColorAndOpacity The ColorAndOpacity to be applied to this widget and all its contents.
	 */
	void SetColorAndOpacity( const TAttribute<FLinearColor>& InColorAndOpacity )
	{
		ColorAndOpacity = InColorAndOpacity;
	}

	/**
	 * Sets the widget's foreground color.
	 *
	 * @param InColor The color to set.
	 */
	void SetForegroundColor( const TAttribute<FSlateColor>& InForegroundColor )
	{
		ForegroundColor = InForegroundColor;
	}

public:

	// Begin SWidgetOverrides

	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;
	
	virtual FChildren* GetChildren() OVERRIDE;

	virtual FVector2D ComputeDesiredSize() const OVERRIDE;

	virtual void ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const OVERRIDE;

	virtual FSlateColor GetForegroundColor() const OVERRIDE;

	// Begin SWidgetOverrides

protected:

	/**
	 * Hidden constructor.
	 */
	FORCENOINLINE SCompoundWidget()
		: ContentScale(FVector2D(1.0f, 1.0f))
		, ColorAndOpacity(FLinearColor::White)
		, ForegroundColor(FSlateColor::UseForeground())
	{ }

protected:

	/** The slot that contains this widget's descendants.*/
	FSimpleSlot ChildSlot;

	/** The layout scale to apply to this widget's contents; useful for animation. */
	TAttribute<FVector2D> ContentScale;

	/** The color and opacity to apply to this widget and all its descendants. */
	TAttribute<FLinearColor> ColorAndOpacity;

	/** Optional foreground color that will be inherited by all of this widget's contents */
	TAttribute<FSlateColor> ForegroundColor;
};
