// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintProfilerModule.h"

/** Blueprint performance view type */
namespace EBlueprintPerfViewType
{
	enum Type
	{
		None = 0,
		Overview,
		ExecutionGraph,
		LeastPerformant
	};
}

//////////////////////////////////////////////////////////////////////////
// SBlueprintProfilerView

class SBlueprintProfilerView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBlueprintProfilerView)
		: _ProfileViewType(EBlueprintPerfViewType::ExecutionGraph)
		{}

		SLATE_ARGUMENT(TWeakPtr<FBlueprintEditor>, AssetEditor)
		SLATE_ARGUMENT(EBlueprintPerfViewType::Type, ProfileViewType)
	SLATE_END_ARGS()


public:

	~SBlueprintProfilerView();

	void Construct(const FArguments& InArgs);

protected:

	/** Called when the profiler state is toggled */
	void OnToggleProfiler(bool bEnabled);

	/** Returns the profiler current status message */
	FText GetProfilerStatusText() const { return StatusText; }

	/** Called to create the child profiler widgets */
	void UpdateActiveProfilerWidget();

	/** Returns the current view label */
	FText GetCurrentViewText() const;

	/** Constructs and returns the view button widget */
	TSharedRef<SWidget> CreateViewButton() const;

	/** Returns the active view button foreground color */
	FSlateColor GetViewButtonForegroundColor() const;

	/** Called when the profiler view type is changed */
	void OnViewSelectionChanged(const EBlueprintPerfViewType::Type NewViewType);

	/** Returns the current heat map display mode label */
	FText GetCurrentHeatMapDisplayModeText() const;

	/** Constructs and returns the heat map display mode button widget */
	TSharedRef<SWidget> CreateHeatMapDisplayModeButton() const;

	/** Returns the active heat map display mode button foreground color */
	FSlateColor GetHeatMapDisplayModeButtonForegroundColor() const;

	/** Returns whether or not the given heat map display mode is selected */
	bool IsHeatMapDisplayModeSelected(const EBlueprintProfilerHeatMapDisplayMode::Type InHeatMapDisplayMode) const;

	/** Called when the heat map display mode is changed */
	void OnHeatMapDisplayModeChanged(const EBlueprintProfilerHeatMapDisplayMode::Type NewHeatMapDisplayMode);

	/** Create active statistic display widget */
	TSharedRef<SWidget> CreateActiveStatisticWidget();

protected:

	/** Profiler status Text */
	FText StatusText;

	/** Blueprint editor */
	TWeakPtr<class FBlueprintEditor> BlueprintEditor;

	/** Current profiler view type */
	EBlueprintPerfViewType::Type CurrentViewType;

	/** View combo button widget */
	TSharedPtr<SComboButton> ViewComboButton;

	/** Heat map display mode combo button widget */
	TSharedPtr<SComboButton> HeatMapDisplayModeComboButton;
};
