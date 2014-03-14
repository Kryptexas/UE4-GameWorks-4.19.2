// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** A custom widget that acts as a container for widgets like SDataGraph or SEventTree. */
class SProfilerGraphPanel : public SCompoundWidget
{
public:
	/** Default constructor. */
	SProfilerGraphPanel();
		
	/** Virtual destructor. */
	virtual ~SProfilerGraphPanel();

	SLATE_BEGIN_ARGS( SProfilerGraphPanel ){}
	SLATE_END_ARGS()

	/**
	 * Construct this widget
	 *
	 * @param InArgs - The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs );

	// SWidget interface
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;
	// End of SWidget interface

	TSharedPtr<SDataGraph>& GetMainDataGraph()
	{
		return DataGraph;
	}

protected:
	/** Called when the status of specified tracked stat has changed. */
	void ProfilerManager_OnTrackedStatChanged( const FTrackedStat& TrackedStat, bool bIsTracked );

	/** Called when the list of session instances have changed. */
	void ProfilerManager_OnSessionInstancesUpdated();

	/**
	 * Called when the user scrolls the scrollbar
	 *
	 * @param ScrollOffset - scroll offset as a fraction between 0 and 1.
	 */
	void ScrollBar_OnUserScrolled( float ScrollOffset );

	/** Called when the frame offset has been changed from the data graph widget. */
	void DataGraph_OnGraphOffsetChanged( int InFrameOffset );

	/** Called when the data graph view mode has changed from the data graph widget. */
	void DataGraph_OnViewModeChanged( EDataGraphViewModes::Type ViewMode );

	void UpdateInternals();

protected:
	/** Holds the data graph widget. */
	TSharedPtr<SDataGraph> DataGraph;

	/** Widget used for scrolling graphs. */
	TSharedPtr<SScrollBar> ScrollBar;

	/** Number of graph points. */
	int32 NumDataPoints;

	/** Number of graph points that can be displayed at once in this widget. */
	int32 NumVisiblePoints;

	/** Current offset of the graph, index of the first visible graph point. */
	int32 GraphOffset;
};