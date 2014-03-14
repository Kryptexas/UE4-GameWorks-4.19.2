// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "STimeline.h"
#include "SLogCategoryFilter.h"
#include "Debug/DebugDrawService.h"

struct FLogsListItem
{
	FString Name;
	float StartTimestamp;
	float EndTimestamp;
	int32 LogIndex;
	
	FLogsListItem(const FString& InName, float InStart, float InEnd, int32 InLogIndex = INDEX_NONE)
		: Name(InName), StartTimestamp(InStart), EndTimestamp(InEnd), LogIndex(InLogIndex)
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

/** Main LogVisualizer UI widget */
class SLogVisualizer : public SCompoundWidget
{
public:
	static const FName NAME_LogName;
	static const FName NAME_StartTime;
	static const FName NAME_EndTime;
	static const FName NAME_LogTimeSpan;

	/** Expressed in Hz */
	static const int32 FullUpdateFrequency = 2; 

	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnZoomChanged, float, float);

	SLATE_BEGIN_ARGS(SLogVisualizer) {}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FLogVisualizer* InAnalyzer);

	virtual ~SLogVisualizer();

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;

	// OVERRIDES
	virtual FReply OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent) OVERRIDE;

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
	void OnSortByChanged(const FName& ColumnName, EColumnSortMode::Type NewSortMode);
	// Table delegates
	TSharedRef<ITableRow> LogsListGenerateRow(TSharedPtr<FLogsListItem> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void LogsListSelectionChanged(TSharedPtr<FLogsListItem> SelectedItem, ESelectInfo::Type SelectInfo);
	TSharedRef<ITableRow> LogEntryLinesGenerateRow(TSharedPtr<FLogEntryItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	
	FOnZoomChanged& OnZoomChanged() { return ZoomChangedNotify; }

	void DoFullUpdate();
	void RequestFullUpdate() { TimeTillNextUpdate = 0; }

	float GetZoom() const
	{
		return MinZoom + ZoomSliderValue * ( MaxZoom - MinZoom );
	}
	float GetScrollbarOffset() const { return ScrollbarOffset; }

	TSharedPtr<STimeline> GetTimeline() { return Timeline; }

	void SetCurrentViewedTime(float NewTime, const bool bForce = false);

	void RequestShowLogEntry(TSharedPtr<FLogsListItem> Item, TSharedPtr<FVisLogEntry> LogEntry);

	void GetVisibleEntries(const TSharedPtr<FActorsVisLog>& Log, TArray<TSharedPtr<FVisLogEntry> >& OutEntries);

	int32 GetCurrentVisibleLogEntryIndex(const TArray<TSharedPtr<FVisLogEntry> >& InVisibleEntries);

protected:
	void ShowLogEntry(TSharedPtr<FLogsListItem> Item, TSharedPtr<FVisLogEntry> LogEntry);

	void SelectionChanged(class AActor* DebuggedActor, bool bIsBeingDebuggedNow);
public:	
	/** Pointer to the visualizer object we want to show ui for */
	FLogVisualizer* LogVisualizer;
	
	/** Current way we are sorting logs */
	ELogsSortMode::Type SortBy;

	float LogsStartTime;
	float LogsEndTime;

	float GetCurrentViewedTime() const { return CurrentViewedTime; }
	
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
	/** Construct main menu */
	TSharedRef< SWidget > MakeMainMenu();
	void FillHelpMenu(FMenuBuilder& MenuBuilder);
	void OpenSavedSession(FMenuBuilder& MenuBuilder);

	FString GetLogEntryStatusText() const;

	void AddLog(int32 Index, const FActorsVisLog* Log);

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

	/** handler for toggling DrawLogEntriesPath option change */
	void OnDrawLogEntriesPathChanged(ESlateCheckBoxState::Type NewState);
	/** retrieves whether DrawLogEntriesPath checkbox should be checked */
	ESlateCheckBoxState::Type GetDrawLogEntriesPathState() const;

	/** handler for toggling IgnoreTrivialLogs option change */
	void OnIgnoreTrivialLogs(ESlateCheckBoxState::Type NewState);
	/** retrieves whether IgnoreTrivialLogs checkbox should be checked */
	ESlateCheckBoxState::Type GetIgnoreTrivialLogs() const;

	/** handler for toggling log visualizer camera in game view */
	void OnToggleCamera(ESlateCheckBoxState::Type NewState);
	/** retrieves whether ToggleCamera toggle button should appear pressed */
	ESlateCheckBoxState::Type GetToggleCameraState() const;

	/** See if a particular log passes the current filter and other display flags */
	bool ShouldListLog(const FActorsVisLog& Log);

	void ShowEntry(const FVisLogEntry* LogEntry);

	int32 FindIndexInLogsList(const int32 LogIndex) const;

	void OnLogCategoryFiltersChanged();

	UWorld* GetWorld() const;

	// MEMBERS

	/** Index into LogVisualizer->Logs array for entries you want to show. */
	TArray<TSharedPtr<FLogsListItem> >	LogsList;
	/** filled with current log entry's log lines is a source of LogsLinesWidget */
	TArray<TSharedPtr<FLogEntryItem> > LogEntryLines;
	
	TArray<FString> UsedCategories;
	FLinearColor GetColorForUsedCategory(int32 Index) const;

	/** used to filter logs by name */
	FString LogNameFilterString;

	FString LastBrowsePath;

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

	FDebugDrawDelegate DrawingOnCanvasDelegate;

	bool bDrawLogEntriesPath;
	bool bIgnoreTrivialLogs;

	TWeakObjectPtr<class ALogVisualizerCameraController> CameraController;

	// WIDGETS

	/** Main log list widget */
	TSharedPtr<SListView<TSharedPtr<FLogsListItem> > > LogsListWidget;
	
	TSharedPtr<SEditableTextBox> LogNameFilterBox;
	TSharedPtr<STextBlock> StatusTextBlock;
	TSharedPtr<SListView<TSharedPtr<FLogEntryItem> > > LogsLinesWidget;
	TSharedPtr<STimeline> Timeline;
	TSharedPtr<SScrollBar> ScrollBar;
	TSharedPtr<SSlider> ZoomSlider;
	TSharedPtr<SLogCategoryFilter> LogFilter;
};
