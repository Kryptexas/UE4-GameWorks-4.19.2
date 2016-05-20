// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "SBlueprintProfilerView.h"

// Profiler View types
#include "SProfilerOverview.h"
#include "SLeastPerformantDisplay.h"
#include "SGraphExecutionStatDisplay.h"
#include "ScriptPerfData.h"

#define LOCTEXT_NAMESPACE "BlueprintProfilerView"

//////////////////////////////////////////////////////////////////////////
// SBlueprintProfilerView

SBlueprintProfilerView::~SBlueprintProfilerView()
{
	// Register for Profiler toggle events
	FBlueprintCoreDelegates::OnToggleScriptProfiler.RemoveAll(this);
}

void SBlueprintProfilerView::Construct(const FArguments& InArgs)
{
	CurrentViewType = InArgs._ProfileViewType;
	BlueprintEditor = InArgs._AssetEditor;
	// Register for Profiler toggle events
	FBlueprintCoreDelegates::OnToggleScriptProfiler.AddSP(this, &SBlueprintProfilerView::OnToggleProfiler);
	// Initialise the number format.
	FNumberFormattingOptions StatisticNumberFormat;
	StatisticNumberFormat.MinimumFractionalDigits = 4;
	StatisticNumberFormat.MaximumFractionalDigits = 4;
	StatisticNumberFormat.UseGrouping = false;
	FScriptPerfData::SetNumberFormattingForStats(StatisticNumberFormat);
	// Create the display text for the user
	if (GetDefault<UEditorExperimentalSettings>()->bBlueprintPerformanceAnalysisTools)
	{
		IBlueprintProfilerInterface* ProfilerInterface = FModuleManager::GetModulePtr<IBlueprintProfilerInterface>("BlueprintProfiler");
		const bool bProfilerEnabled	= ProfilerInterface && ProfilerInterface->IsProfilerEnabled();
		StatusText = bProfilerEnabled ? LOCTEXT("ProfilerNoDataText", "The Blueprint Profiler is active but does not currently have any data to display") :
										LOCTEXT("ProfilerInactiveText", "The Blueprint Profiler is currently Inactive");
	}
	else
	{
		StatusText = LOCTEXT("DisabledProfilerText", "The Blueprint Profiler is experimental and is currently not enabled in the editor preferences");
	}
	// Create the profiler view widgets
	UpdateActiveProfilerWidget();
}

void SBlueprintProfilerView::OnToggleProfiler(bool bEnabled)
{
	if (GetDefault<UEditorExperimentalSettings>()->bBlueprintPerformanceAnalysisTools)
	{
		StatusText = bEnabled ? LOCTEXT("ProfilerNoDataText", "The Blueprint Profiler is active but does not currently have any data to display") :
								LOCTEXT("ProfilerInactiveText", "The Blueprint Profiler is currently Inactive");
	}
	else
	{
		StatusText = LOCTEXT("DisabledProfilerText", "The Blueprint Profiler is experimental and is currently not enabled in the editor preferences");
	}
	UpdateActiveProfilerWidget();
}

void SBlueprintProfilerView::UpdateActiveProfilerWidget()
{
		FMenuBuilder ViewComboContent(true, NULL);
		ViewComboContent.AddMenuEntry(	LOCTEXT("OverviewViewType", "Profiler Overview"), 
										LOCTEXT("OverviewViewTypeDesc", "Displays High Level Profiling Information"), 
										FSlateIcon(), 
										FExecuteAction::CreateSP(this, &SBlueprintProfilerView::OnViewSelectionChanged, EBlueprintPerfViewType::Overview),
										NAME_None, 
										EUserInterfaceActionType::Button);
		ViewComboContent.AddMenuEntry(	LOCTEXT("ExecutionGraphViewType", "Execution Graph"), 
										LOCTEXT("ExecutionGraphViewTypeDesc", "Displays the Execution Graph with Statistics"), 
										FSlateIcon(), 
										FExecuteAction::CreateSP(this, &SBlueprintProfilerView::OnViewSelectionChanged, EBlueprintPerfViewType::ExecutionGraph),
										NAME_None, 
										EUserInterfaceActionType::Button);
		ViewComboContent.AddMenuEntry(	LOCTEXT("LeastPerformantViewType", "Least Performant List"), 
										LOCTEXT("LeastPerformantViewTypeDesc", "Displays a list of Least Performant Areas of the Blueprint"), 
										FSlateIcon(), 
										FExecuteAction::CreateSP(this, &SBlueprintProfilerView::OnViewSelectionChanged, EBlueprintPerfViewType::LeastPerformant),
										NAME_None, 
										EUserInterfaceActionType::Button);

		FMenuBuilder HeatMapDisplayModeComboContent(true, nullptr);
		HeatMapDisplayModeComboContent.AddMenuEntry(LOCTEXT("HeatMapDisplayMode_None", "Off"),
			LOCTEXT("HeatMapDisplayMode_None_Desc", "Normal Display (No Heat Map)"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SBlueprintProfilerView::OnHeatMapDisplayModeChanged, EBlueprintProfilerHeatMapDisplayMode::None),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SBlueprintProfilerView::IsHeatMapDisplayModeSelected, EBlueprintProfilerHeatMapDisplayMode::None)
			),
			NAME_None,
			EUserInterfaceActionType::Check);
		HeatMapDisplayModeComboContent.AddMenuEntry(LOCTEXT("HeatMapDisplayMode_Exclusive", "Exclusive"),
			LOCTEXT("HeatMapDisplayMode_Exclusive_Desc", "Heat Map - Exclusive Timings"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SBlueprintProfilerView::OnHeatMapDisplayModeChanged, EBlueprintProfilerHeatMapDisplayMode::Exclusive),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SBlueprintProfilerView::IsHeatMapDisplayModeSelected, EBlueprintProfilerHeatMapDisplayMode::Exclusive)
			),
			NAME_None,
			EUserInterfaceActionType::Check);
		HeatMapDisplayModeComboContent.AddMenuEntry(LOCTEXT("HeatMapDisplayMode_Inclusive", "Inclusive"),
			LOCTEXT("HeatMapDisplayMode_Inclusive_Desc", "Heat Map - Inclusive Timings"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SBlueprintProfilerView::OnHeatMapDisplayModeChanged, EBlueprintProfilerHeatMapDisplayMode::Inclusive),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SBlueprintProfilerView::IsHeatMapDisplayModeSelected, EBlueprintProfilerHeatMapDisplayMode::Inclusive)
			),
			NAME_None,
			EUserInterfaceActionType::Check);
		HeatMapDisplayModeComboContent.AddMenuEntry(LOCTEXT("HeatMapDisplayMode_MaxTiming", "Max Time"),
			LOCTEXT("HeatMapDisplayMode_MaxTiming_Desc", "Heat Map - Max Time"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SBlueprintProfilerView::OnHeatMapDisplayModeChanged, EBlueprintProfilerHeatMapDisplayMode::MaxTiming),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SBlueprintProfilerView::IsHeatMapDisplayModeSelected, EBlueprintProfilerHeatMapDisplayMode::MaxTiming)
			),
			NAME_None,
			EUserInterfaceActionType::Check);

		FMenuBuilder WireHeatMapDisplayModeComboContent(true, nullptr);
		WireHeatMapDisplayModeComboContent.AddMenuEntry(LOCTEXT("WireHeatMapDisplayMode_None", "Off"),
			LOCTEXT("HeatMapDisplayMode_None_Desc", "Normal Display (No Heat Map)"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SBlueprintProfilerView::OnWireHeatMapDisplayModeChanged, EBlueprintProfilerHeatMapDisplayMode::None),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SBlueprintProfilerView::IsWireHeatMapDisplayModeSelected, EBlueprintProfilerHeatMapDisplayMode::None)
			),
			NAME_None,
			EUserInterfaceActionType::Check);
		WireHeatMapDisplayModeComboContent.AddMenuEntry(LOCTEXT("HeatMapDisplayMode_Exclusive", "Exclusive"),
			LOCTEXT("HeatMapDisplayMode_Exclusive_Desc", "Heat Map - Exclusive Timings"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SBlueprintProfilerView::OnWireHeatMapDisplayModeChanged, EBlueprintProfilerHeatMapDisplayMode::Exclusive),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SBlueprintProfilerView::IsWireHeatMapDisplayModeSelected, EBlueprintProfilerHeatMapDisplayMode::Exclusive)
			),
			NAME_None,
			EUserInterfaceActionType::Check);
		WireHeatMapDisplayModeComboContent.AddMenuEntry(LOCTEXT("HeatMapDisplayMode_Inclusive", "Inclusive"),
			LOCTEXT("HeatMapDisplayMode_Inclusive_Desc", "Heat Map - Inclusive Timings"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SBlueprintProfilerView::OnWireHeatMapDisplayModeChanged, EBlueprintProfilerHeatMapDisplayMode::Inclusive),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SBlueprintProfilerView::IsWireHeatMapDisplayModeSelected, EBlueprintProfilerHeatMapDisplayMode::Inclusive)
			),
			NAME_None,
			EUserInterfaceActionType::Check);
		WireHeatMapDisplayModeComboContent.AddMenuEntry(LOCTEXT("HeatMapDisplayMode_MaxTiming", "Max Time"),
			LOCTEXT("HeatMapDisplayMode_MaxTiming_Desc", "Heat Map - Max Time"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SBlueprintProfilerView::OnWireHeatMapDisplayModeChanged, EBlueprintProfilerHeatMapDisplayMode::MaxTiming),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SBlueprintProfilerView::IsWireHeatMapDisplayModeSelected, EBlueprintProfilerHeatMapDisplayMode::MaxTiming)
			),
			NAME_None,
			EUserInterfaceActionType::Check);

		ChildSlot
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("BlueprintProfiler.ViewToolBar"))
				.Padding(0)
				.VAlign(VAlign_Top)
				.HAlign(HAlign_Right)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FMargin(5, 0))
					[
						SAssignNew(HeatMapDisplayModeComboButton, SComboButton)
						.ForegroundColor(this, &SBlueprintProfilerView::GetHeatMapDisplayModeButtonForegroundColor)
						.ToolTipText(LOCTEXT("BlueprintProfilerHeatMapDisplayMode_Tooltip", "Heat Map Display Mode"))
						.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
						.ContentPadding(2)
						.MenuContent()
						[
							HeatMapDisplayModeComboContent.MakeWidget()
						]
						.ButtonContent()
						[
							CreateHeatMapDisplayModeButton()
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
						.ForegroundColor(this, &SBlueprintProfilerView::GetWireHeatMapDisplayModeButtonForegroundColor)
						.ToolTipText(LOCTEXT("BlueprintProfilerWireHeatMapDisplayMode_Tooltip", "Wire Heat Map Display Mode"))
						.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
						.ContentPadding(2)
						.MenuContent()
						[
							WireHeatMapDisplayModeComboContent.MakeWidget()
						]
						.ButtonContent()
						[
							CreateWireHeatMapDisplayModeButton()
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
					.Padding(FMargin(5,0))
					[
						SAssignNew(ViewComboButton, SComboButton)
						.ForegroundColor(this, &SBlueprintProfilerView::GetViewButtonForegroundColor)
						.ToolTipText(LOCTEXT("BlueprintProfilerViewType", "View Type"))
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
				]
			]
			+SVerticalBox::Slot()
			[
				SNew(SBorder)
				.Padding(FMargin(0,2,0,0))
				.BorderImage(FEditorStyle::GetBrush("NoBorder"))
				[
					CreateActiveStatisticWidget()
				]
			]
		];
	}

FText SBlueprintProfilerView::GetCurrentViewText() const
{
	switch(CurrentViewType)
	{
		case EBlueprintPerfViewType::Overview:			return LOCTEXT("OverviewLabel", "Profiler Overview");
		case EBlueprintPerfViewType::ExecutionGraph:	return LOCTEXT("ExecutionGraphLabel", "Execution Graph");
		case EBlueprintPerfViewType::LeastPerformant:	return LOCTEXT("LeastPerformantLabel", "Least Performant");
	}
	return LOCTEXT("UnknownViewLabel", "Unknown ViewType");
}

TSharedRef<SWidget> SBlueprintProfilerView::CreateViewButton() const
{
	return SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SImage)
			.Image( FEditorStyle::GetBrush("GenericViewButton") )
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2, 0, 0, 0)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(GetCurrentViewText())
		];
}

FSlateColor SBlueprintProfilerView::GetViewButtonForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");
	return ViewComboButton->IsHovered() ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
}

void SBlueprintProfilerView::OnViewSelectionChanged(const EBlueprintPerfViewType::Type NewViewType)
{
	if (CurrentViewType != NewViewType)
	{
		CurrentViewType = NewViewType;
		UpdateActiveProfilerWidget();
	}
}

FText SBlueprintProfilerView::GetCurrentHeatMapDisplayModeText() const
{
	IBlueprintProfilerInterface& ProfilerModule = FModuleManager::LoadModuleChecked<IBlueprintProfilerInterface>("BlueprintProfiler");
	switch (ProfilerModule.GetGraphNodeHeatMapDisplayMode())
	{
	case EBlueprintProfilerHeatMapDisplayMode::None:		return LOCTEXT("HeatMapDisplayModeLabel_None", "Heat Map: Off");
	case EBlueprintProfilerHeatMapDisplayMode::Exclusive:	return LOCTEXT("HeatMapDisplayModeLabel_Exclusive", "Heat Map: Exclusive");
	case EBlueprintProfilerHeatMapDisplayMode::Inclusive:	return LOCTEXT("HeatMapDisplayModeLabel_Inclusive", "Heat Map: Inclusive");
	case EBlueprintProfilerHeatMapDisplayMode::MaxTiming:	return LOCTEXT("HeatMapDisplayModeLabel_MaxTiming", "Heat Map: Max Timing");
	}
	return LOCTEXT("HeatMapDisplayModeLabel_Unknown", "<unknown>");
}

TSharedRef<SWidget> SBlueprintProfilerView::CreateHeatMapDisplayModeButton() const
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(this, &SBlueprintProfilerView::GetCurrentHeatMapDisplayModeText)
		];
}

FSlateColor SBlueprintProfilerView::GetHeatMapDisplayModeButtonForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");
	return HeatMapDisplayModeComboButton->IsHovered() ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
}

bool SBlueprintProfilerView::IsHeatMapDisplayModeSelected(const EBlueprintProfilerHeatMapDisplayMode::Type InHeatMapDisplayMode) const
{
	IBlueprintProfilerInterface& ProfilerModule = FModuleManager::LoadModuleChecked<IBlueprintProfilerInterface>("BlueprintProfiler");
	return ProfilerModule.GetGraphNodeHeatMapDisplayMode() == InHeatMapDisplayMode;
}

void SBlueprintProfilerView::OnHeatMapDisplayModeChanged(const EBlueprintProfilerHeatMapDisplayMode::Type NewHeatMapDisplayMode)
{
	IBlueprintProfilerInterface& ProfilerModule = FModuleManager::LoadModuleChecked<IBlueprintProfilerInterface>("BlueprintProfiler");
	if (ProfilerModule.GetGraphNodeHeatMapDisplayMode() != NewHeatMapDisplayMode)
	{
		ProfilerModule.SetGraphNodeHeatMapDisplayMode(NewHeatMapDisplayMode);
	}
}

FText SBlueprintProfilerView::GetCurrentWireHeatMapDisplayModeText() const
{
	IBlueprintProfilerInterface& ProfilerModule = FModuleManager::LoadModuleChecked<IBlueprintProfilerInterface>("BlueprintProfiler");
	switch (ProfilerModule.GetWireHeatMapDisplayMode())
	{
	case EBlueprintProfilerHeatMapDisplayMode::None:		return LOCTEXT("WireHeatMapDisplayModeLabel_None", "Wire Heat Map: Off");
	case EBlueprintProfilerHeatMapDisplayMode::Exclusive:	return LOCTEXT("WireHeatMapDisplayModeLabel_Exclusive", "Wire Heat Map: Exclusive");
	case EBlueprintProfilerHeatMapDisplayMode::Inclusive:	return LOCTEXT("WireHeatMapDisplayModeLabel_Inclusive", "Wire Heat Map: Inclusive");
	case EBlueprintProfilerHeatMapDisplayMode::MaxTiming:	return LOCTEXT("WireHeatMapDisplayModeLabel_MaxTiming", "Wire Heat Map: Max Timing");
	}
	return LOCTEXT("WireHeatMapDisplayModeLabel_Unknown", "<unknown>");
}

TSharedRef<SWidget> SBlueprintProfilerView::CreateWireHeatMapDisplayModeButton() const
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(this, &SBlueprintProfilerView::GetCurrentWireHeatMapDisplayModeText)
		];
}

FSlateColor SBlueprintProfilerView::GetWireHeatMapDisplayModeButtonForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");
	return WireHeatMapDisplayModeComboButton->IsHovered() ? FEditorStyle::GetSlateColor(InvertedForegroundName) : FEditorStyle::GetSlateColor(DefaultForegroundName);
}

bool SBlueprintProfilerView::IsWireHeatMapDisplayModeSelected(const EBlueprintProfilerHeatMapDisplayMode::Type InHeatMapDisplayMode) const
{
	IBlueprintProfilerInterface& ProfilerModule = FModuleManager::LoadModuleChecked<IBlueprintProfilerInterface>("BlueprintProfiler");
	return ProfilerModule.GetWireHeatMapDisplayMode() == InHeatMapDisplayMode;
}

void SBlueprintProfilerView::OnWireHeatMapDisplayModeChanged(const EBlueprintProfilerHeatMapDisplayMode::Type NewHeatMapDisplayMode)
{
	IBlueprintProfilerInterface& ProfilerModule = FModuleManager::LoadModuleChecked<IBlueprintProfilerInterface>("BlueprintProfiler");
	if (ProfilerModule.GetWireHeatMapDisplayMode() != NewHeatMapDisplayMode)
	{
		ProfilerModule.SetWireHeatMapDisplayMode(NewHeatMapDisplayMode);
	}
}

TSharedRef<SWidget> SBlueprintProfilerView::CreateActiveStatisticWidget()
{
	// Get profiler status
	EBlueprintPerfViewType::Type NewViewType = EBlueprintPerfViewType::None;
	if (GetDefault<UEditorExperimentalSettings>()->bBlueprintPerformanceAnalysisTools)
	{
		IBlueprintProfilerInterface* ProfilerInterface = FModuleManager::GetModulePtr<IBlueprintProfilerInterface>("BlueprintProfiler");
		if (ProfilerInterface && ProfilerInterface->IsProfilerEnabled())
		{
			NewViewType = CurrentViewType;
		}
	}
	switch(NewViewType)
	{
		case EBlueprintPerfViewType::Overview:
		{
			return SNew(SProfilerOverview)
				.AssetEditor(BlueprintEditor);
		}
		case EBlueprintPerfViewType::ExecutionGraph:
		{
			return SNew(SGraphExecutionStatDisplay)
				.AssetEditor(BlueprintEditor);
		}
		case EBlueprintPerfViewType::LeastPerformant:
		{
			return SNew(SLeastPerformantDisplay)
				.AssetEditor(BlueprintEditor);
		}
		default:
		{
			// Default place holder
			return SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(0)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					[
						SNew(SBorder)
						.Padding(0)
						.BorderImage(FEditorStyle::GetBrush("NoBorder"))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(this, &SBlueprintProfilerView::GetProfilerStatusText)
						]
					]
				];
		}
	}
}

#undef LOCTEXT_NAMESPACE
