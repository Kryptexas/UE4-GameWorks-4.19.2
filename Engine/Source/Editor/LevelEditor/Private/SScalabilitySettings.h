// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Scalability.h"
#include "Slate.h"

/**
 * Scalability settings configuration widget                                                                  
 **/

class SScalabilitySettings : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS( SScalabilitySettings )
	{}

	SLATE_END_ARGS()

	// Widget construction
	void Construct( const FArguments& InArgs );

private:

	// Stores state of scalability settings at the point of opening the menu
	Scalability::FQualityLevels CachedQualityLevels;

	// Checks cached quality levels to see if the specified group is at the specified quality level
	ESlateCheckBoxState::Type IsGroupQualityLevelSelected(const TCHAR* InGroupName, int32 InQualityLevel) const;

	// Callback for when a particular scalability group has its quality level changed
	void OnGroupQualityLevelChanged(ESlateCheckBoxState::Type NewState, const TCHAR* InGroupName, int32 InQualityLevel);

	// Callback for when the resolution scale slider changes
	void OnResolutionScaleChanged(float InValue);

	// Callback to retrieve current resolution scale
	float GetResolutionScale() const;

	// Callback to retrieve current resolution scale as a display string
	FString GetResolutionScaleString() const;

	// Makes a button widget for the group quality levels
	TSharedRef<SWidget> MakeButtonWidget(const FText& InName, const TCHAR* InGroupName, int32 InQualityLevel, const FText& InToolTip);

	// Makes a general quality level header button widget
	TSharedRef<SWidget> MakeHeaderButtonWidget(const FText& InName, int32 InQualityLevel, const FText& InToolTip);

	// Makes the auto benchmark button
	TSharedRef<SWidget> MakeAutoButtonWidget();

	// Callback for when a quality level header button is pressed
	FReply OnHeaderClicked(int32 InQualityLevel);

	// Callback for auto benchmark button
	FReply OnAutoClicked();

	// Create a gridslot for the group quality level with all the required formatting
	SGridPanel::FSlot& MakeGridSlot(int32 InCol, int32 InRow, int32 InColSpan = 1, int32 InRowSpan = 1);
};