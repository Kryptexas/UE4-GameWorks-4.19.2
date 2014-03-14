// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherLaunchPage.cpp: Implements the SSessionLauncherLaunchPage class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherLaunchPage"


/* SSessionLauncherLaunchPage structors
 *****************************************************************************/

SSessionLauncherLaunchPage::~SSessionLauncherLaunchPage( )
{
	if (Model.IsValid())
	{
		Model->OnProfileSelected().RemoveAll(this);
	}
}


/* SSessionLauncherLaunchPage interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSessionLauncherLaunchPage::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel )
{
	Model = InModel;

	// create launch modes menu
	FMenuBuilder LaunchModeMenuBuilder(true, NULL);
	{
		FUIAction DefaultRoleAction(FExecuteAction::CreateSP(this, &SSessionLauncherLaunchPage::HandleLaunchModeMenuEntryClicked, ELauncherProfileLaunchModes::DefaultRole));
		LaunchModeMenuBuilder.AddMenuEntry(LOCTEXT("DefaultRoleAction", "Using default role"), LOCTEXT("DefaultRoleActionHint", "Launch with the default role on all deployed devices."), FSlateIcon(), DefaultRoleAction);
		
		FUIAction CustomRolesAction(FExecuteAction::CreateSP(this, &SSessionLauncherLaunchPage::HandleLaunchModeMenuEntryClicked, ELauncherProfileLaunchModes::CustomRoles));
		LaunchModeMenuBuilder.AddMenuEntry(LOCTEXT("CustomRolesAction", "Using custom roles"), LOCTEXT("CustomRolesActionHint", "Launch with per-device custom roles."), FSlateIcon(), CustomRolesAction);

		FUIAction DoNotLaunchAction(FExecuteAction::CreateSP(this, &SSessionLauncherLaunchPage::HandleLaunchModeMenuEntryClicked, ELauncherProfileLaunchModes::DoNotLaunch));
		LaunchModeMenuBuilder.AddMenuEntry(LOCTEXT("DoNotCookAction", "Do not launch"), LOCTEXT("DoNotCookActionHint", "Do not launch the build at this time."), FSlateIcon(), DoNotLaunchAction);
	}

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
					.Visibility(this, &SSessionLauncherLaunchPage::HandleLaunchModeBoxVisibility)

				+ SHorizontalBox::Slot()
					.FillWidth(1.0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("HowToLaunchText", "How would you like to launch?"))
					]

				+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(8.0, 0.0, 0.0, 0.0)
					[
						// launch mode menu
						SNew(SComboButton)
							.ButtonContent()
							[
								SNew(STextBlock)
									.Text(this, &SSessionLauncherLaunchPage::HandleLaunchModeComboButtonContentText)
							]
							.ContentPadding(FMargin(6.0, 2.0))
							.MenuContent()
							[
								LaunchModeMenuBuilder.MakeWidget()
							]
					]
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(8.0)
					.Visibility(this, &SSessionLauncherLaunchPage::HandleValidationErrorIconVisibility, ELauncherProfileValidationErrors::CustomRolesNotSupportedYet)
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
									.Text(LOCTEXT("CopyToDeviceRequiresCookByTheBookText", "Custom launch roles are not supported yet."))
							]
					]
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0, 8.0, 0.0, 0.0)
			[
				SNew(SExpandableArea)
					.AreaTitle(LOCTEXT("DefaultRoleAreaTitle", "Default Role"))
					.InitiallyCollapsed(false)
					.Padding(8.0)
					.Visibility(this, &SSessionLauncherLaunchPage::HandleLaunchSettingsVisibility)
					.BodyContent()
					[
						// launch settings area
						SAssignNew(DefaultRoleEditor, SSessionLauncherLaunchRoleEditor)
							.AvailableCultures(&AvailableCultures)
							.AvailableMaps(&AvailableMaps)
					]
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0, 4.0, 0.0, 0.0)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("CannotLaunchText", "The build is not being deployed and cannot be launched."))
					.Visibility(this, &SSessionLauncherLaunchPage::HandleCannotLaunchTextBlockVisibility)
			]
	];

	Model->OnProfileSelected().AddSP(this, &SSessionLauncherLaunchPage::HandleProfileManagerProfileSelected);

	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();
	if (SelectedProfile.IsValid())
	{
		SelectedProfile->OnProjectChanged().AddSP(this, &SSessionLauncherLaunchPage::HandleProfileProjectChanged);
	}

	Refresh();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


void SSessionLauncherLaunchPage::Refresh( )
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->GetCookMode() == ELauncherProfileCookModes::ByTheBook)
		{
			AvailableCultures = SelectedProfile->GetCookedCultures();
		}
		else
		{
			FInternationalization::GetCultureNames(AvailableCultures);
		}

		AvailableMaps = FGameProjectHelper::GetAvailableMaps(SelectedProfile->GetProjectBasePath(), SelectedProfile->SupportsEngineMaps(), true);

		DefaultRoleEditor->Refresh(SelectedProfile->GetDefaultLaunchRole());
	}
	else
	{
		AvailableCultures.Reset();
		AvailableMaps.Reset();

		DefaultRoleEditor->Refresh(NULL);
	}
}


/* SSessionLauncherLaunchPage callbacks
 *****************************************************************************/

EVisibility SSessionLauncherLaunchPage::HandleCannotLaunchTextBlockVisibility( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->GetDeploymentMode() == ELauncherProfileDeploymentModes::DoNotDeploy)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


EVisibility SSessionLauncherLaunchPage::HandleLaunchModeBoxVisibility( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->GetDeploymentMode() != ELauncherProfileDeploymentModes::DoNotDeploy)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


FString SSessionLauncherLaunchPage::HandleLaunchModeComboButtonContentText( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		ELauncherProfileLaunchModes::Type LaunchMode = SelectedProfile->GetLaunchMode();

		if (LaunchMode == ELauncherProfileLaunchModes::CustomRoles)
		{
			return LOCTEXT("LaunchModeComboButtonCustomRolesText", "Using custom roles").ToString();
		}

		if (LaunchMode == ELauncherProfileLaunchModes::DefaultRole)
		{
			return LOCTEXT("LaunchModeComboButtonDefaultRoleText", "Using default role").ToString();
		}

		if (LaunchMode == ELauncherProfileLaunchModes::DoNotLaunch)
		{
			return LOCTEXT("LaunchModeComboButtonDoNotLaunchText", "Do not launch").ToString();
		}

		return LOCTEXT("LaunchModeComboButtonSelectText", "Select...").ToString();
	}

	return TEXT("");
}


void SSessionLauncherLaunchPage::HandleLaunchModeMenuEntryClicked( ELauncherProfileLaunchModes::Type LaunchMode )
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		SelectedProfile->SetLaunchMode(LaunchMode);
	}
}


EVisibility SSessionLauncherLaunchPage::HandleLaunchSettingsVisibility( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->GetDeploymentMode() != ELauncherProfileDeploymentModes::DoNotDeploy)
		{
			if (SelectedProfile->GetLaunchMode() == ELauncherProfileLaunchModes::DefaultRole)
			{
				return EVisibility::Visible;
			}
		}
	}

	return EVisibility::Collapsed;
}


void SSessionLauncherLaunchPage::HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile )
{
	if (PreviousProfile.IsValid())
	{
		PreviousProfile->OnProjectChanged().RemoveAll(this);
	}
	if (SelectedProfile.IsValid())
	{
		SelectedProfile->OnProjectChanged().AddSP(this, &SSessionLauncherLaunchPage::HandleProfileProjectChanged);
	}
	Refresh();
}

void SSessionLauncherLaunchPage::HandleProfileProjectChanged()
{
	Refresh();
}


EVisibility SSessionLauncherLaunchPage::HandleValidationErrorIconVisibility( ELauncherProfileValidationErrors::Type Error ) const
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
