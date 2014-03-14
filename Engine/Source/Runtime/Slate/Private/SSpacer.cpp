// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "SSpacer.h"



/**
 * Construct this widget
 *
 * @param	InArgs	The declaration data for this widget
 */
void SSpacer::Construct( const FArguments& InArgs )
{
	SpacerSize = InArgs._Size;

	SWidget::Construct( 
		InArgs._ToolTipText,
		InArgs._ToolTip,
		InArgs._Cursor, 
		InArgs._IsEnabled,
		InArgs._Visibility,
		InArgs._Tag);
}


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
int32 SSpacer::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	// We did not paint anything. The parent's current LayerId is probably the max we were looking for.
	return LayerId;
}

FVector2D SSpacer::ComputeDesiredSize() const
{
	return SpacerSize.Get();
}
