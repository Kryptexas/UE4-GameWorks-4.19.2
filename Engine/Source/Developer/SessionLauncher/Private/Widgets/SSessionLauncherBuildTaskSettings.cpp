// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherBuildTaskSettings.cpp: Implements the SSessionLauncherBuildTaskSettings class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherBuildTaskSettings"


/* SSessionLauncherBuildTaskSettings structors
 *****************************************************************************/

SSessionLauncherBuildTaskSettings::~SSessionLauncherBuildTaskSettings( )
{

}


/* SSessionLauncherBuildTaskSettings interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSessionLauncherBuildTaskSettings::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel )
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

						// build section
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
								SNew(SSessionLauncherBuildPage, InModel)
							]
					]
			]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SSessionLauncherBuildTaskSettings callbacks
*****************************************************************************/


#undef LOCTEXT_NAMESPACE
