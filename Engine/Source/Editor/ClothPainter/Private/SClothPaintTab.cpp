// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SClothPaintTab.h"

#include "AssetRegistryModule.h"
#include "ClassViewerModule.h"
#include "ClassViewerFilter.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Dialogs.h"
#include "SCheckBox.h"
#include "SBox.h"
#include "SImage.h"
#include "STextBlock.h"
#include "Toolkits/AssetEditorManager.h"

#include "ClothPaintingModule.h"
#include "ClothPainter.h"
#include "ClothingPaintEditMode.h"

#include "IPersonaPreviewScene.h"
#include "Classes/Animation/DebugSkelMeshComponent.h"

#include "AssetEditorModeManager.h"

#include "ISkeletalMeshEditor.h"

#define LOCTEXT_NAMESPACE "SClothPaintTab"

SClothPaintTab::SClothPaintTab() 
	: bModeApplied(false), bPaintModeEnabled(false)
{	
}

SClothPaintTab::~SClothPaintTab()
{
	if(ISkeletalMeshEditor* SkeletalMeshEditor = static_cast<ISkeletalMeshEditor*>(HostingApp.Pin().Get()))
	{
		SkeletalMeshEditor->GetAssetEditorModeManager()->ActivateDefaultMode();
	}
}

void SClothPaintTab::Construct(const FArguments& InArgs)
{
	HostingApp = InArgs._InHostingApp;

	ModeWidget = SNullWidget::NullWidget;
	
	FSlateIcon TexturePaintIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.MeshPaintMode.TexturePaint");

	this->ChildSlot
	[
		SNew(SOverlay)
		+SOverlay::Slot()
		[
			SAssignNew(ContentBox, SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SCheckBox)
					.Type(ESlateCheckBoxType::ToggleButton)
					.IsChecked_Lambda([=]() { return bPaintModeEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;  })
					.OnCheckStateChanged_Lambda([=](ECheckBoxState NewState) { bPaintModeEnabled = (NewState == ECheckBoxState::Checked); UpdatePaintTools(); })
					.Style(&FEditorStyle::Get().GetWidgetStyle< FCheckBoxStyle >("ToggleButtonCheckbox"))
					[
						SNew(SBox)
						.MinDesiredHeight(25.0f)
						.MinDesiredWidth(100.0f)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(FMargin(4.0f, 4.0f, 4.0f, 4.0f))
						[
							SNew(SVerticalBox)							
							+SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 4.0f)
							[
								SNew(SHorizontalBox)						
								+SHorizontalBox::Slot()
								//.AutoWidth()
								.HAlign(HAlign_Center)
								[
									SNew(SImage)
									.Image(TexturePaintIcon.GetIcon())
								]
								
							]
							+SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString("Enable Paint Tools"))
							]
						]
					]
				]
			]
		]
	];
}

void SClothPaintTab::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

void SClothPaintTab::UpdatePaintTools()
{
	if (bPaintModeEnabled)
	{
		ISkeletalMeshEditor* SkeletalMeshEditor = static_cast<ISkeletalMeshEditor*>(HostingApp.Pin().Get());
		SkeletalMeshEditor->GetAssetEditorModeManager()->ActivateMode(PaintModeID, true);

		FClothingPaintEditMode* PaintMode = (FClothingPaintEditMode*)SkeletalMeshEditor->GetAssetEditorModeManager()->FindMode(PaintModeID);
		if (PaintMode)
		{
			PaintMode->GetMeshPainter()->Reset();
			ModeWidget = PaintMode->GetMeshPainter()->GetWidget();
			PaintMode->SetPersonaToolKit(SkeletalMeshEditor->GetPersonaToolkit());

			ContentBox->AddSlot()
			.AutoHeight()
			[
				ModeWidget->AsShared()
			];
		}
	}
	else
	{
		ContentBox->RemoveSlot(ModeWidget->AsShared());
		ISkeletalMeshEditor* SkeletalMeshEditor = static_cast<ISkeletalMeshEditor*>(HostingApp.Pin().Get());
		SkeletalMeshEditor->GetAssetEditorModeManager()->ActivateDefaultMode();
		ModeWidget = SNullWidget::NullWidget;
	}
}

#undef LOCTEXT_NAMESPACE //"SClothPaintTab"