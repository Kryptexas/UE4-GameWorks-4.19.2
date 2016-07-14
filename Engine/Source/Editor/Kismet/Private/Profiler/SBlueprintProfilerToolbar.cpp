// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "SBlueprintProfilerToolbar.h"
//#include "BPProfilerStatisticWidgets.h"
#include "EventExecution.h"
#include "SNumericEntryBox.h"
#include "Editor/UnrealEd/Classes/Settings/EditorExperimentalSettings.h"

#define LOCTEXT_NAMESPACE "BlueprintProfilerToolbar"

//////////////////////////////////////////////////////////////////////////
// FBPProfilerStatOptions

void FBlueprintProfilerStatOptions::SetActiveInstance(const FName InstanceName)
{
	if (ActiveInstance != InstanceName && HasFlags(ScopeToDebugInstance))
	{
		Flags |= Modified;
	}
	ActiveInstance = InstanceName;
}

void FBlueprintProfilerStatOptions::SetActiveGraph(const FName GraphName)
{
	if (ActiveGraph != GraphName && HasFlags(GraphFilter))
	{
		Flags |= Modified;
	}
	ActiveGraph = GraphName;
}

TSharedRef<SWidget> FBlueprintProfilerStatOptions::CreateToolbar()
{
	return	
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.HAlign(HAlign_Right)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5,0))
			[
				SNew(SCheckBox)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("FilterToGraph", "Filter to Graph"))
				]
				.IsChecked<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::GetChecked, GraphFilter)
				.OnCheckStateChanged<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::OnChecked, GraphFilter)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5,0))
			[
				SNew(SCheckBox)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DisplayPureStats", "Pure Timings"))
				]
				.IsChecked<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::GetChecked, DisplayPure)
				.OnCheckStateChanged<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::OnChecked, DisplayPure)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5,0))
			[
				SNew(SCheckBox)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ShowInstancesCheck", "Show Instances"))
				]
				.IsChecked<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::GetChecked, DisplayByInstance)
				.OnCheckStateChanged<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::OnChecked, DisplayByInstance)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5,0))
			[
				SNew(SCheckBox)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("InstanceFilterCheck", "Debug Filter Scope"))
				]
				.IsChecked<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::GetChecked, ScopeToDebugInstance)
				.OnCheckStateChanged<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::OnChecked, ScopeToDebugInstance)
			]
			//+SHorizontalBox::Slot()
			//.AutoWidth()
			//.Padding(FMargin(5,0))
			//[
			//	SNew(SCheckBox)
			//	.Content()
			//	[
			//		SNew(STextBlock)
			//		.Text(LOCTEXT("AutoItemExpansion", "Auto Expand Statistics"))
			//	]
			//	.IsChecked<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::GetChecked, AutoExpand)
			//	.OnCheckStateChanged<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::OnChecked, AutoExpand)
			//]
		];
}

TSharedRef<SWidget> FBlueprintProfilerStatOptions::CreateComboContent()
{
	return	
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.HAlign(HAlign_Left)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5,2))
			[
				SNew(SCheckBox)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("FilterToGraph_Label", "Filter to Graph"))
					.ToolTipText(LOCTEXT("FilterToGraph_Tooltip", "Only Display events that are applicable to the current graph."))
				]
				.IsChecked<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::GetChecked, GraphFilter)
				.OnCheckStateChanged<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::OnChecked, GraphFilter)
			]
		]
		+SVerticalBox::Slot()
		.HAlign(HAlign_Left)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5,2))
			[
				SNew(SCheckBox)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AverageBlueprintStats_Label", "Use Averaged Blueprint Stats"))
					.ToolTipText(LOCTEXT("AverageBlueprintStats_Tooltip", "Blueprint Statistics are Averaged or Summed By Instance."))
				]
				.IsChecked<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::GetChecked, AverageBlueprintStats)
				.OnCheckStateChanged<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::OnChecked, AverageBlueprintStats)
			]
		]
		+SVerticalBox::Slot()
		.HAlign(HAlign_Left)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5,2))
			[
				SNew(SCheckBox)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DisplayPureStats_Label", "Pure Timings"))
					.ToolTipText(LOCTEXT("DisplayPureStats_Tooltip", "Display timings from pure nodes."))
				]
				.IsChecked<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::GetChecked, DisplayPure)
				.OnCheckStateChanged<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::OnChecked, DisplayPure)
			]
		]
		+SVerticalBox::Slot()
		.HAlign(HAlign_Left)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5,2))
			[
				SNew(SCheckBox)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ShowInstancesCheck_Label", "Show Instances"))
					.ToolTipText(LOCTEXT("ShowInstancesCheck_Tooltip", "Display statistics by instance rather than by blueprint."))
				]
				.IsChecked<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::GetChecked, DisplayByInstance)
				.OnCheckStateChanged<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::OnChecked, DisplayByInstance)
			]
		]
		+SVerticalBox::Slot()
		.HAlign(HAlign_Left)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			.Visibility<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::GetVisibility, ScopeToDebugInstance)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5,2))
			[
				SNew(SCheckBox)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("InstanceFilterCheck_Label", "Debug Filter Scope"))
					.ToolTipText(LOCTEXT("InstanceFilterCheck_Tooltip", "Restrict the displayed instances to match the blueprint debug filter."))
				]
				.IsChecked<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::GetChecked, ScopeToDebugInstance)
				.OnCheckStateChanged<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::OnChecked, ScopeToDebugInstance)
			]
#if 0
		]
		+SVerticalBox::Slot()
		.HAlign(HAlign_Left)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5,2))
			[
				SNew(SCheckBox)
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AutoItemExpansion_Label", "Auto Expand Statistics"))
					.ToolTipText(LOCTEXT("AutoItemExpansion_Tooltip", "Auto expand the statistic trees."))
				]
				.IsChecked<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::GetChecked, AutoExpand)
				.OnCheckStateChanged<FBlueprintProfilerStatOptions, uint32>(this, &FBlueprintProfilerStatOptions::OnChecked, AutoExpand)
			]
#endif
		];
}

ECheckBoxState FBlueprintProfilerStatOptions::GetChecked(const uint32 FlagsIn) const
{
	ECheckBoxState CheckedState;
	if (FlagsIn & ScopeToDebugInstance)
	{
		if (HasFlags(DisplayByInstance))
		{
			CheckedState = HasFlags(ScopeToDebugInstance) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		}
		else
		{
			CheckedState = ECheckBoxState::Undetermined;
		}
	}
	else
	{
		CheckedState = HasAllFlags(FlagsIn) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	return CheckedState;
}

void FBlueprintProfilerStatOptions::OnChecked(ECheckBoxState NewState, const uint32 FlagsIn)
{
	if (NewState == ECheckBoxState::Checked)
	{
		Flags |= FlagsIn;
	}
	else
	{
		Flags &= ~FlagsIn;
	}
	if (FlagsIn & AverageBlueprintStats)
	{
		FScriptPerfData::EnableBlueprintStatAverage(NewState == ECheckBoxState::Checked);
	}
	Flags |= Modified;
}

EVisibility FBlueprintProfilerStatOptions::GetVisibility(const uint32 FlagsIn) const
{
	EVisibility Result = EVisibility::Hidden;
	if (FlagsIn & ScopeToDebugInstance)
	{
		Result = (Flags & DisplayByInstance) != 0 ? EVisibility::Visible : EVisibility::Collapsed;
	}
	return Result;
}

bool FBlueprintProfilerStatOptions::IsFiltered(TSharedPtr<FScriptExecutionNode> Node) const
{
	bool bFilteredOut = !HasFlags(EDisplayFlags::DisplayPure) && Node->HasFlags(EScriptExecutionNodeFlags::PureStats);
	if (Node->IsEvent() && HasFlags(EDisplayFlags::GraphFilter))
	{
		if (Node->GetGraphName() == UEdGraphSchema_K2::FN_UserConstructionScript)
		{
			bFilteredOut = ActiveGraph != UEdGraphSchema_K2::FN_UserConstructionScript;
		}
		else
		{
			bFilteredOut = ActiveGraph == UEdGraphSchema_K2::FN_UserConstructionScript;
		}
	}
	return bFilteredOut;
}

//////////////////////////////////////////////////////////////////////////
// SBlueprintProfilerToolbar

void SBlueprintProfilerToolbar::Construct(const FArguments& InArgs)
{
	CurrentViewType = InArgs._ProfileViewType;
	DisplayOptions = InArgs._DisplayOptions;
	ViewChangeDelegate = InArgs._OnViewChanged;

	FMenuBuilder ViewComboContent(true, NULL);
	ViewComboContent.AddMenuEntry(	LOCTEXT("OverviewViewType", "Profiler Overview"), 
									LOCTEXT("OverviewViewTypeDesc", "Displays High Level Profiling Information"), 
									FSlateIcon(), 
									FExecuteAction::CreateSP(this, &SBlueprintProfilerToolbar::OnViewSelectionChanged, EBlueprintPerfViewType::Overview),
									NAME_None, 
									EUserInterfaceActionType::Button);
	ViewComboContent.AddMenuEntry(	LOCTEXT("ExecutionGraphViewType", "Execution Graph"), 
									LOCTEXT("ExecutionGraphViewTypeDesc", "Displays the Execution Graph with Statistics"), 
									FSlateIcon(), 
									FExecuteAction::CreateSP(this, &SBlueprintProfilerToolbar::OnViewSelectionChanged, EBlueprintPerfViewType::ExecutionGraph),
									NAME_None, 
									EUserInterfaceActionType::Button);
	ViewComboContent.AddMenuEntry(	LOCTEXT("LeastPerformantViewType", "Least Performant List"), 
									LOCTEXT("LeastPerformantViewTypeDesc", "Displays a list of Least Performant Areas of the Blueprint"), 
									FSlateIcon(), 
									FExecuteAction::CreateSP(this, &SBlueprintProfilerToolbar::OnViewSelectionChanged, EBlueprintPerfViewType::LeastPerformant),
									NAME_None, 
									EUserInterfaceActionType::Button);

	FMenuBuilder HeatMapDisplayModeComboContent(true, nullptr);
	HeatMapDisplayModeComboContent.AddMenuEntry(LOCTEXT("HeatMapDisplayMode_None", "Off"),
		LOCTEXT("HeatMapDisplayMode_None_Desc", "Normal Display (No Heat Map)"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SBlueprintProfilerToolbar::OnHeatMapDisplayModeChanged, EHeatmapControlId::NodeHeatmap, EBlueprintProfilerHeatMapDisplayMode::None),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SBlueprintProfilerToolbar::IsHeatMapDisplayModeSelected, EHeatmapControlId::NodeHeatmap, EBlueprintProfilerHeatMapDisplayMode::None)
		),
		NAME_None,
		EUserInterfaceActionType::Check);
	HeatMapDisplayModeComboContent.AddMenuEntry(LOCTEXT("HeatMapDisplayMode_Exclusive", "Exclusive"),
		LOCTEXT("HeatMapDisplayMode_Exclusive_Desc", "Heat Map - Exclusive Timings"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SBlueprintProfilerToolbar::OnHeatMapDisplayModeChanged, EHeatmapControlId::NodeHeatmap, EBlueprintProfilerHeatMapDisplayMode::Exclusive),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SBlueprintProfilerToolbar::IsHeatMapDisplayModeSelected, EHeatmapControlId::NodeHeatmap, EBlueprintProfilerHeatMapDisplayMode::Exclusive)
		),
		NAME_None,
		EUserInterfaceActionType::Check);
	HeatMapDisplayModeComboContent.AddMenuEntry(LOCTEXT("HeatMapDisplayMode_Inclusive", "Inclusive"),
		LOCTEXT("HeatMapDisplayMode_Inclusive_Desc", "Heat Map - Inclusive Timings"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SBlueprintProfilerToolbar::OnHeatMapDisplayModeChanged, EHeatmapControlId::NodeHeatmap, EBlueprintProfilerHeatMapDisplayMode::Inclusive),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SBlueprintProfilerToolbar::IsHeatMapDisplayModeSelected, EHeatmapControlId::NodeHeatmap, EBlueprintProfilerHeatMapDisplayMode::Inclusive)
		),
		NAME_None,
		EUserInterfaceActionType::Check);
	HeatMapDisplayModeComboContent.AddMenuEntry(LOCTEXT("HeatMapDisplayMode_MaxTiming", "Max Time"),
		LOCTEXT("HeatMapDisplayMode_MaxTiming_Desc", "Heat Map - Max Time"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SBlueprintProfilerToolbar::OnHeatMapDisplayModeChanged, EHeatmapControlId::NodeHeatmap, EBlueprintProfilerHeatMapDisplayMode::MaxTiming),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SBlueprintProfilerToolbar::IsHeatMapDisplayModeSelected, EHeatmapControlId::NodeHeatmap, EBlueprintProfilerHeatMapDisplayMode::MaxTiming)
		),
		NAME_None,
		EUserInterfaceActionType::Check);
	HeatMapDisplayModeComboContent.AddMenuEntry(LOCTEXT("HeatMapDisplayMode_TotalTiming", "Total Time"),
		LOCTEXT("HeatMapDisplayMode_TotalTiming_Desc", "Heat Map - Max Time"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SBlueprintProfilerToolbar::OnHeatMapDisplayModeChanged, EHeatmapControlId::NodeHeatmap, EBlueprintProfilerHeatMapDisplayMode::Total),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SBlueprintProfilerToolbar::IsHeatMapDisplayModeSelected, EHeatmapControlId::NodeHeatmap, EBlueprintProfilerHeatMapDisplayMode::Total)
		),
		NAME_None,
		EUserInterfaceActionType::Check);

	FMenuBuilder WireHeatMapDisplayModeComboContent(true, nullptr);
	WireHeatMapDisplayModeComboContent.AddMenuEntry(LOCTEXT("WireHeatMapDisplayMode_None", "Off"),
		LOCTEXT("HeatMapDisplayMode_None_Desc", "Normal Display (No Heat Map)"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SBlueprintProfilerToolbar::OnHeatMapDisplayModeChanged, EHeatmapControlId::WireHeatmap, EBlueprintProfilerHeatMapDisplayMode::None),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SBlueprintProfilerToolbar::IsHeatMapDisplayModeSelected, EHeatmapControlId::WireHeatmap, EBlueprintProfilerHeatMapDisplayMode::None)
		),
		NAME_None,
		EUserInterfaceActionType::Check);
	WireHeatMapDisplayModeComboContent.AddMenuEntry(LOCTEXT("WireHeatMapDisplayMode_HottestPath", "Hottest Path"),
		LOCTEXT("WireHeatMapDisplayMode_HottestPath_Desc", "Heat Map - Hottest Path"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SBlueprintProfilerToolbar::OnHeatMapDisplayModeChanged, EHeatmapControlId::WireHeatmap, EBlueprintProfilerHeatMapDisplayMode::HottestPath),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SBlueprintProfilerToolbar::IsHeatMapDisplayModeSelected, EHeatmapControlId::WireHeatmap, EBlueprintProfilerHeatMapDisplayMode::HottestPath)
		),
		NAME_None,
		EUserInterfaceActionType::Check);
	WireHeatMapDisplayModeComboContent.AddMenuEntry(LOCTEXT("WireHeatMapDisplayMode_HottestEndpoint", "Hottest Endpoints"),
		LOCTEXT("WireHeatMapDisplayMode_HottestEndpoints_Desc", "Heat Map - Hottest Endpoints"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SBlueprintProfilerToolbar::OnHeatMapDisplayModeChanged, EHeatmapControlId::WireHeatmap, EBlueprintProfilerHeatMapDisplayMode::HottestEndpoint),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SBlueprintProfilerToolbar::IsHeatMapDisplayModeSelected, EHeatmapControlId::WireHeatmap, EBlueprintProfilerHeatMapDisplayMode::HottestEndpoint)
		),
		NAME_None,
		EUserInterfaceActionType::Check);
	WireHeatMapDisplayModeComboContent.AddMenuEntry(LOCTEXT("WireHeatMapDisplayMode_PinToPin", "Pin To Pin Heatmap"),
		LOCTEXT("WireHeatMapDisplayMode_PinToPin_Desc", "Heat Map - Pin To Pin"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SBlueprintProfilerToolbar::OnHeatMapDisplayModeChanged, EHeatmapControlId::WireHeatmap, EBlueprintProfilerHeatMapDisplayMode::PinToPin),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SBlueprintProfilerToolbar::IsHeatMapDisplayModeSelected, EHeatmapControlId::WireHeatmap, EBlueprintProfilerHeatMapDisplayMode::PinToPin)
		),
		NAME_None,
		EUserInterfaceActionType::Check);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("BlueprintProfiler.ViewToolBar"))
		.Padding(0)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Right)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.AutoWidth()
			.Padding(FMargin(5, 0))
			[
				SAssignNew(GeneralOptionsComboButton, SComboButton)
				.ForegroundColor(this, &SBlueprintProfilerToolbar::GetButtonForegroundColor, EHeatmapControlId::GeneralOptions)
				.ToolTipText(LOCTEXT("GeneralOptions_Tooltip", "General Options"))
				.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
				.ContentPadding(2)
				.OnGetMenuContent(DisplayOptions.Get(), &FBlueprintProfilerStatOptions::CreateComboContent)
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("GeneralOptions_Label", "General Options"))
					.ColorAndOpacity(FLinearColor::White)
				]
			]
			+SHorizontalBox::Slot()
			.Padding(FMargin(5, 0))
			[
				SNew(SSeparator)
				.Orientation(Orient_Vertical)
			]
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.AutoWidth()
			.Padding(FMargin(5, 0))
			[
				SAssignNew(HeatThresholdComboButton, SComboButton)
				.ForegroundColor(this, &SBlueprintProfilerToolbar::GetButtonForegroundColor, EHeatmapControlId::HeatThresholds)
				.ToolTipText(LOCTEXT("HeatMapDisplayMode_Tooltip", "Heat Map Display Mode"))
				.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
				.ContentPadding(2)
				.OnGetMenuContent(this, &SBlueprintProfilerToolbar::CreateHeatThresholdsMenuContent)
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("HeatmapThresholdsCombo_Label", "Heatmap Thresholds"))
					.ColorAndOpacity(FLinearColor::White)
				]
			]
			+SHorizontalBox::Slot()
			.Padding(FMargin(5, 0))
			[
				SNew(SSeparator)
				.Orientation(Orient_Vertical)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5, 0))
			[
				SAssignNew(HeatMapDisplayModeComboButton, SComboButton)
				.ForegroundColor(this, &SBlueprintProfilerToolbar::GetButtonForegroundColor, EHeatmapControlId::NodeHeatmap)
				.ToolTipText(LOCTEXT("HeatMapDisplayMode_Tooltip", "Heat Map Display Mode"))
				.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
				.ContentPadding(2)
				.MenuContent()
				[
					HeatMapDisplayModeComboContent.MakeWidget()
				]
				.ButtonContent()
				[
					CreateHeatMapDisplayModeButton(EHeatmapControlId::NodeHeatmap)
				]
			]
			+SHorizontalBox::Slot()
			.Padding(FMargin(5, 0))
			[
				SNew(SSeparator)
				.Orientation(Orient_Vertical)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5, 0))
			[
				SAssignNew(WireHeatMapDisplayModeComboButton, SComboButton)
				.ForegroundColor(this, &SBlueprintProfilerToolbar::GetButtonForegroundColor, EHeatmapControlId::WireHeatmap)
				.ToolTipText(LOCTEXT("WireHeatMapDisplayMode_Tooltip", "Wire Heat Map Display Mode"))
				.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
				.ContentPadding(2)
				.MenuContent()
				[
					WireHeatMapDisplayModeComboContent.MakeWidget()
				]
				.ButtonContent()
				[
					CreateHeatMapDisplayModeButton(EHeatmapControlId::WireHeatmap)
				]
			]
#if 0
			+SHorizontalBox::Slot()
			.Padding(FMargin(5, 0))
			[
				SNew(SSeparator)
				.Orientation(Orient_Vertical)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(5,0))
			[
				SAssignNew(ViewComboButton, SComboButton)
				.ForegroundColor(this, &SBlueprintProfilerToolbar::GetButtonForegroundColor, EHeatmapControlId::ViewType)
				.ToolTipText(LOCTEXT("ViewType", "View Type"))
				.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
				.ContentPadding(2)
				.MenuContent()
				[
					ViewComboContent.MakeWidget()
				]
				.ButtonContent()
				[
					CreateViewButton()
				]
			]
#endif
		]
	];
}

void SBlueprintProfilerToolbar::OnViewSelectionChanged(const EBlueprintPerfViewType::Type NewViewType)
{
	if (CurrentViewType != NewViewType)
	{
		CurrentViewType = NewViewType;
		ViewChangeDelegate.ExecuteIfBound();
	}
}

TSharedRef<SWidget> SBlueprintProfilerToolbar::CreateViewButton() const
{
	return SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Top)
		.Padding(0)
		[
			SNew(SImage)
			.Image( FEditorStyle::GetBrush("GenericViewButton") )
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2,0)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ProfilerViewLabel", "Current View: "))
			.ColorAndOpacity(FLinearColor::White)
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2,0)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(GetCurrentViewText())
			.ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.4f))
		];
}

FText SBlueprintProfilerToolbar::GetCurrentViewText() const
{
	switch(CurrentViewType)
	{
		case EBlueprintPerfViewType::Overview:			return LOCTEXT("OverviewLabel", "Profiler Overview");
		case EBlueprintPerfViewType::ExecutionGraph:	return LOCTEXT("ExecutionGraphLabel", "Execution Graph");
		case EBlueprintPerfViewType::LeastPerformant:	return LOCTEXT("LeastPerformantLabel", "Least Performant");
	}
	return LOCTEXT("UnknownViewLabel", "Unknown ViewType");
}

FText SBlueprintProfilerToolbar::GetCurrentHeatMapDisplayModeText(const EHeatmapControlId::Type ControlId) const
{
	FText Result = LOCTEXT("HeatMapDisplayModeLabel_Unknown", "<unknown>");
	IBlueprintProfilerInterface& ProfilerModule = FModuleManager::LoadModuleChecked<IBlueprintProfilerInterface>("BlueprintProfiler");
	EBlueprintProfilerHeatMapDisplayMode::Type DisplayMode = ControlId == EHeatmapControlId::NodeHeatmap ? ProfilerModule.GetGraphNodeHeatMapDisplayMode() : ProfilerModule.GetWireHeatMapDisplayMode();
	switch (DisplayMode)
	{
		case EBlueprintProfilerHeatMapDisplayMode::None:			Result = LOCTEXT("HeatMapDisplayModeLabel_None", "Off"); break;
		case EBlueprintProfilerHeatMapDisplayMode::Exclusive:		Result = LOCTEXT("HeatMapDisplayModeLabel_Exclusive", "Exclusive"); break;
		case EBlueprintProfilerHeatMapDisplayMode::Inclusive:		Result = LOCTEXT("HeatMapDisplayModeLabel_Inclusive", "Inclusive"); break;
		case EBlueprintProfilerHeatMapDisplayMode::MaxTiming:		Result = LOCTEXT("HeatMapDisplayModeLabel_MaxTiming", "Max Timing"); break;
		case EBlueprintProfilerHeatMapDisplayMode::Total:			Result = LOCTEXT("HeatMapDisplayModeLabel_TotalTiming", "Total Timing"); break;
		case EBlueprintProfilerHeatMapDisplayMode::HottestPath:		Result = LOCTEXT("HeatMapDisplayModeLabel_HottestPath", "Hottest Path"); break;
		case EBlueprintProfilerHeatMapDisplayMode::HottestEndpoint:	Result = LOCTEXT("HeatMapDisplayModeLabel_HottestEndpoint", "Hottest Endpoints"); break;
		case EBlueprintProfilerHeatMapDisplayMode::PinToPin:		Result = LOCTEXT("HeatMapDisplayModeLabel_PinToPin", "Pin to Pin Heatmap"); break;
	}
	return Result;
}

TSharedRef<SWidget> SBlueprintProfilerToolbar::CreateHeatMapDisplayModeButton(const EHeatmapControlId::Type ControlId) const
{
	return SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Left)
		.Padding(0,0)
		.AutoWidth()
		[
			SNew(STextBlock)
			.Text(ControlId == EHeatmapControlId::NodeHeatmap ? LOCTEXT("NodeHeatMapLabel_None", "Node Heatmap: ") : LOCTEXT("WireHeatMapLabel_None", "Wire Heatmap: "))
			.ColorAndOpacity(FLinearColor::White)
		]
		+SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Left)
		.Padding(2,0)
		.AutoWidth()
		[
			SNew(STextBlock)
			.Text(this, &SBlueprintProfilerToolbar::GetCurrentHeatMapDisplayModeText, ControlId)
			.ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.4f))
		];
}

FSlateColor SBlueprintProfilerToolbar::GetButtonForegroundColor(const EHeatmapControlId::Type ControlId) const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");
	bool bButtonHovered = false;
	switch (ControlId)
	{
		case EHeatmapControlId::ViewType:			bButtonHovered = ViewComboButton->IsHovered(); break;
		case EHeatmapControlId::GeneralOptions:		bButtonHovered = GeneralOptionsComboButton->IsHovered(); break;
		case EHeatmapControlId::NodeHeatmap:		bButtonHovered = HeatMapDisplayModeComboButton->IsHovered(); break;
		case EHeatmapControlId::WireHeatmap:		bButtonHovered = WireHeatMapDisplayModeComboButton->IsHovered(); break;
		case EHeatmapControlId::HeatThresholds:		bButtonHovered = HeatThresholdComboButton->IsHovered();	break;
	}
	return bButtonHovered ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
}

bool SBlueprintProfilerToolbar::IsHeatMapDisplayModeSelected(const EHeatmapControlId::Type ControlId, const EBlueprintProfilerHeatMapDisplayMode::Type InHeatMapDisplayMode) const
{
	IBlueprintProfilerInterface& ProfilerModule = FModuleManager::LoadModuleChecked<IBlueprintProfilerInterface>("BlueprintProfiler");
	return ControlId == EHeatmapControlId::NodeHeatmap ? (ProfilerModule.GetGraphNodeHeatMapDisplayMode() == InHeatMapDisplayMode) :
														 (ProfilerModule.GetWireHeatMapDisplayMode() == InHeatMapDisplayMode);
}

void SBlueprintProfilerToolbar::OnHeatMapDisplayModeChanged(const EHeatmapControlId::Type ControlId, const EBlueprintProfilerHeatMapDisplayMode::Type NewHeatMapDisplayMode)
{
	IBlueprintProfilerInterface& ProfilerModule = FModuleManager::LoadModuleChecked<IBlueprintProfilerInterface>("BlueprintProfiler");
	if (ControlId == EHeatmapControlId::NodeHeatmap)
	{
		if (ProfilerModule.GetGraphNodeHeatMapDisplayMode() != NewHeatMapDisplayMode)
		{
			ProfilerModule.SetGraphNodeHeatMapDisplayMode(NewHeatMapDisplayMode);
		}
	}
	else 
	{
		if (ProfilerModule.GetWireHeatMapDisplayMode() != NewHeatMapDisplayMode)
		{
			ProfilerModule.SetWireHeatMapDisplayMode(NewHeatMapDisplayMode);
		}
	}
}

TSharedRef<SWidget> SBlueprintProfilerToolbar::CreateHeatThresholdsMenuContent()
{
	return SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.Padding(FMargin(0,2))
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(4,1,4,1))
			[
				SNew(STextBlock)
				.Text(LOCTEXT("EventPerformanceThreshold_Label", "Event Heat Threshold"))
				.ToolTipText(LOCTEXT("EventPerformanceThreshold_Tooltip", "The Hot Threshold for Overall Event Timings"))
			]
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.Padding(FMargin(4,1,1,1))
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SNew(SNumericEntryBox<float>)
					.AllowSpin(true)
					.MinValue(0.01f)
					.MaxValue(5.f)
					.MinSliderValue(0.01f)
					.MaxSliderValue(5.f)
					.Value(this, &SBlueprintProfilerToolbar::GetEventThreshold)
					.OnValueChanged(this, &SBlueprintProfilerToolbar::SetEventThreshold)
					.MinDesiredValueWidth(100)
					.ToolTipText(LOCTEXT("EventPerformanceThreshold_Tooltip", "The Hot Threshold for Overall Event Timings"))
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.ToolTipText(LOCTEXT("ResetToDefaultToolTip", "Reset to Default"))
					.ButtonStyle(FEditorStyle::Get(), "NoBorder")
					.ContentPadding(4)
					.Visibility(this, &SBlueprintProfilerToolbar::IsEventDefaultButtonVisible)
					.OnClicked(this, &SBlueprintProfilerToolbar::ResetEventThreshold)
					.Content()
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
					]
				]
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.Padding(FMargin(0,2))
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(4,1,4,1))
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NodeAveragePerformanceThreshold_Label", "Node Average Heat Threshold"))
				.ToolTipText(LOCTEXT("NodeAveragePerformanceThreshold_Tooltip", "The Hot Threshold for Node Average Timings"))
			]
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.Padding(FMargin(4,1,1,1))
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SNew(SNumericEntryBox<float>)
					.AllowSpin(true)
					.MinValue(0.001f)
					.MaxValue(0.5f)
					.MinSliderValue(0.001f)
					.MaxSliderValue(0.5f)
					.Value(this, &SBlueprintProfilerToolbar::GetNodeExclusiveThreshold)
					.OnValueChanged(this, &SBlueprintProfilerToolbar::SetNodeExclusiveThreshold)
					.MinDesiredValueWidth(100)
					.ToolTipText(LOCTEXT("NodeAveragePerformanceThreshold_Tooltip", "The Hot Threshold for Node Average Timings"))
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.ToolTipText(LOCTEXT("ResetToDefaultToolTip", "Reset to Default"))
					.ButtonStyle(FEditorStyle::Get(), "NoBorder")
					.ContentPadding(4)
					.Visibility(this, &SBlueprintProfilerToolbar::IsNodeExclDefaultButtonVisible)
					.OnClicked(this, &SBlueprintProfilerToolbar::ResetNodeExclThreshold)
					.Content()
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
					]
				]
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.Padding(FMargin(0,2))
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(4,1,4,1))
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NodeInclusivePerformanceThreshold_Label", "Node Inclusive Heat Threshold"))
				.ToolTipText(LOCTEXT("NodeInclusivePerformanceThreshold_Tooltip", "The Hot Threshold for Node Inclusive Timings"))
			]
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.Padding(1)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SNew(SNumericEntryBox<float>)
					.AllowSpin(true)
					.MinValue(0.001f)
					.MaxValue(0.5f)
					.MinSliderValue(0.001f)
					.MaxSliderValue(0.5f)
					.Value(this, &SBlueprintProfilerToolbar::GetNodeInclusiveThreshold)
					.OnValueChanged(this, &SBlueprintProfilerToolbar::SetNodeInclusiveThreshold)
					.MinDesiredValueWidth(100)
					.ToolTipText(LOCTEXT("NodeInclusivePerformanceThreshold_Tooltip", "The Hot Threshold for Node Inclusive Timings"))
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.ToolTipText(LOCTEXT("ResetToDefaultToolTip", "Reset to Default"))
					.ButtonStyle(FEditorStyle::Get(), "NoBorder")
					.ContentPadding(4)
					.Visibility(this, &SBlueprintProfilerToolbar::IsNodeInclDefaultButtonVisible)
					.OnClicked(this, &SBlueprintProfilerToolbar::ResetNodeInclThreshold)
					.Content()
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
					]
				]
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.Padding(FMargin(0,2))
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(4,1,4,1))
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NodeMaxPerformanceThreshold_Label", "Node Max Heat Threshold"))
				.ToolTipText(LOCTEXT("NodeMaxPerformanceThreshold_Tooltip", "The Hot Threshold for Max Node Timings"))
			]
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.Padding(FMargin(4,1,1,1))
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SNew(SNumericEntryBox<float>)
					.AllowSpin(true)
					.MinValue(0.01f)
					.MaxValue(2.f)
					.MinSliderValue(0.01f)
					.MaxSliderValue(2.f)
					.Value(this, &SBlueprintProfilerToolbar::GetNodeMaxThreshold)
					.OnValueChanged(this, &SBlueprintProfilerToolbar::SetNodeMaxThreshold)
					.MinDesiredValueWidth(100)
					.ToolTipText(LOCTEXT("NodeMaxPerformanceThreshold_Tooltip", "The Hot Threshold for Max Node Timings"))
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.ToolTipText(LOCTEXT("ResetToDefaultToolTip", "Reset to Default"))
					.ButtonStyle(FEditorStyle::Get(), "NoBorder")
					.ContentPadding(4)
					.Visibility(this, &SBlueprintProfilerToolbar::IsNodeMaxDefaultButtonVisible)
					.OnClicked(this, &SBlueprintProfilerToolbar::ResetNodeMaxThreshold)
					.Content()
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
					]
				]
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.Padding(FMargin(0,2))
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(4,1,16,1))
			[
				SNew(STextBlock)
				.Text(LOCTEXT("RecentSampleCheck_Label", "Enable Recent Sample Bias"))
				.ToolTipText(LOCTEXT("RecentSampleCheck_Tooltip", "Enable a user controlled bias that controls the weighting of new samples against historical."))
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(4,1,4,1))
			[
				SNew(SCheckBox)
				.IsChecked(this, &SBlueprintProfilerToolbar::GetRecentSampleBiasChecked)
				.OnCheckStateChanged(this, &SBlueprintProfilerToolbar::OnRecentSampleBiasChecked)
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.Padding(FMargin(0,2))
		[
			SNew(SHorizontalBox)
			.Visibility(this, &SBlueprintProfilerToolbar::GetRecentSampleBiasVisibility)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(4,1,4,1))
			[
				SNew(STextBlock)
				.Text(LOCTEXT("RecentSampleValue_Label", "Recent Sample Bias"))
				.ToolTipText(LOCTEXT("RecentSampleValue_Tooltip", "Shifts the weighting of more recent samples to control the effect of sampling spikes."))
			]
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.Padding(FMargin(4,1,1,1))
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SNew(SNumericEntryBox<float>)
					.AllowSpin(true)
					.MinValue(0.01f)
					.MaxValue(1.f)
					.MinSliderValue(0.01f)
					.MaxSliderValue(1.f)
					.Value(this, &SBlueprintProfilerToolbar::GetRecentSampleBiasValue)
					.OnValueChanged(this, &SBlueprintProfilerToolbar::SetRecentSampleBiasValue)
					.MinDesiredValueWidth(100)
					.ToolTipText(LOCTEXT("RecentSampleValue_Tooltip", "Shifts the weighting of more recent samples to control the effect of sampling spikes."))
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.ToolTipText(LOCTEXT("ResetToDefaultToolTip", "Reset to Default"))
					.ButtonStyle(FEditorStyle::Get(), "NoBorder")
					.ContentPadding(4)
					.Visibility(this, &SBlueprintProfilerToolbar::IsRecentSampleBiasDefaultButtonVisible)
					.OnClicked(this, &SBlueprintProfilerToolbar::ResetRecentSampleBias)
					.Content()
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
					]
				]
			]
		];
}

EVisibility SBlueprintProfilerToolbar::GetRecentSampleBiasVisibility() const
{
	EVisibility Result = EVisibility::Collapsed;
	if (const UEditorExperimentalSettings* EditorSettings = GetDefault<UEditorExperimentalSettings>())
	{
		Result = EditorSettings->bEnableBlueprintProfilerRecentSampleBias ? EVisibility::Visible : EVisibility::Collapsed;
	}
	return Result;
}

ECheckBoxState SBlueprintProfilerToolbar::GetRecentSampleBiasChecked() const
{
	ECheckBoxState Result = ECheckBoxState::Unchecked;
	if (const UEditorExperimentalSettings* EditorSettings = GetDefault<UEditorExperimentalSettings>())
	{
		Result = EditorSettings->bEnableBlueprintProfilerRecentSampleBias ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	return Result;
}

void SBlueprintProfilerToolbar::OnRecentSampleBiasChecked(ECheckBoxState NewState)
{
	if (UEditorExperimentalSettings* EditorSettings = GetMutableDefault<UEditorExperimentalSettings>())
	{
		EditorSettings->bEnableBlueprintProfilerRecentSampleBias = NewState == ECheckBoxState::Checked;
	}
}

TOptional<float> SBlueprintProfilerToolbar::GetRecentSampleBiasValue() const
{
	TOptional<float> Result;
	if (const UEditorExperimentalSettings* EditorSettings = GetDefault<UEditorExperimentalSettings>())
	{
		Result = EditorSettings->BlueprintProfilerRecentSampleBias;
	}
	return Result;
}

void SBlueprintProfilerToolbar::SetRecentSampleBiasValue(const float NewValue)
{
	if (UEditorExperimentalSettings* EditorSettings = GetMutableDefault<UEditorExperimentalSettings>())
	{
		EditorSettings->BlueprintProfilerRecentSampleBias = NewValue;
		FScriptPerfData::SetRecentSampleBias(NewValue);
	}
}

EVisibility SBlueprintProfilerToolbar::IsRecentSampleBiasDefaultButtonVisible() const
{
	EVisibility Result = EVisibility::Hidden;
	if (UEditorExperimentalSettings* EditorSettings = GetMutableDefault<UEditorExperimentalSettings>())
	{
		Result = EditorSettings->BlueprintProfilerRecentSampleBias == 0.2f ? EVisibility::Hidden : EVisibility::Visible;
	}
	return Result;
}

FReply SBlueprintProfilerToolbar::ResetRecentSampleBias()
{
	if (UEditorExperimentalSettings* EditorSettings = GetMutableDefault<UEditorExperimentalSettings>())
	{
		EditorSettings->BlueprintProfilerRecentSampleBias = 0.2f;
		FScriptPerfData::SetRecentSampleBias(0.2f);
	}
	return FReply::Handled();
}

TOptional<float> SBlueprintProfilerToolbar::GetEventThreshold() const
{
	TOptional<float> Result;
	if (const UEditorExperimentalSettings* EditorSettings = GetDefault<UEditorExperimentalSettings>())
	{
		Result = EditorSettings->BlueprintProfilerEventThreshold;
	}
	return Result;
}

void SBlueprintProfilerToolbar::SetEventThreshold(const float NewValue)
{
	if (UEditorExperimentalSettings* EditorSettings = GetMutableDefault<UEditorExperimentalSettings>())
	{
		EditorSettings->BlueprintProfilerEventThreshold = NewValue;
		FScriptPerfData::SetEventPerformanceThreshold(NewValue);
	}
}

EVisibility SBlueprintProfilerToolbar::IsEventDefaultButtonVisible() const
{
	EVisibility Result = EVisibility::Hidden;
	if (UEditorExperimentalSettings* EditorSettings = GetMutableDefault<UEditorExperimentalSettings>())
	{
		Result = EditorSettings->BlueprintProfilerEventThreshold == 1.f ? EVisibility::Hidden : EVisibility::Visible;
	}
	return Result;
}

FReply SBlueprintProfilerToolbar::ResetEventThreshold()
{
	if (UEditorExperimentalSettings* EditorSettings = GetMutableDefault<UEditorExperimentalSettings>())
	{
		EditorSettings->BlueprintProfilerEventThreshold = 1.f;
		FScriptPerfData::SetEventPerformanceThreshold(1.f);
	}
	return FReply::Handled();
}

TOptional<float> SBlueprintProfilerToolbar::GetNodeExclusiveThreshold() const
{
	TOptional<float> Result;
	if (const UEditorExperimentalSettings* EditorSettings = GetDefault<UEditorExperimentalSettings>())
	{
		Result = EditorSettings->BlueprintProfilerExclNodeThreshold;
	}
	return Result;
}

void SBlueprintProfilerToolbar::SetNodeExclusiveThreshold(const float NewValue)
{
	if (UEditorExperimentalSettings* EditorSettings = GetMutableDefault<UEditorExperimentalSettings>())
	{
		EditorSettings->BlueprintProfilerExclNodeThreshold = NewValue;
		FScriptPerfData::SetExclusivePerformanceThreshold(NewValue);
	}
}

EVisibility SBlueprintProfilerToolbar::IsNodeExclDefaultButtonVisible() const
{
	EVisibility Result = EVisibility::Hidden;
	if (UEditorExperimentalSettings* EditorSettings = GetMutableDefault<UEditorExperimentalSettings>())
	{
		Result = EditorSettings->BlueprintProfilerExclNodeThreshold == 0.2f ? EVisibility::Hidden : EVisibility::Visible;
	}
	return Result;
}

FReply SBlueprintProfilerToolbar::ResetNodeExclThreshold()
{
	if (UEditorExperimentalSettings* EditorSettings = GetMutableDefault<UEditorExperimentalSettings>())
	{
		EditorSettings->BlueprintProfilerExclNodeThreshold = 0.2f;
		FScriptPerfData::SetExclusivePerformanceThreshold(0.2f);
	}
	return FReply::Handled();
}

TOptional<float> SBlueprintProfilerToolbar::GetNodeInclusiveThreshold() const
{
	TOptional<float> Result;
	if (const UEditorExperimentalSettings* EditorSettings = GetDefault<UEditorExperimentalSettings>())
	{
		Result = EditorSettings->BlueprintProfilerInclNodeThreshold;
	}
	return Result;
}

void SBlueprintProfilerToolbar::SetNodeInclusiveThreshold(const float NewValue)
{
	if (UEditorExperimentalSettings* EditorSettings = GetMutableDefault<UEditorExperimentalSettings>())
	{
		EditorSettings->BlueprintProfilerInclNodeThreshold = NewValue;
		FScriptPerfData::SetInclusivePerformanceThreshold(NewValue);
	}
}

EVisibility SBlueprintProfilerToolbar::IsNodeInclDefaultButtonVisible() const
{
	EVisibility Result = EVisibility::Hidden;
	if (UEditorExperimentalSettings* EditorSettings = GetMutableDefault<UEditorExperimentalSettings>())
	{
		Result = EditorSettings->BlueprintProfilerInclNodeThreshold == 0.25f ? EVisibility::Hidden : EVisibility::Visible;
	}
	return Result;
}

FReply SBlueprintProfilerToolbar::ResetNodeInclThreshold()
{
	if (UEditorExperimentalSettings* EditorSettings = GetMutableDefault<UEditorExperimentalSettings>())
	{
		EditorSettings->BlueprintProfilerInclNodeThreshold = 0.25f;
		FScriptPerfData::SetInclusivePerformanceThreshold(0.25f);
	}
	return FReply::Handled();
}

TOptional<float> SBlueprintProfilerToolbar::GetNodeMaxThreshold() const
{
	TOptional<float> Result;
	if (const UEditorExperimentalSettings* EditorSettings = GetDefault<UEditorExperimentalSettings>())
	{
		Result = EditorSettings->BlueprintProfilerMaxNodeThreshold;
	}
	return Result;
}

void SBlueprintProfilerToolbar::SetNodeMaxThreshold(const float NewValue)
{
	if (UEditorExperimentalSettings* EditorSettings = GetMutableDefault<UEditorExperimentalSettings>())
	{
		EditorSettings->BlueprintProfilerMaxNodeThreshold = NewValue;
		FScriptPerfData::SetMaxPerformanceThreshold(NewValue);
	}
}

EVisibility SBlueprintProfilerToolbar::IsNodeMaxDefaultButtonVisible() const
{
	EVisibility Result = EVisibility::Hidden;
	if (UEditorExperimentalSettings* EditorSettings = GetMutableDefault<UEditorExperimentalSettings>())
	{
		Result = EditorSettings->BlueprintProfilerMaxNodeThreshold == 0.5f ? EVisibility::Hidden : EVisibility::Visible;
	}
	return Result;
}

FReply SBlueprintProfilerToolbar::ResetNodeMaxThreshold()
{
	if (UEditorExperimentalSettings* EditorSettings = GetMutableDefault<UEditorExperimentalSettings>())
	{
		EditorSettings->BlueprintProfilerMaxNodeThreshold = 0.5f;
		FScriptPerfData::SetMaxPerformanceThreshold(0.5f);
	}
	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE
