// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherDeployTaskSettings.cpp: Implements the SSessionLauncherDeployTaskSettings class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherDeployTaskSettings"


/* SSessionLauncherDeployTaskSettings structors
 *****************************************************************************/

SSessionLauncherDeployTaskSettings::~SSessionLauncherDeployTaskSettings( )
{

}


/* SSessionLauncherDeployTaskSettings interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSessionLauncherDeployTaskSettings::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel )
{
	Model = InModel;

	ChildSlot
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			[
				SNew(SScrollBox)

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
								SNew(SSessionLauncherDeployPage, InModel, true)
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
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SSessionLauncherDeployTaskSettings callbacks
*****************************************************************************/


#undef LOCTEXT_NAMESPACE
