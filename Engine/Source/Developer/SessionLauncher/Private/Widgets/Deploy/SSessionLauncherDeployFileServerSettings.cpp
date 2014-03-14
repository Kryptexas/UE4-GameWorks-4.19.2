// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherDeployFileServerSettings.cpp: Implements the SSessionLauncherDeployFileServerSettings class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherDeployFileServerSettings"


/* SSessionLauncherDeployFileServerSettings interface
 *****************************************************************************/

void SSessionLauncherDeployFileServerSettings::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel )
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
						SNew(SSessionLauncherDeployTargets, InModel)
					]
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
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
								// hide file server check box
								SNew(SCheckBox)
									.IsChecked(this, &SSessionLauncherDeployFileServerSettings::HandleHideWindowCheckBoxIsChecked)
									.OnCheckStateChanged(this, &SSessionLauncherDeployFileServerSettings::HandleHideWindowCheckBoxCheckStateChanged)
									.Padding(FMargin(4.0f, 0.0f))
									.ToolTipText(LOCTEXT("HideWindowCheckBoxTooltip", "If checked, the file server's console window will be hidden from your desktop.").ToString())
									.Content()
									[
										SNew(STextBlock)
											.Text(LOCTEXT("HideWindowCheckBoxText", "Hide the file server's console window").ToString())
									]
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 4.0f, 0.0f, 0.0f)
							[
								// streaming file server check box
								SNew(SCheckBox)
									.IsChecked(this, &SSessionLauncherDeployFileServerSettings::HandleStreamingServerCheckBoxIsChecked)
									.OnCheckStateChanged(this, &SSessionLauncherDeployFileServerSettings::HandleStreamingServerCheckBoxCheckStateChanged)
									.Padding(FMargin(4.0f, 0.0f))
									.ToolTipText(LOCTEXT("StreamingServerCheckBoxTooltip", "If checked, the file server uses an experimental implementation that can serve multiple files simultaneously.").ToString())
									.Content()
									[
										SNew(STextBlock)
											.Text(LOCTEXT("StreamingServerCheckBoxText", "Streaming server (experimental)").ToString())
									]
							]
					]
			]
	];
}


/* SSessionLauncherDeployFileServerSettings callbacks
 *****************************************************************************/

void SSessionLauncherDeployFileServerSettings::HandleHideWindowCheckBoxCheckStateChanged( ESlateCheckBoxState::Type NewState )
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		SelectedProfile->SetHideFileServerWindow(NewState == ESlateCheckBoxState::Checked);
	}
}


ESlateCheckBoxState::Type SSessionLauncherDeployFileServerSettings::HandleHideWindowCheckBoxIsChecked( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->IsFileServerHidden())
		{
			return ESlateCheckBoxState::Checked;
		}
	}

	return ESlateCheckBoxState::Unchecked;
}


void SSessionLauncherDeployFileServerSettings::HandleStreamingServerCheckBoxCheckStateChanged( ESlateCheckBoxState::Type NewState )
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		SelectedProfile->SetStreamingFileServer(NewState == ESlateCheckBoxState::Checked);
	}
}


ESlateCheckBoxState::Type SSessionLauncherDeployFileServerSettings::HandleStreamingServerCheckBoxIsChecked( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->IsFileServerStreaming())
		{
			return ESlateCheckBoxState::Checked;
		}
	}

	return ESlateCheckBoxState::Unchecked;
}


#undef LOCTEXT_NAMESPACE
