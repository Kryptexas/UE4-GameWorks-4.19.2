// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherPackagePage.cpp: Implements the SSessionLauncherPackagePage class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherPackagePage"


/* SSessionLauncherPackagePage structors
 *****************************************************************************/

SSessionLauncherPackagePage::~SSessionLauncherPackagePage( )
{
	if (Model.IsValid())
	{
		Model->OnProfileSelected().RemoveAll(this);
	}
}


/* SSessionLauncherPackagePage interface
 *****************************************************************************/

void SSessionLauncherPackagePage::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel )
{
	Model = InModel;

	// create packing mode menu
	FMenuBuilder PackagingModeMenuBuilder(true, NULL);
	{
		FUIAction LocallyAction(FExecuteAction::CreateSP(this, &SSessionLauncherPackagePage::HandlePackagingModeMenuEntryClicked, ELauncherProfilePackagingModes::Locally));
		PackagingModeMenuBuilder.AddMenuEntry(LOCTEXT("LocallyAction", "Package & store locally"), LOCTEXT("LocallyActionHint", "Store this build locally."), FSlateIcon(), LocallyAction);
		
		FUIAction SharedRepositoryAction(FExecuteAction::CreateSP(this, &SSessionLauncherPackagePage::HandlePackagingModeMenuEntryClicked, ELauncherProfilePackagingModes::SharedRepository));
		PackagingModeMenuBuilder.AddMenuEntry(LOCTEXT("SharedRepositoryAction", "Package & store in repository"), LOCTEXT("SharedRepositoryActionHint", "Store this build in a shared repository."), FSlateIcon(), SharedRepositoryAction);

		FUIAction DoNotPackageAction(FExecuteAction::CreateSP(this, &SSessionLauncherPackagePage::HandlePackagingModeMenuEntryClicked, ELauncherProfilePackagingModes::DoNotPackage));
		PackagingModeMenuBuilder.AddMenuEntry(LOCTEXT("DoNotPackageAction", "Do not package"), LOCTEXT("DoNotPackageActionHint", "Do not package the build at this time."), FSlateIcon(), DoNotPackageAction);
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
							.Text(LOCTEXT("WhereToStoreBuildText", "How would you like to package the build?"))			
					]

				+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(8.0, 0.0, 0.0, 0.0)
					[
						// packaging mode menu
						SNew(SComboButton)
							.ButtonContent()
							[
								SNew(STextBlock)
									.Text(this, &SSessionLauncherPackagePage::HandlePackagingModeComboButtonContentText)
							]
							.ContentPadding(FMargin(6.0, 2.0))
							.MenuContent()
							[
								PackagingModeMenuBuilder.MakeWidget()
							]
					]
			]

		+ SVerticalBox::Slot()
			.FillHeight(1.0)
			.Padding(0.0, 8.0, 0.0, 0.0)
			[
				SNew(SSessionLauncherPackagingSettings, InModel)
					.Visibility(this, &SSessionLauncherPackagePage::HandlePackagingSettingsAreaVisibility)
			]
	];

	Model->OnProfileSelected().AddSP(this, &SSessionLauncherPackagePage::HandleProfileManagerProfileSelected);
}


/* SSessionLauncherLaunchPage callbacks
 *****************************************************************************/

FString SSessionLauncherPackagePage::HandlePackagingModeComboButtonContentText( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		ELauncherProfilePackagingModes::Type PackagingMode = SelectedProfile->GetPackagingMode();

		if (PackagingMode == ELauncherProfilePackagingModes::DoNotPackage)
		{
			return TEXT("Do not package");
		}

		if (PackagingMode == ELauncherProfilePackagingModes::Locally)
		{
			return TEXT("Package & store locally");
		}

		if (PackagingMode == ELauncherProfilePackagingModes::SharedRepository)
		{
			return TEXT("Package & store in repository");
		}

		return LOCTEXT("PackagingModeComboButtonDefaultText", "Select...").ToString();
	}

	return TEXT("");
}


void SSessionLauncherPackagePage::HandlePackagingModeMenuEntryClicked( ELauncherProfilePackagingModes::Type PackagingMode )
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		SelectedProfile->SetPackagingMode(PackagingMode);
	}
}



EVisibility SSessionLauncherPackagePage::HandlePackagingSettingsAreaVisibility( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		ELauncherProfilePackagingModes::Type PackagingMode = SelectedProfile->GetPackagingMode();

		if ((PackagingMode == ELauncherProfilePackagingModes::Locally) || (PackagingMode == ELauncherProfilePackagingModes::SharedRepository))
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


void SSessionLauncherPackagePage::HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile )
{

}


#undef LOCTEXT_NAMESPACE
