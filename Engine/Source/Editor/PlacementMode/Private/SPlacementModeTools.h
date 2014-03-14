// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ClassIconFinder.h"
#include "AssetData.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"

class SPlacementModeTools : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SPlacementModeTools ){}
	SLATE_END_ARGS();

	void Construct( const FArguments& InArgs );

	virtual ~SPlacementModeTools();

private:

	// Gets the border image for the tab, this is the 'active' orange bar.
	const FSlateBrush* PlacementGroupBorderImage( int32 PlacementGroupIndex ) const;

	// When the tab is clicked we adjust the check state, so that the right style is displayed.
	void OnPlacementTabChanged( ESlateCheckBoxState::Type NewState, int32 PlacementGroupIndex );

	// Gets the tab 'active' state, so that we can show the active style
	ESlateCheckBoxState::Type GetPlacementTabCheckedState( int32 PlacementGroupIndex ) const;

	// Creates a tab widget to show on the left that when clicked actives the right widget switcher index.
	TSharedRef< SWidget > CreatePlacementGroupTab( int32 TabIndex, FText TabText, bool Important );

	// Builds the lights collection widget.
	TSharedRef< SWidget > BuildLightsWidget();

	// Builds the visual collection widget.
	TSharedRef< SWidget > BuildVisualWidget();

	// Builds the 'Basic' collection widget.
	TSharedRef< SWidget > BuildBasicWidget();

	// Called when the recently placed assets changes.
	void UpdateRecentlyPlacedAssets( const TArray< FActorPlacementInfo >& RecentlyPlaced );

	// Rebuilds the recently placed widget
	void RefreshRecentlyPlaced();

	// Rebuilds the 'Classes' widget.
	void RefreshPlaceables();

	// Rebuilds the 'Volumes' widget.
	void RefreshVolumes();

	// Begin SWidget
	void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;
	// End SWidget

private:

	// Flag set when the placeables widget needs to be rebuilt.
	bool bPlaceablesRefreshRequested;

	// Flag set when the volumes widget needs to be rebuilt.
	bool bVolumesRefreshRequested;

	// Holds the widget switcher.
	TSharedPtr<SWidgetSwitcher> WidgetSwitcher;

	// The container widget for the recently placed.
	TSharedPtr< SVerticalBox > RecentlyPlacedContainer;

	// The container widget for the volumes.
	TSharedPtr< SVerticalBox > VolumesContainer;

	// The container widget for the classes.
	TSharedPtr< SVerticalBox > PlaceablesContainer;
};