// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

struct FLogsListItem;

/** Implements a row widget for log list. */
class SLogsTableRow : public SMultiColumnTableRow< TSharedPtr<FLogsListItem> >
{
	typedef SMultiColumnTableRow< TSharedPtr<FLogsListItem> > Super;
public:

	SLATE_BEGIN_ARGS(SLogsTableRow) {}
		SLATE_ARGUMENT(TSharedPtr<class SLogVisualizer>, OwnerVisualizerWidget)
		SLATE_ARGUMENT(TSharedPtr<FLogsListItem>, Item)
	SLATE_END_ARGS()


public:

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) OVERRIDE;
		
private:
	int32 GetCurrentLogEntryIndex() const;

	void OnBarGraphSelectionChanged(TSharedPtr<FVisLogEntry> Selection);
	void OnBarGeometryChanged(FGeometry Geometry);

	bool ShouldDrawSelection();

	TArray<TSharedPtr<FVisLogEntry> > VisibleEntries;

	/** Tree item */
	TSharedPtr<FLogsListItem> Item;
	/** Analyzer widget that owns us */
	TWeakPtr<SLogVisualizer> OwnerVisualizerWidgetPtr;

	TSharedPtr<class SLogBar> LogBar;
};