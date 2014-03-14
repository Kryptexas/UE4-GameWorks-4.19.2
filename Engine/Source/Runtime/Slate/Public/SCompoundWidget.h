// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
 
#pragma once

/**
 * A CompoundWidget is the base from which most non-primitive widgets should be built.
 * CompoundWidgets have a protected member named ChildSlot.
 */
class SLATE_API SCompoundWidget : public SWidget
{
public:
	// INTERFACE TO IMPLEMENT

	/**
	 * The widget should respond by populating the OutDrawElements array with FDrawElements 
	 * that represent it and any of its children.
	 *
	 * @param AllottedGeometry  The FGeometry that describes an area in which the widget should appear.
	 * @param MyClippingRect    The clipping rectangle allocated for this widget and its children.
	 * @param OutDrawElements   A list of FDrawElements to populate with the output.
	 * @param LayerId           The Layer onto which this widget should be rendered.
	 * @param InColorAndOpacity Color and Opacity to be applied to all the descendants of the widget being painted
 	 * @param bParentEnabled	True if the parent of this widget is enabled.
	 *
	 * @return The maximum layer ID attained by this widget or any of its children.
	 */
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;
	
	virtual FChildren* GetChildren() OVERRIDE;

	virtual FVector2D ComputeDesiredSize() const OVERRIDE;
	virtual void ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const OVERRIDE;
	
public:
	/**
	 * Returns the size scaling factor for this widget
	 *
	 * @return	Size scaling factor
	 */
	const FVector2D GetContentScale() const
	{
		return ContentScale.Get();
	}

	/**
	 * Sets the content scale for this widget
	 *
	 * @param	InContentScale	Content scaling factor
	 */
	void SetContentScale( const TAttribute< FVector2D >& InContentScale )
	{
		ContentScale = InContentScale;
	}

	/** @param InColorAndOpacity   The ColorAndOpacity to be applied to this widget and all its contents */
	void SetColorAndOpacity( const TAttribute<FLinearColor>& InColorAndOpacity )
	{
		ColorAndOpacity = InColorAndOpacity;
	}

	/** Set the foreground color for this compound widget at runtime */
	void SetForegroundColor( const TAttribute<FSlateColor>& InColor )
	{
		ForegroundColor = InColor;
	}

	virtual FSlateColor GetForegroundColor() const OVERRIDE;

protected:

	/** Disallow public construction */
	FORCENOINLINE SCompoundWidget()
		: ContentScale( FVector2D(1,1) )
		, ColorAndOpacity( FLinearColor::White )
		, ForegroundColor( FSlateColor::UseForeground() )
	{
	}

	/** The slot that contains this widget's descendants.*/
	FSimpleSlot ChildSlot;

	/** The layout scale to apply to this widget's contents; useful for animation. */
	TAttribute<FVector2D> ContentScale;

	/** The color and opacity to apply to this widget and all its descendants. */
	TAttribute<FLinearColor> ColorAndOpacity;

	/** Optional foreground color that will be inherited by all of this widget's contents */
	TAttribute<FSlateColor> ForegroundColor;
};
