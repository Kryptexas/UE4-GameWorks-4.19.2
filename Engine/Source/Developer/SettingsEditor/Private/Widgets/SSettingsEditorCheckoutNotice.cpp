// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SettingsEditorPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSettingsEditorCheckoutNotice"


/* SSettingsEditorCheckoutNotice interface
 *****************************************************************************/

void SSettingsEditorCheckoutNotice::Construct( const FArguments& InArgs )
{
	CheckOutClickedDelegate = InArgs._OnCheckOutClicked;
	bIsUnlocked = InArgs._Unlocked;
	ConfigFilePath = InArgs._ConfigFilePath;
	bLookingForSourceControlState = InArgs._LookingForSourceControlState;

	// default configuration notice
	ChildSlot
	[
		SNew(SBorder)
			.BorderBackgroundColor(this, &SSettingsEditorCheckoutNotice::GetLockedOrUnlockedStatusBarColor)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.LightGroupBorder"))
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
									.ColorAndOpacity(FLinearColor::White)
									.ShadowColorAndOpacity(FLinearColor::Black)
									.ShadowOffset(FVector2D::UnitVector)
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
									.ColorAndOpacity(FLinearColor::White)
									.ShadowColorAndOpacity(FLinearColor::Black)
									.ShadowOffset(FVector2D::UnitVector)
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
	FText ConfigFilename = FText::FromString(FPaths::GetCleanFilename(ConfigFilePath.Get()));

	return FText::Format(ISourceControlModule::Get().IsEnabled() ?
		LOCTEXT("DefaultSettingsNotice_WithSourceControl", "These settings are saved in {0}, which is not currently checked out.") :
		LOCTEXT("DefaultSettingsNotice_Source", "These settings are saved {0}, which is not currently writable."), ConfigFilename);
}

FText SSettingsEditorCheckoutNotice::HandleUnlockedStatusText() const
{
	FText ConfigFilename = FText::FromString(FPaths::GetCleanFilename(ConfigFilePath.Get()));
	
	return FText::Format(ISourceControlModule::Get().IsEnabled() ?
		LOCTEXT("DefaultSettingsNotice_CheckedOut", "These settings are saved in {0}, which is currently checked out.") :
		LOCTEXT("DefaultSettingsNotice_Writable", "These settings are saved in {0}, which is currently writable."), ConfigFilename);
}

FSlateColor SSettingsEditorCheckoutNotice::GetLockedOrUnlockedStatusBarColor() const
{
	return bIsUnlocked.Get() ? FLinearColor::Green : FLinearColor::Yellow;
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
