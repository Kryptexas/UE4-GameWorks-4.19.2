// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherBuildConfigurationSelector.h: Declares the SSessionLauncherBuildConfigurationSelector class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SSessionLauncherBuildConfigurationSelector"


/**
 * Delegate type for build configuration selections.
 *
 * The first parameter is the selected build configuration.
 */
DECLARE_DELEGATE_OneParam(FOnSessionLauncherBuildConfigurationSelected, EBuildConfigurations::Type)


/**
 * Implements a build configuration selector widget.
 */
class SSessionLauncherBuildConfigurationSelector
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherBuildConfigurationSelector) { }
		SLATE_EVENT(FOnSessionLauncherBuildConfigurationSelected, OnConfigurationSelected)
		SLATE_ATTRIBUTE(FString, Text)
	SLATE_END_ARGS()


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct(	const FArguments& InArgs )
	{
		OnConfigurationSelected = InArgs._OnConfigurationSelected;

		// create build configurations menu
		FMenuBuilder MenuBuilder(true, NULL);
		{
			FUIAction DebugAction(FExecuteAction::CreateSP(this, &SSessionLauncherBuildConfigurationSelector::HandleMenuEntryClicked, EBuildConfigurations::Debug));
			MenuBuilder.AddMenuEntry(LOCTEXT("DebugAction", "Debug"), LOCTEXT("DebugActionHint", "Debug configuration."), FSlateIcon(), DebugAction);

			FUIAction DebugGameAction(FExecuteAction::CreateSP(this, &SSessionLauncherBuildConfigurationSelector::HandleMenuEntryClicked, EBuildConfigurations::DebugGame));
			MenuBuilder.AddMenuEntry(LOCTEXT("DebugGameAction", "DebugGame"), LOCTEXT("DebugGameActionHint", "DebugGame configuration."), FSlateIcon(), DebugGameAction);

			FUIAction DevelopmentAction(FExecuteAction::CreateSP(this, &SSessionLauncherBuildConfigurationSelector::HandleMenuEntryClicked, EBuildConfigurations::Development));
			MenuBuilder.AddMenuEntry(LOCTEXT("DevelopmentAction", "Development"), LOCTEXT("DevelopmentActionHint", "Development configuration."), FSlateIcon(), DevelopmentAction);

			FUIAction ShippingAction(FExecuteAction::CreateSP(this, &SSessionLauncherBuildConfigurationSelector::HandleMenuEntryClicked, EBuildConfigurations::Shipping));
			MenuBuilder.AddMenuEntry(LOCTEXT("ShippingAction", "Shipping"), LOCTEXT("ShippingActionHint", "Shipping configuration."), FSlateIcon(), ShippingAction);

			FUIAction TestAction(FExecuteAction::CreateSP(this, &SSessionLauncherBuildConfigurationSelector::HandleMenuEntryClicked, EBuildConfigurations::Test));
			MenuBuilder.AddMenuEntry(LOCTEXT("TestAction", "Test"), LOCTEXT("TestActionHint", "Test configuration."), FSlateIcon(), TestAction);
		}

		ChildSlot
		[
			// build configuration menu
			SNew(SComboButton)
				.ButtonContent()
				[
					SNew(STextBlock)
						.Text(InArgs._Text)
				]
				.ContentPadding(FMargin(6.0f, 2.0f))
				.MenuContent()
				[
					MenuBuilder.MakeWidget()
				]
		];
	}


private:

	// Callback for clicking a menu entry.
	void HandleMenuEntryClicked( EBuildConfigurations::Type Configuration )
	{
		OnConfigurationSelected.ExecuteIfBound(Configuration);
	}


private:

	// Holds a delegate to be invoked when a build configuration has been selected.
	FOnSessionLauncherBuildConfigurationSelected OnConfigurationSelected;
};


#undef LOCTEXT_NAMESPACE
