// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VisualLogger/VisualLogger.h"

struct FLogEntryBarProxy
{
	FLogEntryBarProxy(const FVisualLogEntry* LogEntry)
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
	DECLARE_DELEGATE_OneParam(FOnSelectionChanged, TSharedPtr<FVisualLogEntry>);
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

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime);

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	
	void OnCurrentTimeChanged(float NewTime);

	FVector2D ComputeDesiredSize() const;

	/**
	 * Adds profiler events to draw as bars
	 *
	 * @param InEvent	Events to draw
	 */
	void SetEntries(const TArray<TSharedPtr<FVisualLogEntry> >& InEntries, float InStartTime, float InTotalTime);
	
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

	void SetHistogramWindow(float InWindowSize);

	float GetHistogramPreviewWindow() { return HistogramPreviewWindow; }

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

	FORCEINLINE_DEBUGGABLE bool CalculateEntryGeometry(const FVisualLogEntry* LogEntry, const FGeometry& InGeometry, float& OutStartX, float& OutEndX) const
	{
		const float EventStart = (LogEntry->TimeStamp - StartTime) / TotalTime;
		const float EventDuration = TimeUnit / TotalTime;
		const float Start = Offset + EventStart * Zoom;
		const float ClampedStart = FMath::Clamp(Start, 0.0f, 1.0f);
		const float End = Offset + (EventStart + EventDuration) * Zoom;
		const float ClampedEnd = FMath::Clamp(End, 0.0f, 1.0f);
		const float ClampedSize = ClampedEnd - ClampedStart;
		const float BarWidth = InGeometry.Size.X - 3;
		OutStartX = (float)(BarWidth * ClampedStart);
		OutEndX = OutStartX + (float)FMath::Max(BarWidth * ClampedSize, ClampedEnd > 0.0f ? SubPixelMinSize : 0.0f);
		return End >= 0.0f && Start <= 1.0f;
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
	TArray<TSharedPtr<FVisualLogEntry> > Entries;

	/** Start time (0.0 - 1.0) */
	float StartTime;
	
	/** End time (0.0 - 1.0) */
	float TotalTime;

	/** Current zoom of the graph */
	float Zoom;

	/** Current offset of the graph */
	float Offset;

	float HistogramPreviewWindow;

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
};
