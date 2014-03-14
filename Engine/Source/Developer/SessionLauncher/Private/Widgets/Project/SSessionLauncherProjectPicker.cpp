// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherUnrealProjectPicker.cpp: Implements the SSessionLauncherUnrealProjectPicker class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherUnrealProjectPicker"


/* SSessionLauncherProjectPicker structors
 *****************************************************************************/

SSessionLauncherProjectPicker::~SSessionLauncherProjectPicker( )
{
	if (Model.IsValid())
	{
		Model->OnProfileSelected().RemoveAll(this);
	}
}


/* SSessionLauncherProjectPicker interface
 *****************************************************************************/

void SSessionLauncherProjectPicker::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel, bool InShowConfig )
{
	Model = InModel;

	ChildSlot
	[
		InShowConfig ? MakeProjectAndConfigWidget() : MakeProjectWidget()
	];

	Model->OnProfileSelected().AddSP(this, &SSessionLauncherProjectPicker::HandleProfileManagerProfileSelected);
}


/* SSessionLauncherProjectPicker implementation
 *****************************************************************************/

TSharedRef<SWidget> SSessionLauncherProjectPicker::MakeProjectMenuWidget( )
{
	FMenuBuilder MenuBuilder(true, NULL);
	{
		const TArray<FString>& AvailableGames = FGameProjectHelper::GetAvailableGames();

		for (int32 Index = 0; Index < AvailableGames.Num(); ++Index)
		{
			FString ProjectPath = FPaths::RootDir() / AvailableGames[Index] / AvailableGames[Index] + TEXT(".uproject");
			FUIAction ProjectAction(FExecuteAction::CreateSP(this, &SSessionLauncherProjectPicker::HandleProjectMenuEntryClicked, ProjectPath));
			MenuBuilder.AddMenuEntry( FText::FromString( AvailableGames[Index] ), FText::FromString( ProjectPath ), FSlateIcon(), ProjectAction);
		}

		MenuBuilder.AddMenuSeparator();

		FUIAction BrowseAction(FExecuteAction::CreateSP(this, &SSessionLauncherProjectPicker::HandleProjectMenuEntryClicked, FString()));
		MenuBuilder.AddMenuEntry(LOCTEXT("BrowseAction", "Browse..."), LOCTEXT("BrowseActionHint", "Browse for a project on your computer"), FSlateIcon(), BrowseAction);
	}

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SSessionLauncherProjectPicker::MakeProjectWidget()
{
	TSharedRef<SWidget> Widget = SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.FillHeight(0.5)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(8.0)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSessionLauncherFormLabel)
					.ErrorToolTipText(NSLOCTEXT("SSessionLauncherBuildValidation", "NoProjectSelectedError", "A Project must be selected."))
					.ErrorVisibility(this, &SSessionLauncherProjectPicker::HandleValidationErrorIconVisibility, ELauncherProfileValidationErrors::NoProjectSelected)
					.LabelText(LOCTEXT("ProjectComboBoxLabel", "Project:"))
				]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						// project selector
						SNew(SComboButton)
						.ButtonContent()
						[
							SNew(STextBlock)
							.Text(this, &SSessionLauncherProjectPicker::HandleProjectComboButtonText)
						]
						.ContentPadding(FMargin(6.0f, 2.0f))
							.MenuContent()
							[
								MakeProjectMenuWidget()
							]
						.ToolTipText(this, &SSessionLauncherProjectPicker::HandleProjectComboButtonToolTip)
					]
			]
		];

	return Widget;
}

TSharedRef<SWidget> SSessionLauncherProjectPicker::MakeProjectAndConfigWidget()
{
	TSharedRef<SWidget> Widget = SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.FillHeight(0.5)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(8.0)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSessionLauncherFormLabel)
					.ErrorToolTipText(NSLOCTEXT("SSessionLauncherBuildValidation", "NoProjectSelectedError", "A Project must be selected."))
					.ErrorVisibility(this, &SSessionLauncherProjectPicker::HandleValidationErrorIconVisibility, ELauncherProfileValidationErrors::NoProjectSelected)
					.LabelText(LOCTEXT("ProjectComboBoxLabel", "Project:"))
				]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						// project selector
						SNew(SComboButton)
						.ButtonContent()
						[
							SNew(STextBlock)
							.Text(this, &SSessionLauncherProjectPicker::HandleProjectComboButtonText)
						]
						.ContentPadding(FMargin(6.0f, 2.0f))
							.MenuContent()
							[
								MakeProjectMenuWidget()
							]
						.ToolTipText(this, &SSessionLauncherProjectPicker::HandleProjectComboButtonToolTip)
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(SSessionLauncherFormLabel)
						.ErrorToolTipText(NSLOCTEXT("SSessionLauncherBuildValidation", "NoBuildConfigurationSelectedError", "A Build Configuration must be selected."))
						.ErrorVisibility(this, &SSessionLauncherProjectPicker::HandleValidationErrorIconVisibility, ELauncherProfileValidationErrors::NoBuildConfigurationSelected)
						.LabelText(LOCTEXT("ConfigurationComboBoxLabel", "Build Configuration:"))
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						// build configuration selector
						SNew(SSessionLauncherBuildConfigurationSelector)
						.OnConfigurationSelected(this, &SSessionLauncherProjectPicker::HandleBuildConfigurationSelectorConfigurationSelected)
						.Text(this, &SSessionLauncherProjectPicker::HandleBuildConfigurationSelectorText)
					]
			]
		];

	return Widget;
}

/* SSessionLauncherProjectPicker callbacks
 *****************************************************************************/

void SSessionLauncherProjectPicker::HandleBuildConfigurationSelectorConfigurationSelected( EBuildConfigurations::Type Configuration )
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		SelectedProfile->SetBuildConfiguration(Configuration);
	}
}


FString SSessionLauncherProjectPicker::HandleBuildConfigurationSelectorText( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		return EBuildConfigurations::ToString(SelectedProfile->GetBuildConfiguration());
	}

	return FString();
}


void SSessionLauncherProjectPicker::HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile )
{

}


FString SSessionLauncherProjectPicker::HandleProjectComboButtonText( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		FString ProjectName = SelectedProfile->GetProjectName();

		if (!ProjectName.IsEmpty())
		{
			return ProjectName;
		}
	}

	return LOCTEXT("SelectProfileText", "Select...").ToString();
}


FString SSessionLauncherProjectPicker::HandleProjectComboButtonToolTip( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		const FString& SelectedProjectPath = SelectedProfile->GetProjectPath();

		if (!SelectedProjectPath.IsEmpty())
		{
			return SelectedProjectPath;
		}
	}

	return LOCTEXT("SelectProfileTooltip", "Select or browse for a profile").ToString();
}


void SSessionLauncherProjectPicker::HandleProjectMenuEntryClicked( FString ProjectPath )
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (!SelectedProfile.IsValid())
	{
		return;
	}

	if (ProjectPath.IsEmpty())
	{
		TArray<FString> OutFiles;

		FString DefaultPath = FPaths::GetPath(SelectedProfile->GetProjectPath());

		if (DefaultPath.IsEmpty())
		{
			DefaultPath = FPaths::RootDir();
		}

		if (FDesktopPlatformModule::Get()->OpenFileDialog(NULL, LOCTEXT("SelectProjectDialogTitle", "Select a project").ToString(), DefaultPath, TEXT(""), TEXT("Project files (*.uproject)|*.uproject"), EFileDialogFlags::None, OutFiles))
		{
			SelectedProfile->SetProjectPath(OutFiles[0]);
		}
	}
	else
	{
		SelectedProfile->SetProjectPath(ProjectPath);
	}
}


EVisibility SSessionLauncherProjectPicker::HandleValidationErrorIconVisibility( ELauncherProfileValidationErrors::Type Error ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->HasValidationError(Error))
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Hidden;
}


#undef LOCTEXT_NAMESPACE
