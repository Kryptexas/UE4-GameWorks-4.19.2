// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherDevicesPage.cpp: Implements the SSessionLauncherDevicesPage class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherDevicesPage"


/* SSessionLauncherProfilePage structors
 *****************************************************************************/

SSessionLauncherDeployPage::~SSessionLauncherDeployPage( )
{
	if (Model.IsValid())
	{
		Model->OnProfileSelected().RemoveAll(this);
	}
}


/* SSessionLauncherDevicesPage interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSessionLauncherDeployPage::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel, bool IsFromRepository )
{
	Model = InModel;

	// create deployment mode menu
	FMenuBuilder DeploymentModeMenuBuilder(true, NULL);
	{
		FUIAction FileServerAction(FExecuteAction::CreateSP(this, &SSessionLauncherDeployPage::HandleDeploymentModeMenuEntryClicked, ELauncherProfileDeploymentModes::FileServer));
		DeploymentModeMenuBuilder.AddMenuEntry(LOCTEXT("FileServerAction", "File server"), LOCTEXT("FileServerActionHint", "Use a file server to deploy game content on the fly."), FSlateIcon(), FileServerAction);
		
		FUIAction CopyToDeviceAction(FExecuteAction::CreateSP(this, &SSessionLauncherDeployPage::HandleDeploymentModeMenuEntryClicked, ELauncherProfileDeploymentModes::CopyToDevice));
		DeploymentModeMenuBuilder.AddMenuEntry(LOCTEXT("CopyToDeviceAction", "Copy to device"), LOCTEXT("CopyToDeviceActionHint", "Copy the entire build to the device."), FSlateIcon(), CopyToDeviceAction);

		FUIAction DoNotDeployAction(FExecuteAction::CreateSP(this, &SSessionLauncherDeployPage::HandleDeploymentModeMenuEntryClicked, ELauncherProfileDeploymentModes::DoNotDeploy));
		DeploymentModeMenuBuilder.AddMenuEntry(LOCTEXT("DoNotDeployAction", "Do not deploy"), LOCTEXT("DoNotDeployActionHint", "Do not deploy the build at this time."), FSlateIcon(), DoNotDeployAction);

		FUIAction CopyRepositoryAction(FExecuteAction::CreateSP(this, &SSessionLauncherDeployPage::HandleDeploymentModeMenuEntryClicked, ELauncherProfileDeploymentModes::CopyRepository));
		DeploymentModeMenuBuilder.AddMenuEntry(LOCTEXT("CopyRepositoryAction", "Copy repository"), LOCTEXT("CopyRepositoryActionHint", "Copy a build from a repository to the device."), FSlateIcon(), CopyRepositoryAction);
	}

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				.Visibility(IsFromRepository ? EVisibility::Hidden : EVisibility::Visible)

				+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("HowToDeployText", "How would you like to deploy the build?"))			
					]

				+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(8.0f, 0.0f, 0.0f, 0.0f)
					[
						// deployment mode menu
						SNew(SComboButton)
							.ButtonContent()
							[
								SNew(STextBlock)
									.Text(this, &SSessionLauncherDeployPage::HandleDeploymentModeComboButtonContentText)
							]
							.ContentPadding(FMargin(6.0f, 2.0f))
							.MenuContent()
							[
								DeploymentModeMenuBuilder.MakeWidget()
							]
					]
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(8.0f)
					.Visibility(this, &SSessionLauncherDeployPage::HandleValidationErrorIconVisibility, ELauncherProfileValidationErrors::CopyToDeviceRequiresCookByTheBook)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SImage)
									.Image(FEditorStyle::GetBrush(TEXT("Icons.Error")))
							]

						+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(4.0f, 0.0f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
									.Text(LOCTEXT("CopyToDeviceRequiresCookByTheBookText", "This mode requires 'By The Book' cooking"))
							]
					]
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				// file server settings area
				SNew(SSessionLauncherDeployFileServerSettings, InModel)
					.Visibility(this, &SSessionLauncherDeployPage::HandleDeployFileServerSettingsVisibility)
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				// deploy to devices settings area
				SNew(SSessionLauncherDeployToDeviceSettings, InModel)
					.Visibility(this, &SSessionLauncherDeployPage::HandleDeployToDeviceSettingsVisibility)
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				// deploy repository to devices settings area
				SNew(SSessionLauncherDeployRepositorySettings, InModel)
					.Visibility(this, &SSessionLauncherDeployPage::HandleDeployRepositorySettingsVisibility)
			]
	];

	Model->OnProfileSelected().AddSP(this, &SSessionLauncherDeployPage::HandleProfileManagerProfileSelected);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SSessionLauncherDevicesPage callbacks
 *****************************************************************************/

FString SSessionLauncherDeployPage::HandleDeploymentModeComboButtonContentText( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		ELauncherProfileDeploymentModes::Type DeploymentMode = SelectedProfile->GetDeploymentMode();

		if (DeploymentMode == ELauncherProfileDeploymentModes::CopyToDevice)
		{
			return TEXT("Copy to device");
		}

		if (DeploymentMode == ELauncherProfileDeploymentModes::DoNotDeploy)
		{
			return TEXT("Do not deploy");
		}

		if (DeploymentMode == ELauncherProfileDeploymentModes::FileServer)
		{
			return TEXT("File server");
		}

		if (DeploymentMode == ELauncherProfileDeploymentModes::CopyRepository)
		{
			return TEXT("Copy repository");
		}

		return LOCTEXT("DeploymentModeComboButtonDefaultText", "Select...").ToString();
	}

	return TEXT("");
}


void SSessionLauncherDeployPage::HandleDeploymentModeMenuEntryClicked( ELauncherProfileDeploymentModes::Type DeploymentMode )
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		SelectedProfile->SetDeploymentMode(DeploymentMode);
	}
}


EVisibility SSessionLauncherDeployPage::HandleDeployFileServerSettingsVisibility( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->GetDeploymentMode() == ELauncherProfileDeploymentModes::FileServer)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


EVisibility SSessionLauncherDeployPage::HandleDeployToDeviceSettingsVisibility( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->GetDeploymentMode() == ELauncherProfileDeploymentModes::CopyToDevice)
		{
			if (!SelectedProfile->HasValidationError(ELauncherProfileValidationErrors::CopyToDeviceRequiresCookByTheBook))
			{
				return EVisibility::Visible;
			}
		}
	}

	return EVisibility::Collapsed;
}


EVisibility SSessionLauncherDeployPage::HandleDeployRepositorySettingsVisibility( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->GetDeploymentMode() == ELauncherProfileDeploymentModes::CopyRepository)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


void SSessionLauncherDeployPage::HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile )
{

}


EVisibility SSessionLauncherDeployPage::HandleValidationErrorIconVisibility( ELauncherProfileValidationErrors::Type Error ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->HasValidationError(Error))
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


#undef LOCTEXT_NAMESPACE
