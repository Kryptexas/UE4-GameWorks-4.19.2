// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ScriptPerfData.h"
#include "TracePath.h"

//////////////////////////////////////////////////////////////////////////
// SProfilerStatRow

/** Enum for each column type in the execution tree widget */
namespace EBlueprintProfilerStat
{
	enum Type
	{
		Name = 0,
		TotalTime,
		InclusiveTime,
		Time,
		PureTime,
		MaxTime,
		MinTime,
		Samples
	};
};

// Shared pointer to a performance stat
typedef TSharedPtr<class FBPProfilerStatWidget> FBPStatWidgetPtr;

class SProfilerStatRow : public SMultiColumnTableRow<FBPStatWidgetPtr>
{
protected:

	/** The profiler statistic associated with this stat row */
	FBPStatWidgetPtr ItemToEdit;

public:

	SLATE_BEGIN_ARGS(SProfilerStatRow){}
	SLATE_END_ARGS()

	/** Constructs the statistic row item widget */
	void Construct(const FArguments& InArgs, TSharedRef<STableViewBase> OwnerTableView, FBPStatWidgetPtr InItemToEdit);

	/** Generates and returns a widget for the specified column name and the associated statistic */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

	/** Converts the profiler stat enum into the associated column FName */
	static const FName GetStatName(const EBlueprintProfilerStat::Type StatId);

	/** Converts the profiler stat enum into the associated column FText */
	static const FText GetStatText(const EBlueprintProfilerStat::Type StatId);

};

//////////////////////////////////////////////////////////////////////////
// FBPProfilerStatDiplayOptions

struct FBPProfilerStatDiplayOptions : public TSharedFromThis<FBPProfilerStatDiplayOptions>
{
public:

	/** Display flags controlling widget appearance */
	enum EDisplayFlags
	{
		DisplayByInstance			= 0x00000001,	// Show instance stats
		ScopeToDebugInstance		= 0x00000002,	// Scope stats to current debug instance
		GraphFilter					= 0x00000004,	// Filter to current graph
		DisplayPure					= 0x00000008,	// Display pure stats
		AutoExpand					= 0x00000010,	// Auto expand stats
		Modified					= 0x10000000,	// Display state was changed
		Default						= DisplayByInstance|ScopeToDebugInstance|DisplayPure|GraphFilter|Modified
	};

	FBPProfilerStatDiplayOptions()
		: ActiveInstance(NAME_None)
		, ActiveGraph(NAME_None)
		, Flags(Default)
	{
	}

	/** Marks display state as modified */
	void SetStateModified() { Flags |= Modified; }

	/** Returns if the filter state options are modified */
	bool IsStateModified() const { return (Flags & Modified) != 0U; }

	/** Add flags */
	void AddFlags(const uint32 FlagsIn) { Flags |= FlagsIn; }

	/** Clear flags */
	void ClearFlags(const uint32 FlagsIn) { Flags &= ~FlagsIn; }

	/** Returns true if any flags match */
	bool HasFlags(const uint32 FlagsIn) const { return (Flags & FlagsIn) != 0U; }

	/** Returns true if all flags match */
	bool HasAllFlags(const uint32 FlagsIn) const { return (Flags & FlagsIn) == FlagsIn; }

	/** Set active instance name */
	FName GetActiveInstance() const { return ActiveInstance; }

	/** Set active instance name */
	void SetActiveInstance(const FName InstanceName);

	/** Set active graph name */
	void SetActiveGraph(const FName GraphName);

	/** Create toolbar widget */
	TSharedRef<SWidget> CreateToolbar();

	/** Returns if the UI should be checked for flags */
	ECheckBoxState GetChecked(const uint32 FlagsIn) const;

	/** Handles UI check states */
	void OnChecked(ECheckBoxState NewState, const uint32 FlagsIn);

	/** Returns true if the exec node passes current filters */
	bool IsFiltered(TSharedPtr<class FScriptExecutionNode> Node) const;

private:

	/** The active instance name */
	FName ActiveInstance;
	/** The active graph name */
	FName ActiveGraph;
	/** The display flags */
	uint32 Flags;
};

//////////////////////////////////////////////////////////////////////////
// FBPProfilerStatWidget

class FBPProfilerStatWidget : public TSharedFromThis<FBPProfilerStatWidget>
{
public:

	FBPProfilerStatWidget(TSharedPtr<class FScriptExecutionNode> InExecNode, const FTracePath& WidgetTracePathIn)
		: WidgetTracePath(WidgetTracePathIn)
		, ExecNode(InExecNode)
	{
	}

	virtual ~FBPProfilerStatWidget() {}

	/** Generate exec node widgets */
	virtual void GenerateExecNodeWidgets(const TSharedPtr<FBPProfilerStatDiplayOptions> DisplayOptions);

	/** Gather children for list/tree widget */
	void GatherChildren(TArray<TSharedPtr<FBPProfilerStatWidget>>& OutChildren);

	/** Generate column widget */
	TSharedRef<SWidget> GenerateColumnWidget(FName ColumnName);

	/** Returns the exec path object context for this statistic */
	TSharedPtr<class FScriptExecutionNode> GetExecNode() { return ExecNode; }

	/** Returns the performance data object for this statistic */
	TSharedPtr<FScriptPerfData> GetPerformanceData() { return PerformanceStats; }

	/** Get expansion state */
	bool GetExpansionState() const;

	/** Set expansion state */
	void SetExpansionState(bool bExpansionStateIn);

	/** Expand widget state */
	void ExpandWidgetState(TSharedPtr<STreeView<FBPStatWidgetPtr>> TreeView, bool bStateIn);

	/** Restore widget Expansion state */
	void RestoreWidgetExpansionState(TSharedPtr<STreeView<FBPStatWidgetPtr>> TreeView);

	/** Returns if any children expect to be expanded */
	bool ProbeChildWidgetExpansionStates();

protected:

	/** Navigate to referenced node */
	void NavigateTo() const;

protected:

	/** Widget script tracepath */
	FTracePath WidgetTracePath;
	/** Script execution event node */
	TSharedPtr<FScriptExecutionNode> ExecNode;
	/** Script performance data */
	TSharedPtr<FScriptPerfData> PerformanceStats;
	/** Child statistics */
	TArray<TSharedPtr<FBPProfilerStatWidget>> CachedChildren;

};
