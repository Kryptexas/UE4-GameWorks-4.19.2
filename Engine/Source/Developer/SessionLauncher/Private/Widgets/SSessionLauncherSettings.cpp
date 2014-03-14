// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherSettings.cpp: Implements the SSessionLauncherSettings class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherSettings"


/* SSessionLauncherSettings structors
 *****************************************************************************/

SSessionLauncherSettings::~SSessionLauncherSettings( )
{

}


/* SSessionLauncherSettings interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSessionLauncherSettings::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel )
{
	Model = InModel;

	ChildSlot
	[
		SNew(SVerticalBox)

		+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.Padding(FMargin(8.0f, 6.0f, 8.0f, 4.0f))
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					// profile selector
					SNew(SSessionLauncherProfileSelector, InModel)
				]
			]

		+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(8.0f, 8.0f, 0.0f, 8.0f)
			[
				SNew(SOverlay)

				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				[
					SNew(SScrollBox)
					.Visibility(this, &SSessionLauncherSettings::HandleSettingsScrollBoxVisibility)

					+ SScrollBox::Slot()
					.Padding(0.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(SGridPanel)
						.FillColumn(1, 1.0f)

						// project section
						+ SGridPanel::Slot(0, 0)
						.Padding(8.0f, 0.0f, 0.0f, 0.0f)
						.VAlign(VAlign_Top)
						[
							SNew(STextBlock)
							.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 13))
							.Text(LOCTEXT("ProjectSectionHeader", "Project").ToString())
						]

						+ SGridPanel::Slot(1, 0)
							.Padding(32.0f, 0.0f, 8.0f, 0.0f)
							[
								SNew(SSessionLauncherProjectPage, InModel)
							]

						// build section
						+ SGridPanel::Slot(0, 1)
							.ColumnSpan(3)
							.Padding(0.0f, 16.0f)
							[
								SNew(SSeparator)
								.Orientation(Orient_Horizontal)
							]

						+ SGridPanel::Slot(0, 2)
							.Padding(8.0f, 0.0f, 0.0f, 0.0f)
							.VAlign(VAlign_Top)
							[
								SNew(STextBlock)
								.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 13))
								.Text(LOCTEXT("BuildSectionHeader", "Build").ToString())
							]

						+ SGridPanel::Slot(1, 2)
							.Padding(32.0f, 0.0f, 8.0f, 0.0f)
							[
								SNew(SSessionLauncherBuildPage, InModel)
							]

						// cook section
						+ SGridPanel::Slot(0, 3)
							.ColumnSpan(3)
							.Padding(0.0f, 16.0f)
							[
								SNew(SSeparator)
								.Orientation(Orient_Horizontal)
							]

						+ SGridPanel::Slot(0, 4)
							.Padding(8.0f, 0.0f, 0.0f, 0.0f)
							.VAlign(VAlign_Top)
							[
								SNew(STextBlock)
								.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 13))
								.Text(LOCTEXT("CookSectionHeader", "Cook").ToString())
							]

						+ SGridPanel::Slot(1, 4)
							.Padding(32.0f, 0.0f, 8.0f, 0.0f)
							[
								SNew(SSessionLauncherCookPage, InModel)
							]

						// package section
						+ SGridPanel::Slot(0, 5)
							.ColumnSpan(3)
							.Padding(0.0f, 16.0f)
							[
								SNew(SSeparator)
								.Orientation(Orient_Horizontal)
							]

						+ SGridPanel::Slot(0, 6)
							.VAlign(VAlign_Top)
							[
								SNew(STextBlock)
								.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 13))
								.Text(LOCTEXT("PackageSectionHeader", "Package").ToString())
							]

						+ SGridPanel::Slot(1, 6)
							.Padding(32.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(SSessionLauncherPackagePage, InModel)
							]

						// deploy section
						+ SGridPanel::Slot(0, 7)
							.ColumnSpan(3)
							.Padding(0.0f, 16.0f)
							[
								SNew(SSeparator)
								.Orientation(Orient_Horizontal)
							]

						+ SGridPanel::Slot(0, 8)
							.Padding(8.0f, 0.0f, 0.0f, 0.0f)
							.VAlign(VAlign_Top)
							[
								SNew(STextBlock)
								.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 13))
								.Text(LOCTEXT("DeploySectionHeader", "Deploy").ToString())
							]

						+ SGridPanel::Slot(1, 8)
							.Padding(32.0f, 0.0f, 8.0f, 0.0f)
							[
								SNew(SSessionLauncherDeployPage, InModel)
							]

						// launch section
						+ SGridPanel::Slot(0, 9)
							.ColumnSpan(3)
							.Padding(0.0f, 16.0f)
							[
								SNew(SSeparator)
								.Orientation(Orient_Horizontal)
							]

						+ SGridPanel::Slot(0, 10)
							.Padding(8.0f, 0.0f, 0.0f, 0.0f)
							.VAlign(VAlign_Top)
							[
								SNew(STextBlock)
								.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 13))
								.Text(LOCTEXT("LaunchSectionHeader", "Launch").ToString())
							]

						+ SGridPanel::Slot(1, 10)
							.HAlign(HAlign_Fill)
							.Padding(32.0f, 0.0f, 8.0f, 0.0f)
							[
								SNew(SSessionLauncherLaunchPage, InModel)
							]
					]
				]

				+ SOverlay::Slot()
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SelectProfileText", "Select or create a new profile to continue.").ToString())
						.Visibility(this, &SSessionLauncherSettings::HandleSelectProfileTextBlockVisibility)
					]
			]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SSessionLauncherSettings callbacks
*****************************************************************************/

EVisibility SSessionLauncherSettings::HandleSelectProfileTextBlockVisibility( ) const
{
	if (Model->GetSelectedProfile().IsValid())
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}


EVisibility SSessionLauncherSettings::HandleSettingsScrollBoxVisibility( ) const
{
	if (Model->GetSelectedProfile().IsValid())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}


#undef LOCTEXT_NAMESPACE
