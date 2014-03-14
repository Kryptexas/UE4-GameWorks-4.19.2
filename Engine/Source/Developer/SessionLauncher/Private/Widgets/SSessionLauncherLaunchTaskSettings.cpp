// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherLaunchTaskSettings.cpp: Implements the SSessionLauncherLaunchTaskSettings class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherLaunchTaskSettings"


/* SSessionLauncherLaunchTaskSettings structors
 *****************************************************************************/

SSessionLauncherLaunchTaskSettings::~SSessionLauncherLaunchTaskSettings( )
{

}


/* SSessionLauncherLaunchTaskSettings interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSessionLauncherLaunchTaskSettings::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel )
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
								SNew(SSessionLauncherProjectPage, InModel, false)
							]

						// cook section
/*						+ SGridPanel::Slot(0, 3)
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
								SNew(SSessionLauncherSimpleCookPage, InModel)
							]*/

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
									.Text(LOCTEXT("TargetSectionHeader", "Target").ToString())
							]

						+ SGridPanel::Slot(1, 10)
							.HAlign(HAlign_Fill)
							.Padding(32.0f, 0.0f, 8.0f, 0.0f)
							[
								SNew(SSessionLauncherDeployToDeviceSettings, InModel, EVisibility::Hidden)
							]
					]
			]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SSessionLauncherLaunchTaskSettings callbacks
*****************************************************************************/


#undef LOCTEXT_NAMESPACE
