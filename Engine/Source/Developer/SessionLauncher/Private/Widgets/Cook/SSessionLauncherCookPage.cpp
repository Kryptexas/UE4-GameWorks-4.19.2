// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherCookPage.cpp: Implements the SSessionLauncherCookPage class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherCookPage"


/* SSessionLauncherCookPage structors
 *****************************************************************************/

SSessionLauncherCookPage::~SSessionLauncherCookPage( )
{
	if (Model.IsValid())
	{
		Model->OnProfileSelected().RemoveAll(this);
	}
}


/* SSessionLauncherCookPage interface
 *****************************************************************************/

void SSessionLauncherCookPage::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel )
{
	Model = InModel;

	// create cook modes menu
	FMenuBuilder CookModeMenuBuilder(true, NULL);
	{
		FUIAction ByTheBookAction(FExecuteAction::CreateSP(this, &SSessionLauncherCookPage::HandleCookModeMenuEntryClicked, ELauncherProfileCookModes::ByTheBook));
		CookModeMenuBuilder.AddMenuEntry(LOCTEXT("ByTheBookAction", "By the book"), LOCTEXT("ByTheBookActionHint", "Specify which content should be cooked and cook everything in advance prior to launching the game."), FSlateIcon(), ByTheBookAction);
		
		FUIAction OnTheFlyAction(FExecuteAction::CreateSP(this, &SSessionLauncherCookPage::HandleCookModeMenuEntryClicked, ELauncherProfileCookModes::OnTheFly));
		CookModeMenuBuilder.AddMenuEntry(LOCTEXT("OnTheFlyAction", "On the fly"), LOCTEXT("OnTheFlyActionHint", "Cook the content at run-time before it is being sent to the device."), FSlateIcon(), OnTheFlyAction);

		FUIAction DoNotCookAction(FExecuteAction::CreateSP(this, &SSessionLauncherCookPage::HandleCookModeMenuEntryClicked, ELauncherProfileCookModes::DoNotCook));
		CookModeMenuBuilder.AddMenuEntry(LOCTEXT("DoNotCookAction", "Do not cook"), LOCTEXT("DoNotCookActionHint", "Do not cook the content at this time."), FSlateIcon(), DoNotCookAction);
	}

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
					.FillWidth(1.0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("HowToCookText", "How would you like to cook the content?"))
					]

				+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(8.0, 0.0, 0.0, 0.0)
					[
						// cooking mode menu
						SNew(SComboButton)
							.ButtonContent()
							[
								SNew(STextBlock)
									.Text(this, &SSessionLauncherCookPage::HandleCookModeComboButtonContentText)
							]
							.ContentPadding(FMargin(6.0, 2.0))
							.MenuContent()
							[
								CookModeMenuBuilder.MakeWidget()
							]
					]
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0, 8.0, 0.0, 0.0)
			[
				SNew(SSessionLauncherCookOnTheFlySettings, InModel)
					.Visibility(this, &SSessionLauncherCookPage::HandleCookOnTheFlySettingsVisibility)
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0, 8.0, 0.0, 0.0)
			[
				SNew(SSessionLauncherCookByTheBookSettings, InModel)
					.Visibility(this, &SSessionLauncherCookPage::HandleCookByTheBookSettingsVisibility)
			]
	];

	Model->OnProfileSelected().AddSP(this, &SSessionLauncherCookPage::HandleProfileManagerProfileSelected);
}


/* SSessionLauncherCookPage callbacks
 *****************************************************************************/

EVisibility SSessionLauncherCookPage::HandleCookByTheBookSettingsVisibility( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->GetCookMode() == ELauncherProfileCookModes::ByTheBook)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


FText SSessionLauncherCookPage::HandleCookModeComboButtonContentText( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		ELauncherProfileCookModes::Type CookMode = SelectedProfile->GetCookMode();

		if (CookMode == ELauncherProfileCookModes::ByTheBook)
		{
			return LOCTEXT("CookModeComboButton_ByTheBook", "By the book");
		}

		if (CookMode == ELauncherProfileCookModes::DoNotCook)
		{
			return LOCTEXT("CookModeComboButton_DoNotCook", "Do not cook");
		}

		if (CookMode == ELauncherProfileCookModes::OnTheFly)
		{
			return LOCTEXT("CookModeComboButton_OnTheFly", "On the fly");
		}

		return LOCTEXT("CookModeComboButtonDefaultText", "Select...");
	}

	return FText();
}


void SSessionLauncherCookPage::HandleCookModeMenuEntryClicked( ELauncherProfileCookModes::Type CookMode )
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		SelectedProfile->SetCookMode(CookMode);
	}
}


EVisibility SSessionLauncherCookPage::HandleCookOnTheFlySettingsVisibility( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->GetCookMode() == ELauncherProfileCookModes::OnTheFly)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


void SSessionLauncherCookPage::HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile )
{
	// reload settings
}


#undef LOCTEXT_NAMESPACE
