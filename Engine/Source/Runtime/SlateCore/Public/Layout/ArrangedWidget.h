// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ArrangedWidget.h: Declares the FArrangedWidget class.
=============================================================================*/

#pragma once


/**
 * A pair: Widget and its Geometry. Widgets populate an list of WidgetGeometries
 * when they arrange their children. See SWidget::ArrangeChildren.
 */
class FArrangedWidget
{
public:

	FArrangedWidget( TSharedRef<SWidget> InWidget, const FGeometry& InGeometry )
		: Geometry(InGeometry)
		, Widget(InWidget)
	{ }

public:

	/**
	 * Gets the string representation of the Widget and corresponding Geometry.
	 *
	 * @return String representation.
	 */
	FString ToString( ) const;

public:

	/**
	 * Compares this widget arrangement with another for equality.
	 *
	 * @param Other The other arrangement to compare with.
	 *
	 * @return true if the two arrangements are equal, false otherwise.
	 */
	bool operator==( const FArrangedWidget& Other ) const 
	{
		return Widget == Other.Widget;
	}

public:

	/** The widget's geometry. */
	FGeometry Geometry;

	/** The widget that is being arranged. */
	TSharedRef<SWidget> Widget;
};
