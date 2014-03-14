// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * A LeafWidget is a Widget that has no slots for children.
 * LeafWidgets are usually intended as building blocks for aggregate
 * widgets.
 */
class SLATE_API SLeafWidget : public SWidget
{

public:
	// INTERFACE TO IMPLEMENT
	
	/**
	 * LeafWidgets provide a visual representation of themselves. They do so by adding DrawElement(s) to the OutDrawElements.
	 * DrawElements should have their positions set to absolute coordinates in Window space; for this purpose the
	 * Slate system provides the AllottedGeometry parameter. AllottedGeometry describes the space allocated for the visualization
	 * of this widget.
	 * Whenever possible, LeafWidgets should avoid dealing with layout properties. See TextBlock for an example.
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
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const = 0;
	
	/**
	 * LeafWidgets should compute their DesiredSize based solely on their visual representation. There is no need to
	 * take child widgets into account as LeafWidgets have none by definition.
	 * For example, the TextBlock widget simply measures the area necessary to display its text with the given font
	 * and font size.
	 */
	virtual FVector2D ComputeDesiredSize() const = 0;
	
public:
	/** @return NoChildren; leaf widgets never have children */
	virtual FChildren* GetChildren();

	/**
	 * Compute the Geometry of all the children and add populate the ArrangedChildren list with their values.
	 * Each type of Layout panel should arrange children based on desired behavior.
	 */
	virtual void ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const;

private:
	/** Shared instance of FNoChildren for all widgets with no children. */
	static FNoChildren NoChildrenInstance;

};
