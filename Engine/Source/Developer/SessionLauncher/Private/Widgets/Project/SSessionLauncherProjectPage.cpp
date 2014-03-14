// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherProjectPage.cpp: Implements the SSessionLauncherProjectPage class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherProjectPage"


/* SSessionLauncherProjectPage interface
 *****************************************************************************/

void SSessionLauncherProjectPage::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel, bool InShowConfig )
{
	Model = InModel;

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
					.FillWidth(1.0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("WhichProjectToUseText", "Which project would you like to use?").ToString())			
					]
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0, 8.0, 0.0, 0.0)
			[
				// project loading area
				SNew(SSessionLauncherProjectPicker, InModel, InShowConfig)
			]
	];
}


#undef LOCTEXT_NAMESPACE
