// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSettingsEditorCheckoutNotice.cpp: Implements the SSettingsEditorCheckoutNotice class.
=============================================================================*/

#include "SettingsEditorPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSettingsEditorCheckoutNotice"


/* SSettingsEditorCheckoutNotice interface
 *****************************************************************************/

void SSettingsEditorCheckoutNotice::Construct( const FArguments& InArgs )
{
	bIsUnlocked = InArgs._Unlocked;

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

						// Status icon
						+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SImage)
									.Image(FEditorStyle::GetBrush("GenericLock"))
							]

						// Notice
						+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.Padding(16.0f, 0.0f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
									.Text(LOCTEXT("DefaultSettingsNotice_Source", "These settings are always saved in the default configuration file, which is currently under source control."))
							]

						// Call to action
						+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								InArgs._LockedContent.Widget
							]

						+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(STextBlock)
									.Text(this, &SSettingsEditorCheckoutNotice::HandleSccUnavailableTextBlockText)
							]
					]

				// Unlocked slot
				+ SWidgetSwitcher::Slot()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SImage)
									.Image(FEditorStyle::GetBrush("GenericUnlock"))
							]

						+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(16.0f, 0.0f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
									.Text(LOCTEXT("DefaultSettingsNotice_Writable", "These settings are always saved in the default configuration file, which is currently writable."))
							]
					]
			]
	];
}


/* SSettingsEditorCheckoutNotice callbacks
 *****************************************************************************/

EVisibility SSettingsEditorCheckoutNotice::HandleCheckOutButtonVisibility( ) const
{
	if (ISourceControlModule::Get().IsEnabled() && ISourceControlModule::Get().GetProvider().IsAvailable())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}


FText SSettingsEditorCheckoutNotice::HandleSccUnavailableTextBlockText( ) const
{
	if (!ISourceControlModule::Get().IsEnabled())
	{
		return LOCTEXT("SccDisabledNotice", "[Source control is disabled]");
	}

	if (!ISourceControlModule::Get().GetProvider().IsAvailable())
	{
		return LOCTEXT("SccUnavailableNotice", "[Source control server unavailable]");
	}

	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE