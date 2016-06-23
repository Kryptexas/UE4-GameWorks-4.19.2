// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "BPProfilerStatisticWidgets.h"
#include "SBlueprintProfilerToolbar.h"
#include "SGraphExecutionStatDisplay.h"
#include "BlueprintEditor.h"
#include "Public/Profiler/EventExecution.h"
#include "SDockTab.h"
#include "Developer/BlueprintProfiler/Public/ScriptInstrumentationPlayback.h"

#define LOCTEXT_NAMESPACE "BlueprintProfilerGraphExecView"

//////////////////////////////////////////////////////////////////////////
// SGraphExecutionStatDisplay

SGraphExecutionStatDisplay::~SGraphExecutionStatDisplay()
{
	// Remove delegate for profiling toggle events
	FBlueprintCoreDelegates::OnToggleScriptProfiler.RemoveAll(this);
	// Remove delegate for graph structural changes
	if (IBlueprintProfilerInterface* Profiler = FModuleManager::GetModulePtr<IBlueprintProfilerInterface>("BlueprintProfiler"))
	{
		Profiler->GetGraphLayoutChangedDelegate().RemoveAll(this);
	}
}

void SGraphExecutionStatDisplay::Construct(const FArguments& InArgs)
{	
	BlueprintEditor = InArgs._AssetEditor;
	DisplayOptions = InArgs._DisplayOptions;
	// Register for profiling toggle events
	FBlueprintCoreDelegates::OnToggleScriptProfiler.AddSP(this, &SGraphExecutionStatDisplay::OnToggleProfiler);
	// Register delegate for graph structural changes
	if (IBlueprintProfilerInterface* Profiler = FModuleManager::GetModulePtr<IBlueprintProfilerInterface>("BlueprintProfiler"))
	{
		Profiler->GetGraphLayoutChangedDelegate().AddSP(this, &SGraphExecutionStatDisplay::OnGraphLayoutChanged);
	}

	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		[
			SNew(SBorder)
			.Padding(0)
			.BorderImage(FEditorStyle::GetBrush("NoBorder"))
			[
				SAssignNew(ExecutionStatTree, STreeView<FBPStatWidgetPtr>)
				.TreeItemsSource(&RootTreeItems)
				.SelectionMode(ESelectionMode::Single)
				.OnGetChildren(this, &SGraphExecutionStatDisplay::OnGetChildren)
				.OnGenerateRow(this, &SGraphExecutionStatDisplay::OnGenerateRow)
				.OnMouseButtonDoubleClick(this, &SGraphExecutionStatDisplay::OnDoubleClickStatistic)
				.OnExpansionChanged(this, &SGraphExecutionStatDisplay::OnStatisticExpansionChanged)
				.HeaderRow
				(
					SNew(SHeaderRow)
					+SHeaderRow::Column(SProfilerStatRow::GetStatName(EBlueprintProfilerStat::Name))
					.DefaultLabel(SProfilerStatRow::GetStatText(EBlueprintProfilerStat::Name))
					.ManualWidth(450)
					+SHeaderRow::Column(SProfilerStatRow::GetStatName(EBlueprintProfilerStat::AverageTime))
					.DefaultLabel(SProfilerStatRow::GetStatText(EBlueprintProfilerStat::AverageTime))
					.ManualWidth(90)
					+SHeaderRow::Column(SProfilerStatRow::GetStatName(EBlueprintProfilerStat::InclusiveTime))
					.DefaultLabel(SProfilerStatRow::GetStatText(EBlueprintProfilerStat::InclusiveTime))
					.ManualWidth(120)
					+SHeaderRow::Column(SProfilerStatRow::GetStatName(EBlueprintProfilerStat::MaxTime))
					.DefaultLabel(SProfilerStatRow::GetStatText(EBlueprintProfilerStat::MaxTime))
					.ManualWidth(90)
					+SHeaderRow::Column(SProfilerStatRow::GetStatName(EBlueprintProfilerStat::MinTime))
					.DefaultLabel(SProfilerStatRow::GetStatText(EBlueprintProfilerStat::MinTime))
					.ManualWidth(90)
					+SHeaderRow::Column(SProfilerStatRow::GetStatName(EBlueprintProfilerStat::TotalTime))
					.DefaultLabel(SProfilerStatRow::GetStatText(EBlueprintProfilerStat::TotalTime))
					.ManualWidth(80)
					+SHeaderRow::Column(SProfilerStatRow::GetStatName(EBlueprintProfilerStat::Samples))
					.DefaultLabel(SProfilerStatRow::GetStatText(EBlueprintProfilerStat::Samples))
					.ManualWidth(60)
				)
			]
		]
	];
}

void SGraphExecutionStatDisplay::OnGraphLayoutChanged(TWeakObjectPtr<UBlueprint> Blueprint)
{
	if (CurrentBlueprint == Blueprint)
	{
		// Force a stat update.
		DisplayOptions->SetStateModified();
	}
}

void SGraphExecutionStatDisplay::OnToggleProfiler(bool bEnabled)
{
	// Force a stat update.
	DisplayOptions->SetStateModified();
}

TSharedRef<ITableRow> SGraphExecutionStatDisplay::OnGenerateRow(FBPStatWidgetPtr InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SProfilerStatRow, OwnerTable, InItem);
}

void SGraphExecutionStatDisplay::OnGetChildren(FBPStatWidgetPtr InParent, TArray<FBPStatWidgetPtr>& OutChildren)
{
	if (InParent.IsValid())
	{
		InParent->GatherChildren(OutChildren);
	}
}

void SGraphExecutionStatDisplay::OnDoubleClickStatistic(FBPStatWidgetPtr Item)
{
	if (Item.IsValid())
	{
		Item->ExpandWidgetState(ExecutionStatTree, !Item->GetExpansionState());
	}
}

void SGraphExecutionStatDisplay::OnStatisticExpansionChanged(FBPStatWidgetPtr Item, bool bExpanded)
{
	if (Item.IsValid() && !DisplayOptions->HasFlags(FBlueprintProfilerStatOptions::AutoExpand))
	{
		Item->SetExpansionState(bExpanded);
	}
}

void SGraphExecutionStatDisplay::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (GetDefault<UEditorExperimentalSettings>()->bBlueprintPerformanceAnalysisTools)
	{
		if (IBlueprintProfilerInterface* Profiler = FModuleManager::GetModulePtr<IBlueprintProfilerInterface>("BlueprintProfiler"))
		{
			// Find and process execution path timing data.
			if (BlueprintEditor.IsValid())
			{
				const TArray<UObject*>* Blueprints = BlueprintEditor.Pin()->GetObjectsCurrentlyBeingEdited();

				if (Blueprints->Num() == 1)
				{
					UBlueprint* NewBlueprint = Cast<UBlueprint>((*Blueprints)[0]);
					UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(NewBlueprint->GeneratedClass);
					TWeakObjectPtr<const UObject> NewInstance = NewBlueprint->GetObjectBeingDebugged();
					TSharedPtr<FBlueprintExecutionContext> BlueprintExecContext = Profiler->GetBlueprintContext(BPGC->GetPathName());

					if (BlueprintExecContext.IsValid())
					{
						// Check blueprint
						if (CurrentBlueprint.Get() != NewBlueprint)
						{
							CurrentBlueprint = NewBlueprint;
							DisplayOptions->SetStateModified();
						}
						// Find Current instance
						FName NewInstancePath = BlueprintExecContext->RemapInstancePath(FName(*NewInstance->GetPathName()));
						DisplayOptions->SetActiveInstance(NewInstancePath);
						// Find active graph to filter on
						TSharedPtr<SDockTab> DockTab = BlueprintEditor.Pin()->DocumentManager->GetActiveTab();
						if (DockTab.IsValid())
						{
							TSharedRef<SGraphEditor> ActiveGraphEditor = StaticCastSharedRef<SGraphEditor>(DockTab->GetContent());
							const UEdGraph* CurrentGraph = ActiveGraphEditor->GetCurrentGraph();
							DisplayOptions->SetActiveGraph(CurrentGraph ? CurrentGraph->GetFName() : NAME_None);
						}
						if (DisplayOptions->IsStateModified())
						{
							TSharedPtr<FScriptExecutionBlueprint> BlueprintExecNode = BlueprintExecContext->GetBlueprintExecNode();
							RootTreeItems.Reset(0);

							if (BlueprintExecNode.IsValid())
							{
								// Build Instance widget execution trees
								DisplayOptions->ClearFlags(FBlueprintProfilerStatOptions::Modified);
								// Cache Active blueprint and Instance
								const FName CurrentInstancePath = DisplayOptions->GetActiveInstance();

								if (DisplayOptions->HasFlags(FBlueprintProfilerStatOptions::DisplayByInstance))
								{
									if (DisplayOptions->HasFlags(FBlueprintProfilerStatOptions::ScopeToDebugInstance))
									{
										if (CurrentInstancePath != NAME_None)
										{
											TSharedPtr<FScriptExecutionNode> InstanceStat = BlueprintExecNode->GetInstanceByName(CurrentInstancePath);
											if (InstanceStat.IsValid())
											{
												FTracePath InstanceTracePath;
												DisplayOptions->SetActiveInstance(InstanceStat->GetName());
												FBPStatWidgetPtr InstanceWidget = MakeShareable<FBPProfilerStatWidget>(new FBPProfilerStatWidget(InstanceStat, InstanceTracePath));
												InstanceWidget->GenerateExecNodeWidgets(DisplayOptions);
												RootTreeItems.Add(InstanceWidget);
											}
										}
									}
									else
									{
										for (int32 InstanceIdx = 0; InstanceIdx < BlueprintExecNode->GetInstanceCount(); ++InstanceIdx)
										{
											TSharedPtr<FScriptExecutionNode> InstanceStat = BlueprintExecNode->GetInstanceByIndex(InstanceIdx);
											FTracePath InstanceTracePath;
											DisplayOptions->SetActiveInstance(InstanceStat->GetName());
											FBPStatWidgetPtr InstanceWidget = MakeShareable<FBPProfilerStatWidget>(new FBPProfilerStatWidget(InstanceStat, InstanceTracePath));
											InstanceWidget->GenerateExecNodeWidgets(DisplayOptions);
											RootTreeItems.Add(InstanceWidget);
										}
									}
								}
								else
								{
									FTracePath TracePath;
									FBPStatWidgetPtr BlueprintWidget = MakeShareable<FBPProfilerStatWidget>(new FBPProfilerStatWidget(BlueprintExecNode, TracePath));
									BlueprintWidget->GenerateExecNodeWidgets(DisplayOptions);
									RootTreeItems.Add(BlueprintWidget);
								}
							}
							// Refresh Tree
							if (ExecutionStatTree.IsValid())
							{
								ExecutionStatTree->RequestTreeRefresh();
								if (DisplayOptions->HasFlags(FBlueprintProfilerStatOptions::AutoExpand))
								{
									for (auto Iter : RootTreeItems)
									{
										Iter->ExpandWidgetState(ExecutionStatTree, true);
									}
								}
								else
								{
									for (auto Iter : RootTreeItems)
									{
										Iter->ProbeChildWidgetExpansionStates();
										Iter->RestoreWidgetExpansionState(ExecutionStatTree);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
