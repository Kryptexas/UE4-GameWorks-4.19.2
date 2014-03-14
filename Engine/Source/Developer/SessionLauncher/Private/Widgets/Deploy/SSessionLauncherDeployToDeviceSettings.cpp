// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherDeployToDeviceSettings.cpp: Implements the SSessionLauncherDeployToDeviceSettings class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherDeployToDeviceSettings"


/* SSessionLauncherDeployToDeviceSettings interface
 *****************************************************************************/

void SSessionLauncherDeployToDeviceSettings::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel, EVisibility InShowAdvanced )
{
	Model = InModel;

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
					.Padding(8.0f)
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					[
						// deploy targets area
						SNew(SSessionLauncherDeployTargets, InModel, InShowAdvanced != EVisibility::Visible)
					]
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SVerticalBox)
				.Visibility(InShowAdvanced)

				+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SExpandableArea)
							.AreaTitle(LOCTEXT("AdvancedAreaTitle", "Advanced Settings").ToString())
							.InitiallyCollapsed(true)
							.Padding(8.0f)
							.BodyContent()
							[
								SNew(SVerticalBox)

								+ SVerticalBox::Slot()
									.AutoHeight()
									[
										// unreal pak check box
										SNew(SCheckBox)
											.IsChecked(this, &SSessionLauncherDeployToDeviceSettings::HandleUnrealPakCheckBoxIsChecked)
											.OnCheckStateChanged(this, &SSessionLauncherDeployToDeviceSettings::HandleUnrealPakCheckBoxCheckStateChanged)
											.Padding(FMargin(4.0f, 0.0f))
											.ToolTipText(LOCTEXT("UnrealPakCheckBoxTooltip", "If checked, the content will be deployed as a single UnrealPak file instead of many separate files.").ToString())
											.Content()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("UnrealPakCheckBoxText", "Store all content in a single file (UnrealPak)").ToString())
											]
									]
							]
					]
			]
	];
}


/* SSessionLauncherDeployToDeviceSettings callbacks
 *****************************************************************************/

void SSessionLauncherDeployToDeviceSettings::HandleUnrealPakCheckBoxCheckStateChanged( ESlateCheckBoxState::Type NewState )
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		SelectedProfile->SetDeployWithUnrealPak(NewState == ESlateCheckBoxState::Checked);
	}
}


ESlateCheckBoxState::Type SSessionLauncherDeployToDeviceSettings::HandleUnrealPakCheckBoxIsChecked( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->IsPackingWithUnrealPak())
		{
			return ESlateCheckBoxState::Checked;
		}
	}

	return ESlateCheckBoxState::Unchecked;
}


#undef LOCTEXT_NAMESPACE
