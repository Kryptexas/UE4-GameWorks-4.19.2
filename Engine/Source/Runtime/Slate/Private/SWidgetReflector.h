// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** The widget reflector interface */
class SLATE_API SWidgetReflector : public SCompoundWidget
{
public:

	/** Called when the user has picked a widget to observe. */
	virtual void OnWidgetPicked() = 0;

	/** @return true if we are visualizing the focused widgets */
	virtual bool IsShowingFocus() const = 0;

	/** @return True if the user is in the process of selecting a widget. */
	virtual bool IsInPickingMode() const = 0;

	/** @return True if we should be inspecting widgets and visualizing their layout */
	virtual bool IsVisualizingLayoutUnderCursor() const = 0;

	/**
	 * Take a snapshot of the UI pertaining to the widget that the user is hovering and visualize it.
	 * If we are not taking a snapshot, draw the overlay from a previous snapshot, if possible.
	 *
	 * @param InWidgetsToVisualize  WidgetPath that the cursor is currently over; could be null.
	 * @param OutDrawElements       List of draw elements to which we will add a visualization overlay
	 * @param LayerId               The maximum layer id attained in the draw element list so far.
	 *
	 * @return The maximum layer ID that we attained while painting the overlay.
	 */
	virtual int32 Visualize( const FWidgetPath& InWidgetsToVisualize, FSlateWindowElementList& OutDrawElements, int32 LayerId ) = 0;

	/** @param InWidgetsToVisualize  A Widget Path to inspect via the reflector */
	virtual void SetWidgetsToVisualize( const FWidgetPath& InWidgetsToVisualize ) = 0;

	/** @param InDelegate A delegate to access source code with */
	virtual void SetSourceAccessDelegate( FAccessSourceCode InDelegate ) = 0;
	

	/**
	 * @param ThisWindow    Do we want to draw something for this window?
	 *
	 * @return true if we want to draw something for this window; false otherwise
	 */
	virtual bool ReflectorNeedsToDrawIn( TSharedRef<SWindow> ThisWindow ) const = 0;

	/** @return A new widget reflector */
	static TSharedRef<SWidgetReflector> Make();

};
