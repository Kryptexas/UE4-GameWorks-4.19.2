// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SettingsEditorPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSettingsEditorCheckoutNotice"


/* SSettingsEditorCheckoutNotice interface
 *****************************************************************************/

void SSettingsEditorCheckoutNotice::Construct( const FArguments& InArgs )
{
	CheckOutClickedDelegate = InArgs._OnCheckOutClicked;
	bIsUnlocked = InArgs._Unlocked;
	bLookingForSourceControlState = InArgs._LookingForSourceControlState;

	// default configuration notice
	ChildSlot
	[
		SNew(SBorder)
			.BorderBackgroundColor(FLinearColor::Yellow)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(8.0f)
			[
				SNew(SWidgetSwitcher)
					.WidgetIndex(this, &SSettingsEditorCheckoutNotice::HandleNoticeSwitcherWidgetIndex)

				// Locked slot
				+ SWidgetSwitcher::Slot()
					[
						SNew(SHorizontalBox)

						// Locked icon
						+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SImage)
									.Image(FEditorStyle::GetBrush("GenericLock"))
							]

						// Locked notice
						+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.Padding(16.0f, 0.0f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
									.Text(this, &SSettingsEditorCheckoutNotice::HandleLockedStatusText)
							]

						// Check out button
						+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SButton)
									.OnClicked(this, &SSettingsEditorCheckoutNotice::HandleCheckOutButtonClicked)
									.Text(this, &SSettingsEditorCheckoutNotice::HandleCheckOutButtonText)
									.ToolTipText(this, &SSettingsEditorCheckoutNotice::HandleCheckOutButtonToolTip)
									.Visibility(this, &SSettingsEditorCheckoutNotice::HandleCheckOutButtonVisibility)
							]

						// Source control status throbber
						+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(0.0f, 1.0f)
							[
								SNew(SThrobber)
									.Visibility(this, &SSettingsEditorCheckoutNotice::HandleThrobberVisibility)
							]
					]

				// Unlocked slot
				+ SWidgetSwitcher::Slot()
					[
						SNew(SHorizontalBox)

						// Unlocked icon
						+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SImage)
									.Image(FEditorStyle::GetBrush("GenericUnlock"))
							]

						// Unlocked notice
						+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(16.0f, 0.0f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
									.Text(this, &SSettingsEditorCheckoutNotice::HandleUnlockedStatusText)
							]
					]
			]
	];
}


/* SSettingsEditorCheckoutNotice callbacks
 *****************************************************************************/

FReply SSettingsEditorCheckoutNotice::HandleCheckOutButtonClicked( )
{
	if (CheckOutClickedDelegate.IsBound())
	{
		return CheckOutClickedDelegate.Execute();
	}

	return FReply::Handled();
}


FText SSettingsEditorCheckoutNotice::HandleCheckOutButtonText( ) const
{
	if (ISourceControlModule::Get().IsEnabled() && ISourceControlModule::Get().GetProvider().IsAvailable())
	{
		return LOCTEXT("CheckOutFile", "Check Out File");
	}

	return LOCTEXT("MakeWritable", "Make Writable");
}


FText SSettingsEditorCheckoutNotice::HandleCheckOutButtonToolTip( ) const
{
	if (ISourceControlModule::Get().IsEnabled() && ISourceControlModule::Get().GetProvider().IsAvailable())
	{
		return LOCTEXT("CheckOutFileTooltip", "Check out the default configuration file that holds these settings.");
	}

	return LOCTEXT("MakeWritableTooltip", "Make the default configuration file that holds these settings writable.");
}


EVisibility SSettingsEditorCheckoutNotice::HandleCheckOutButtonVisibility( ) const
{
	if (ISourceControlModule::Get().IsEnabled() && ISourceControlModule::Get().GetProvider().IsAvailable())
	{
		return !bLookingForSourceControlState.Get() ? EVisibility::Visible : EVisibility::Collapsed;
	}

	return EVisibility::Collapsed;
}

FText SSettingsEditorCheckoutNotice::HandleLockedStatusText() const
{
	return ISourceControlModule::Get().IsEnabled() ? 
		LOCTEXT("DefaultSettingsNotice_WithSourceControl", "These settings are always saved in the default configuration file, which is currently not checked out.") :
		LOCTEXT("DefaultSettingsNotice_Source", "These settings are always saved in the default configuration file, which is currently not writable.");
}

FText SSettingsEditorCheckoutNotice::HandleUnlockedStatusText() const
{
	return ISourceControlModule::Get().IsEnabled() ? 
		LOCTEXT("DefaultSettingsNotice_CheckedOut", "These settings are always saved in the default configuration file, which is currently checked out.") :
		LOCTEXT("DefaultSettingsNotice_Writable", "These settings are always saved in the default configuration file, which is currently writable.");
}

EVisibility SSettingsEditorCheckoutNotice::HandleThrobberVisibility() const
{
	if(ISourceControlModule::Get().IsEnabled())
	{
		return bLookingForSourceControlState.Get() ? EVisibility::Visible : EVisibility::Collapsed;
	}
	return EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE
