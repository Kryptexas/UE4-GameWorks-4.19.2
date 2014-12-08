// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ProjectLauncherPrivatePCH.h"
#include "SExpandableArea.h"


#define LOCTEXT_NAMESPACE "SProjectLauncherDeployToDeviceSettings"


/* SProjectLauncherDeployToDeviceSettings interface
 *****************************************************************************/

void SProjectLauncherDeployToDeviceSettings::Construct( const FArguments& InArgs, const FProjectLauncherModelRef& InModel, EVisibility InShowAdvanced )
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
						SNew(SProjectLauncherDeployTargets, InModel)
					]
			]

/*		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SVerticalBox)
				.Visibility(InShowAdvanced)

				+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SExpandableArea)
							.AreaTitle(LOCTEXT("AdvancedAreaTitle", "Advanced Settings"))
							.InitiallyCollapsed(true)
							.Padding(8.0f)
							.BodyContent()
							[
							]
					]
			]*/
	];
}


/* SProjectLauncherDeployToDeviceSettings callbacks
 *****************************************************************************/


#undef LOCTEXT_NAMESPACE
