// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "STimeline.h"
#include "Debug/DebugDrawService.h"

class SLogFilterList;

struct FLogsListItem
{
	FString Name;
	float StartTimestamp;
	float EndTimestamp;
	float LastEndTimestamp;
	int32 LogIndex;
	
	FLogsListItem(const FString& InName, float InStart, float InEnd, int32 InLogIndex = INDEX_NONE)
		: Name(InName), StartTimestamp(InStart), EndTimestamp(InEnd), LastEndTimestamp(0), LogIndex(InLogIndex)
	{
	}

	bool operator==(const FLogsListItem& Other) const
	{
		return LogIndex == Other.LogIndex;
	}
};

struct FLogEntryItem
{
	FString Category;
	FLinearColor CategoryColor;
	ELogVerbosity::Type Verbosity;
	FString Line;
	int64 UserData;
	FName TagName;
};

namespace ELogsSortMode
{
	enum Type
	{
		ByName,
		ByStartTime,
		ByEndTime,
	};
}

struct FLogStatusItem
{
	FString ItemText;
	FString ValueText;

	TArray< TSharedPtr< FLogStatusItem > > Children;

	FLogStatusItem(const FString& InItemText) : ItemText(InItemText) {}
	FLogStatusItem(const FString& InItemText, const FString& InValueText) : ItemText(InItemText), ValueText(InValueText) {}
};

template <typename ItemType>
class SLogListView;

/** Main LogVisualizer UI widget */
class SLogVisualizer : public SCompoundWidget
{
#if ENABLE_VISUAL_LOG
public:
	static const FName NAME_LogName;
	static const FName NAME_StartTime;
	static const FName NAME_EndTime;
	static const FName NAME_LogTimeSpan;

	/** Expressed in Hz */
	static const int32 FullUpdateFrequency = 2;

	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnZoomChanged, float, float);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnHistogramWindowChanged, float);

	SLATE_BEGIN_ARGS(SLogVisualizer) {}

	SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, FLogVisualizer* InAnalyzer);

	virtual ~SLogVisualizer();

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;

	// overrides
	virtual FReply OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	// Get delegates
	const FSlateBrush* GetRecordButtonBrush() const;
	FString GetStatusText() const;
	ESlateCheckBoxState::Type GetPauseState() const;
	EColumnSortMode::Type GetLogsSortMode() const;
	// Handler delegates
	FReply OnRecordButtonClicked();
	FReply OnLoad();
	FReply OnSave();
	FReply OnRemove();
	void OnPauseChanged(ESlateCheckBoxState::Type NewState);
	void FilterTextCommitted(const FText& CommentText, ETextCommit::Type CommitInfo);
	void OnQuickFilterTextChanged(const FText& CommentText, ETextCommit::Type CommitInfo);
	void OnSortByChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnName, const EColumnSortMode::Type NewSortMode);
	// Table delegates
	TSharedRef<ITableRow> LogsListGenerateRow(TSharedPtr<FLogsListItem> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void LogsListSelectionChanged(TSharedPtr<FLogsListItem> SelectedItem, ESelectInfo::Type SelectInfo);
	TSharedRef<ITableRow> LogEntryLinesGenerateRow(TSharedPtr<FLogEntryItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void LogEntryLineSelectionChanged(TSharedPtr<FLogEntryItem> SelectedItem, ESelectInfo::Type SelectInfo);
	
	FOnZoomChanged& OnZoomChanged() { return ZoomChangedNotify; }

	FOnHistogramWindowChanged& OnHistogramWindowChanged() { return HistogramWindowChangedNotify; }

	TSharedPtr<SWidget> GetListRightClickMenuContent();
	void GenerateReport();

	void OnListDoubleClick(TSharedPtr<FLogsListItem>);
	void DoFullUpdate();
	void DoTickUpdate();
	void RequestFullUpdate() { TimeTillNextUpdate = 0; }

	float GetZoom() const
	{
		return MinZoom + ZoomSliderValue * ( MaxZoom - MinZoom );
	}
	float GetScrollbarOffset() const { return ScrollbarOffset; }

	TSharedPtr<STimeline> GetTimeline() { return Timeline; }

	void SetCurrentViewedTime(float NewTime, const bool bForce = false);

	void RequestShowLogEntry(TSharedPtr<FLogsListItem> Item, TSharedPtr<FVisualLogEntry> LogEntry);

	void UpdateVisibleEntriesCache(const TSharedPtr<FActorsVisLog>& Log, int32 Index);
	void GetVisibleEntries(const TSharedPtr<FActorsVisLog>& Log, TArray<TSharedPtr<FVisualLogEntry> >& OutEntries);

	int32 GetCurrentVisibleLogEntryIndex(const TArray<TSharedPtr<FVisualLogEntry> >& InVisibleEntries);

protected:
	void ShowLogEntry(TSharedPtr<FLogsListItem> Item, TSharedPtr<FVisualLogEntry> LogEntry);

	void SelectionChanged(class AActor* DebuggedActor, bool bIsBeingDebuggedNow);
public:	
	/** Pointer to the visualizer object we want to show ui for */
	FLogVisualizer* LogVisualizer;
	
	/** Current way we are sorting logs */
	ELogsSortMode::Type SortBy;

	float LogsStartTime;
	float LogsEndTime;

	float GetCurrentViewedTime() const { return CurrentViewedTime; }

	static FLinearColor GetColorForUsedCategory(int32 Index);
	float GetHistogramPreviewWindow() { return HistogramPreviewWindow; }

protected:
	float GetMaxScrollOffsetFraction() const
	{
		return 1.0f - 1.0f / GetZoom();
	}

	float GetMaxGraphOffset() const
	{
		return GetZoom() - 1.0f;
	}


	void DrawOnCanvas(UCanvas* Canvas, APlayerController*);

	void LoadFiles(TArray<FString>& OpenFilenames);
	void SaveSelectedLogs(FString& Filename);

private:
	void InvalidateCanvas();
	void UpdateStatusItems(const FVisualLogEntry* LogEntry);
	TSharedRef<ITableRow> HandleGenerateLogStatus(TSharedPtr<FLogStatusItem> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnLogStatusGetChildren(TSharedPtr<FLogStatusItem> InItem, TArray< TSharedPtr<FLogStatusItem> >& OutItems);

	/** Construct main menu */
	TSharedRef< SWidget > MakeMainMenu();
	void FillHelpMenu(FMenuBuilder& MenuBuilder);
	void OpenSavedSession(FMenuBuilder& MenuBuilder);

	FString GetLogEntryStatusText() const;

	void AddOrUpdateLog(int32 Index, const FActorsVisLog* Log);

	void SelectActor(class AActor* SelectedActor);
	void CameraActorSelected(class AActor* SelectedActor);	

	void IncrementCurrentLogIndex(int32 IncrementBy);

	/** Called when a single log is added */
	void OnLogAdded();
	/** Regenerate logs list based on current filter */
	void RebuildFilteredList();
	/** Update filtering members from entry widgets */
	void UpdateFilterInfo();
	
	// zoom functionality

	float GetZoomValue() const;
	void OnSetZoomValue(float NewValue);
	void OnZoomScrolled(float InScrollOffsetFraction);

	void OnSetHistogramWindowValue(float NewValue);

	/** handler for toggling DrawLogEntriesPath option change */
	void OnDrawLogEntriesPathChanged(ESlateCheckBoxState::Type NewState);
	/** retrieves whether DrawLogEntriesPath checkbox should be checked */
	ESlateCheckBoxState::Type GetDrawLogEntriesPathState() const;

	/** handler for toggling IgnoreTrivialLogs option change */
	void OnIgnoreTrivialLogs(ESlateCheckBoxState::Type NewState);
	/** retrieves whether IgnoreTrivialLogs checkbox should be checked */
	ESlateCheckBoxState::Type GetIgnoreTrivialLogs() const;

	void OnChangeHistogramLabelLocation(ESlateCheckBoxState::Type NewState);
	ESlateCheckBoxState::Type GetHistogramLabelLocation() const;

	void OnStickToLastData(ESlateCheckBoxState::Type NewState);
	ESlateCheckBoxState::Type GetStickToLastData() const;
	
	/** handler for toggling log visualizer camera in game view */
	void OnToggleCamera(ESlateCheckBoxState::Type NewState);
	/** retrieves whether ToggleCamera toggle button should appear pressed */
	ESlateCheckBoxState::Type GetToggleCameraState() const;

	void OnOffsetDataSets(ESlateCheckBoxState::Type NewState);
	ESlateCheckBoxState::Type GetOffsetDataSets() const;

	void OnHistogramGraphsFilter(ESlateCheckBoxState::Type NewState);
	ESlateCheckBoxState::Type GetHistogramGraphsFilter() const;
	
	/** See if a particular log passes the current filter and other display flags */
	bool ShouldListLog(const TSharedPtr<FActorsVisLog>& Log);

	void ShowEntry(const FVisualLogEntry* LogEntry);

	int32 FindIndexInLogsList(const int32 LogIndex) const;

	void OnLogCategoryFiltersChanged();

	UWorld* GetWorld() const;

	// MEMBERS

	/** The filter list */
	TSharedPtr<SLogFilterList> FilterListPtr;

	/** Index into LogVisualizer->Logs array for entries you want to show. */
	TArray<TSharedPtr<FLogsListItem> >	LogsList;
	/** filled with current log entry's log lines is a source of LogsLinesWidget */
	TArray<TSharedPtr<FLogEntryItem> > LogEntryLines;
	
	TArray<FString> UsedCategories;
	TMap<FString, TArray<FString> > UsedGraphCategories;
	/** Color palette for bars coloring */
	static FColor ColorPalette[];

	/** used to filter logs by name */
	FString LogNameFilterString;

	FString LastBrowsePath;

	FString QuickFilterText;

	/** index of currently viewed log */
	int32 LogEntryIndex;
	/** index of entry withing currently viewed log  */
	int32 SelectedLogIndex;
	
	float TimeTillNextUpdate;

	/** Zoom slider value */
	float ZoomSliderValue;
	float ScrollbarOffset;
	float LastBarsOffset;

	float CurrentViewedTime;

	float MinZoom;
	float MaxZoom;
	
	FOnZoomChanged ZoomChangedNotify;
	FOnHistogramWindowChanged HistogramWindowChangedNotify;
	float HistogramPreviewWindow;

	FDebugDrawDelegate DrawingOnCanvasDelegate;

	bool bDrawLogEntriesPath;
	bool bIgnoreTrivialLogs;
	bool bShowHistogramLabelsOutside;
	bool bStickToLastData;
	bool bOffsetDataSet;
	bool bHistogramGraphsFilter;

	TWeakObjectPtr<class ALogVisualizerCameraController> CameraController;
	TArray< TSharedPtr<FLogStatusItem> > StatusItems;

	struct FCachedEntries
	{
		TSharedPtr<FActorsVisLog> Log;
		TArray<TSharedPtr<FVisualLogEntry> > CachedEntries;
	};
	TArray<FCachedEntries> OutEntriesCached;
		
	// WIDGETS

	/** Main log list widget */
	TSharedPtr<SLogListView<TSharedPtr<FLogsListItem> > > LogsListWidget;
	TSharedPtr< STreeView< TSharedPtr<FLogStatusItem> > > StatusItemsView;
	TSharedPtr<SEditableTextBox> LogNameFilterBox;
	TSharedPtr<SEditableTextBox> QuickFilterBox;
	TSharedPtr<STextBlock> StatusTextBlock;
	TSharedPtr<SListView<TSharedPtr<FLogEntryItem> > > LogsLinesWidget;
	TSharedPtr<STimeline> Timeline;
	TSharedPtr<SScrollBar> ScrollBar;
	TSharedPtr<SSlider> ZoomSlider;
#endif
};
