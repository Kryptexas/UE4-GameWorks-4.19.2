// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherValidation.h: Declares the SSessionLauncherValidation class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SSessionLauncherBuildValidation"


/**
 * Implements the launcher's profile validation panel.
 */
class SSessionLauncherValidation
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherValidation) { }
	SLATE_END_ARGS()


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel )
	{
		Model = InModel;

		ChildSlot
		[
			SNew(SVerticalBox)
			
			// build settings
			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeValidationMessage(TEXT("Icons.Error"), LOCTEXT("NoBuildGameSelectedError", "A Project must be selected.").ToString(), ELauncherProfileValidationErrors::NoProjectSelected)
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeValidationMessage(TEXT("Icons.Error"), LOCTEXT("NoBuildConfigurationSelectedError", "A Build Configuration must be selected.").ToString(), ELauncherProfileValidationErrors::NoBuildConfigurationSelected)
				]

			// cook settings
			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeValidationMessage(TEXT("Icons.Error"), LOCTEXT("NoCookedPlatformSelectedError", "At least one Platform must be selected when cooking by the book.").ToString(), ELauncherProfileValidationErrors::NoPlatformSelected)
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeValidationMessage(TEXT("Icons.Error"), LOCTEXT("NoCookedCulturesSelectedError", "At least one Culture must be selected when cooking by the book.").ToString(), ELauncherProfileValidationErrors::NoCookedCulturesSelected)
				]

			// packaging settings

			// deployment settings
			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeValidationMessage(TEXT("Icons.Error"), LOCTEXT("CopyToDeviceRequiresCookByTheBookError", "Deployment by copying to device requires 'By The Book' cooking.").ToString(), ELauncherProfileValidationErrors::CopyToDeviceRequiresCookByTheBook)
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeValidationMessage(TEXT("Icons.Error"), LOCTEXT("DeployedDeviceGroupRequired", "A device group must be selected when deploying builds.").ToString(), ELauncherProfileValidationErrors::DeployedDeviceGroupRequired)
				]

			// launch settings
			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeValidationMessage(TEXT("Icons.Error"), LOCTEXT("CustomRolesNotSupportedYet", "Custom launch roles are not supported yet.").ToString(), ELauncherProfileValidationErrors::CustomRolesNotSupportedYet)
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeValidationMessage(TEXT("Icons.Error"), LOCTEXT("InitialCultureNotAvailable", "The Initial Culture selected for launch is not in the build.").ToString(), ELauncherProfileValidationErrors::InitialCultureNotAvailable)
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeValidationMessage(TEXT("Icons.Error"), LOCTEXT("InitialMapNotAvailable", "The Initial Map selected for launch is not in the build.").ToString(), ELauncherProfileValidationErrors::InitialMapNotAvailable)
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeValidationMessage(TEXT("Icons.Error"), LOCTEXT("NoLaunchRoleDeviceAssigned", "One or more launch roles do not have a device assigned.").ToString(), ELauncherProfileValidationErrors::NoLaunchRoleDeviceAssigned)
				]
		];
	}


protected:

	/**
	 * Creates a widget for a validation message.
	 *
	 * @param IconName - The name of the message icon.
	 * @param MessageText - The message text.
	 * @param MessageType - The message type.
	 */
	TSharedRef<SWidget> MakeValidationMessage( const TCHAR* IconName, const FString& MessageText, ELauncherProfileValidationErrors::Type Message )
	{
		return SNew(SHorizontalBox)
			.Visibility(this, &SSessionLauncherValidation::HandleValidationMessageVisibility, Message)

		+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0)
			[
				SNew(SImage)
					.Image(FEditorStyle::GetBrush(IconName))
			]

		+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
					.Text(MessageText)
			];
	}


private:

	// Callback for getting the visibility state of a validation message.
	EVisibility HandleValidationMessageVisibility( ELauncherProfileValidationErrors::Type Error ) const
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


private:

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;
};


#undef LOCTEXT_NAMESPACE
