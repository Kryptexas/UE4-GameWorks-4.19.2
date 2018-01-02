// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "SkeletalMeshCustomizationHelpers.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "DetailLayoutBuilder.h"

#define LOCTEXT_NAMESPACE "SkeletalMeshCustomizationHelpers"

namespace SkeletalMeshCustomizationHelpers
{
	TSharedRef<SWidget> CreateAsyncSceneValueWidgetWithWarning(const TSharedPtr<IPropertyHandle>& AsyncScenePropertyHandle)
	{
		return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			.IsEnabled_Lambda([]() { return UPhysicsSettings::Get()->bEnableAsyncScene; })
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			[
				AsyncScenePropertyHandle->CreatePropertyValueWidget()
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.Padding(FMargin(0, 4))
			.Visibility_Lambda([]() { return UPhysicsSettings::Get()->bEnableAsyncScene ? EVisibility::Collapsed : EVisibility::Visible; })
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor::Yellow)
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(2)
					.HAlign(HAlign_Fill)
					.Clipping(EWidgetClipping::ClipToBounds)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0)
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("Icons.Warning"))
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(FMargin(2, 0))
						[
							SNew(STextBlock)
							.Font(IDetailLayoutBuilder::GetDetailFont())
							.Text(LOCTEXT("WarningForProjectAsyncSceneNotEnabled", "The project setting \"Enable Async Scene\" must be set."))
							.AutoWrapText(true)
							.ToolTipText(LOCTEXT("WarningForProjectAsyncSceneNotEnabledTooltip",
												 "The project setting \"Enable Async Scene\" must be set in order to use an async scene. "
												 "Otherwise, this property will be ignored."))
						]
					]
				]
			]
		];
	}
}

#undef LOCTEXT_NAMESPACE