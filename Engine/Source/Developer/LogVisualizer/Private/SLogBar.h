// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VisualLog.h"

struct FLogEntryBarProxy
{
	FLogEntryBarProxy(const FVisLogEntry* LogEntry)
	{

	}
};

/** A log bar widget.*/
class SLogBar : public SLeafWidget
{
	typedef SLeafWidget Super;

public:
	static const float SubPixelMinSize;
	static const float TimeUnit;
	static const float MaxUnitSizePx;

	/** Delegate used allotted geometry changes */
	DECLARE_DELEGATE_OneParam(FOnGeometryChanged, FGeometry);
	DECLARE_DELEGATE_OneParam(FOnSelectionChanged, TSharedPtr<FVisLogEntry>);
	DECLARE_DELEGATE_RetVal(bool, FShouldDrawSelection);
	DECLARE_DELEGATE_RetVal(float, FCurrentDisplayedTime);
	DECLARE_DELEGATE_RetVal(int32, FCurrentEntryIndex);

public:
	SLATE_BEGIN_ARGS(SLogBar)
		: _OnSelectionChanged()
		{}
		SLATE_EVENT( FOnSelectionChanged, OnSelectionChanged )
		SLATE_EVENT( FOnGeometryChanged, OnGeometryChanged )
		SLATE_EVENT( FShouldDrawSelection, ShouldDrawSelection )
		SLATE_EVENT( FCurrentDisplayedTime, DisplayedTime )
		SLATE_EVENT( FCurrentEntryIndex, CurrentEntryIndex )
	SLATE_END_ARGS()

	/**
	 * Construct the widget
	 * 
	 * @param InArgs   A declaration from which to construct the widget
	 */
	void Construct(const FArguments& InArgs);

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
	virtual int32 OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const OVERRIDE;
	
	/**
	 * Ticks this widget.  Override in derived classes, but always call the parent implementation.
	 *
	 * @param  AllottedGeometry The space allotted for this widget
	 * @param  InCurrentTime  Current absolute real time
	 * @param  InDeltaTime  Real time passed since last tick
	 */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime);

	/**
	 * The system calls this method to notify the widget that a mouse button was pressed within it. This event is bubbled.
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param MouseEvent Information about the input event
	 *
	 * @return Whether the event was handled along with possible requests for the system to take action.
	 */
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;

	/**
	 * The system calls this method to notify the widget that a mouse moved within it. This event is bubbled.
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param MouseEvent Information about the input event
	 *
	 * @return Whether the event was handled along with possible requests for the system to take action.
	 */
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;
	
	void OnCurrentTimeChanged(float NewTime);

	/**
	 * A Panel's desired size in the space required to arrange of its children on the screen while respecting all of
	 * the children's desired sizes and any layout-related options specified by the user. See StackPanel for an example.
	 *
	 * @return The desired size.
	 */
	FVector2D ComputeDesiredSize() const;

	/**
	 * Adds profiler events to draw as bars
	 *
	 * @param InEvent	Events to draw
	 */
	void SetEntries(const TArray<TSharedPtr<FVisLogEntry> >& InEntries, float InStartTime, float InTotalTime);
	
	void SelectEntry(const FGeometry& MyGeometry, const float ClickX);

	/**
	 * Sets the graph's zoom level
	 *
	 * @param NewValue	Zoom value, minimum is 1.0
	 */
	void SetZoom(float InZoom)
	{
		Zoom = FMath::Max(InZoom, 1.0f);
	}

	/**
	 * Sets the graph's offset by which all graph bars should be moved
	 *
	 * @param NewValue	Offset value
	 */
	void SetOffset(float InOffset)
	{
		Offset = InOffset;
	} 

	void SetZoomAndOffset(float InZoom, float InOffset);

	/**
	 * Gets the graph's offset value
	 *
	 * @return Offset value
	 */
	float GetOffset() const
	{
		return Offset;
	}

	void SetRowIndex(const int32 Index) 
	{
		RowIndex = Index;
	}

	int32 GetEntryIndexAtTime(float Time) const;

	void UpdateShouldDrawSelection();
		
private:

	void SelectEntryAtIndex(const int32 Index);

	FORCEINLINE_DEBUGGABLE bool CalculateEntryGeometry(const FVisLogEntry* LogEntry, const FGeometry& InGeometry, float& OutStartX, float& OutEndX) const
	{
		const float EventStart = (LogEntry->TimeStamp - StartTime) / TotalTime;
		const float EventDuration = TimeUnit / TotalTime;
		const float ClampedStart = FMath::Clamp(Offset + EventStart * Zoom, 0.0f, 1.0f );		
		const float ClampedEnd = FMath::Clamp(Offset + (EventStart + EventDuration) * Zoom, 0.0f, 1.0f );
		const float ClampedSize = ClampedEnd - ClampedStart;
		OutStartX = (float)(InGeometry.Size.X * ClampedStart);
		OutEndX = OutStartX + (float)FMath::Max(InGeometry.Size.X * ClampedSize, ClampedEnd > 0.0f ? SubPixelMinSize : 0.0f);
		return ClampedEnd > 0.0f && ClampedStart < 1.0f;
	}

	/** Delegate to invoke when selection changes. */
	FOnSelectionChanged OnSelectionChanged;

	/** Background image to use for the graph bar */
	const FSlateBrush* BackgroundImage;

	/** Foreground image to use for the graph bar */
	const FSlateBrush* FillImage;

	/** Image to be used when drawing selected event */
	const FSlateBrush* SelectedImage;

	const FSlateBrush* TimeMarkImage;

	/** List of all events to draw */
	TArray<TSharedPtr<FVisLogEntry> > Entries;

	/** Start time (0.0 - 1.0) */
	float StartTime;
	
	/** End time (0.0 - 1.0) */
	float TotalTime;

	/** Current zoom of the graph */
	float Zoom;

	/** Current offset of the graph */
	float Offset;

	/** Last hovered event index */
	int32 LastHoveredEvent;
	
	/** Last allotted geometry */
	FGeometry LastGeometry;

	/** Delegate called when the geometry changes */
	FOnGeometryChanged OnGeometryChanged;

	FShouldDrawSelection ShouldDrawSelection;
	FCurrentDisplayedTime DisplayedTime;
	FCurrentEntryIndex CurrentEntryIndex;

	int32 RowIndex;

	int32 bShouldDrawSelection : 1;

	/** Color palette for bars coloring */
	static FColor	ColorPalette[];
};
