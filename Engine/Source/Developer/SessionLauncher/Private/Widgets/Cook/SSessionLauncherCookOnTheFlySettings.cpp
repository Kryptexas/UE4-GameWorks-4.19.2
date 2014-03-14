// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherCookOnTheFlySettings.cpp: Implements the SSessionLauncherCookOnTheFlySettings class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherCookOnTheFlySettings"


/* SSessionLauncherCookOnTheFlySettings structors
 *****************************************************************************/

SSessionLauncherCookOnTheFlySettings::~SSessionLauncherCookOnTheFlySettings( )
{
	if (Model.IsValid())
	{
		Model->OnProfileSelected().RemoveAll(this);
	}
}


/* SSessionLauncherCookOnTheFlySettings interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSessionLauncherCookOnTheFlySettings::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel )
{
	Model = InModel;

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SExpandableArea)
					.AreaTitle(LOCTEXT("AdvancedAreaTitle", "Advanced Settings"))
					.InitiallyCollapsed(true)
					.Padding(8.0)
					.BodyContent()
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								// incremental cook check box
								SNew(SCheckBox)
									.IsChecked(this, &SSessionLauncherCookOnTheFlySettings::HandleIncrementalCheckBoxIsChecked)
									.OnCheckStateChanged(this, &SSessionLauncherCookOnTheFlySettings::HandleIncrementalCheckBoxCheckStateChanged)
									.Padding(FMargin(4.0f, 0.0f))
									.ToolTipText(LOCTEXT("IncrementalCheckBoxTooltip", "If checked, only modified content will be cooked, resulting in much faster cooking times. It is recommended to enable this option whenever possible."))
									.Content()
									[
										SNew(STextBlock)
											.Text(LOCTEXT("IncrementalCheckBoxText", "Only cook modified content"))
									]
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 12.0f, 0.0f, 0.0f)
							[
								SNew(SSessionLauncherFormLabel)
									.LabelText(LOCTEXT("CookerOptionsTextBoxLabel", "Additional Cooker Options:"))
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0, 4.0, 0.0, 0.0)
							[
								// cooker command line options
								SNew(SEditableTextBox)
							]
					]
			]
	];

	Model->OnProfileSelected().AddSP(this, &SSessionLauncherCookOnTheFlySettings::HandleProfileManagerProfileSelected);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SSessionLauncherCookOnTheFlySettings callbacks
 *****************************************************************************/

void SSessionLauncherCookOnTheFlySettings::HandleIncrementalCheckBoxCheckStateChanged( ESlateCheckBoxState::Type NewState )
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		SelectedProfile->SetIncrementalCooking(NewState == ESlateCheckBoxState::Checked);
	}
}


ESlateCheckBoxState::Type SSessionLauncherCookOnTheFlySettings::HandleIncrementalCheckBoxIsChecked( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->IsCookingIncrementally())
		{
			return ESlateCheckBoxState::Checked;
		}
	}

	return ESlateCheckBoxState::Unchecked;
}


void SSessionLauncherCookOnTheFlySettings::HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile )
{

}


EVisibility SSessionLauncherCookOnTheFlySettings::HandleValidationErrorIconVisibility( ELauncherProfileValidationErrors::Type Error ) const
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
