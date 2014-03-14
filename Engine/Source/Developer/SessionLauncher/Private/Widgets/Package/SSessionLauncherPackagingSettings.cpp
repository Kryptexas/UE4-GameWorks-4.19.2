// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherPackagingSettings.cpp: Implements the SSessionLauncherPackagingSettings class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherPackagingSettings"


/* SSessionLauncherPackagingSettings structors
 *****************************************************************************/

SSessionLauncherPackagingSettings::~SSessionLauncherPackagingSettings( )
{
	if (Model.IsValid())
	{
		Model->OnProfileSelected().RemoveAll(this);
	}
}


/* SSessionLauncherPackagingSettings interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSessionLauncherPackagingSettings::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel )
{
	Model = InModel;

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.FillHeight(1.0)
			[
				SNew(SBorder)
					.Padding(8.0)
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
									.Text(LOCTEXT("RepositoryPathLabel", "Repository Path:").ToString())
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0, 4.0, 0.0, 0.0)
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
									.FillWidth(1.0)
									.Padding(0.0, 0.0, 0.0, 3.0)
									[
										// repository path text box
										SAssignNew(RepositoryPathTextBox, SEditableTextBox)
									]

								+ SHorizontalBox::Slot()
									.AutoWidth()
									.HAlign(HAlign_Right)
									.Padding(4.0, 0.0, 0.0, 0.0)
									[
										// browse button
										SNew(SButton)
											.ContentPadding(FMargin(6.0, 2.0))
											.IsEnabled(false)
											.Text(LOCTEXT("BrowseButtonText", "Browse...").ToString())
											.ToolTipText(LOCTEXT("BrowseButtonToolTip", "Browse for the repository").ToString())
											//.OnClicked(this, &SSessionLauncherPackagePage::HandleBrowseButtonClicked)
									]
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0, 8.0, 0.0, 0.0)
							[
								SNew(STextBlock)
									.Text(LOCTEXT("DescriptionTextBoxLabel", "Description:").ToString())
							]

						+ SVerticalBox::Slot()
							.FillHeight(1.0)
							.Padding(0.0, 4.0, 0.0, 0.0)
							[
								SNew(SEditableTextBox)
							]
					]
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0, 8.0, 0.0, 0.0)
			[
				SNew(SExpandableArea)
					.AreaTitle(LOCTEXT("IncrementalPackagingAreaTitle", "Incremental Packaging").ToString())
					.InitiallyCollapsed(true)
					.Padding(8.0)
					.BodyContent()
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								// incremental packaging check box
								SNew(SCheckBox)
									//.IsChecked(this, &SSessionLauncherLaunchPage::HandleIncrementalCheckBoxIsChecked)
									//.OnCheckStateChanged(this, &SSessionLauncherLaunchPage::HandleIncrementalCheckBoxCheckStateChanged)
									.Content()
									[
										SNew(STextBlock)
											.Text(LOCTEXT("IncrementalCheckBoxText", "This is an incremental build").ToString())
									]
							]

						+ SVerticalBox::Slot()
							.FillHeight(1.0)
							.Padding(18.0, 12.0, 0.0, 0.0)
							[
								SNew(SVerticalBox)
							]
					]
			]
	];

	Model->OnProfileSelected().AddSP(this, &SSessionLauncherPackagingSettings::HandleProfileManagerProfileSelected);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SSessionLauncherPackagingSettings callbacks
 *****************************************************************************/

void SSessionLauncherPackagingSettings::HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile )
{

}


#undef LOCTEXT_NAMESPACE
