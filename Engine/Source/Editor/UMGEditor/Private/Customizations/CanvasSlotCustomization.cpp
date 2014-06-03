// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "PropertyEditing.h"
#include "ObjectEditorUtils.h"
#include "WidgetGraphSchema.h"
#include "ScopedTransaction.h"
#include "BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "UMG"

void FCanvasSlotCustomization::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	// Build the layout category
	IDetailCategoryBuilder& LayoutCategory = DetailLayout.EditCategory("Layout");

	TSharedRef<IPropertyHandle> AnchorsHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCanvasPanelSlot, Anchors));
	IDetailPropertyRow& AnchorsPropertyRow = LayoutCategory.AddProperty(AnchorsHandle);

	const bool bShowChildren = true;
	AnchorsPropertyRow.CustomWidget(bShowChildren)
		.ValueContent()
		[
			SNew(SHorizontalBox)
					
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SComboButton)
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AnchorsText", "Anchors"))
				]
				.MenuContent()
				[
					SNew(SUniformGridPanel)

					// Top Row
					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(SButton)
						.OnClicked(this, &FCanvasSlotCustomization::OnAnchorClicked, AnchorsHandle, FAnchors(0, 0, 0, 0))
						[
							SNew(SImage)
							.Image(FEditorStyle::Get().GetBrush("UMGEditor.TopLeft"))
						]
					]

					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton)
						.OnClicked(this, &FCanvasSlotCustomization::OnAnchorClicked, AnchorsHandle, FAnchors(0.5, 0, 0.5, 0))
						[
							SNew(SImage)
							.Image(FEditorStyle::Get().GetBrush("UMGEditor.TopCenter"))
						]
					]

					+ SUniformGridPanel::Slot(2, 0)
					[
						SNew(SButton)
						.OnClicked(this, &FCanvasSlotCustomization::OnAnchorClicked, AnchorsHandle, FAnchors(1, 0, 1, 0))
						[
							SNew(SImage)
							.Image(FEditorStyle::Get().GetBrush("UMGEditor.TopRight"))
						]
					]

					+ SUniformGridPanel::Slot(3, 0)
					[
						SNew(SButton)
						.OnClicked(this, &FCanvasSlotCustomization::OnAnchorClicked, AnchorsHandle, FAnchors(0, 0, 1, 0))
						[
							SNew(SImage)
							.Image(FEditorStyle::Get().GetBrush("UMGEditor.TopFill"))
						]
					]

					// Center Row
					+ SUniformGridPanel::Slot(0, 1)
					[
						SNew(SButton)
						.OnClicked(this, &FCanvasSlotCustomization::OnAnchorClicked, AnchorsHandle, FAnchors(0, 0.5, 0, 0.5))
						[
							SNew(SImage)
							.Image(FEditorStyle::Get().GetBrush("UMGEditor.CenterLeft"))
						]
					]

					+ SUniformGridPanel::Slot(1, 1)
					[
						SNew(SButton)
						.OnClicked(this, &FCanvasSlotCustomization::OnAnchorClicked, AnchorsHandle, FAnchors(0.5, 0.5, 0.5, 0.5))
						[
							SNew(SImage)
							.Image(FEditorStyle::Get().GetBrush("UMGEditor.CenterCenter"))
						]
					]

					+ SUniformGridPanel::Slot(2, 1)
					[
						SNew(SButton)
						.OnClicked(this, &FCanvasSlotCustomization::OnAnchorClicked, AnchorsHandle, FAnchors(1, 0.5, 1, 0.5))
						[
							SNew(SImage)
							.Image(FEditorStyle::Get().GetBrush("UMGEditor.CenterRight"))
						]
					]

					+ SUniformGridPanel::Slot(3, 1)
					[
						SNew(SButton)
						.OnClicked(this, &FCanvasSlotCustomization::OnAnchorClicked, AnchorsHandle, FAnchors(0, 0.5f, 1, 0.5f))
						[
							SNew(SImage)
							.Image(FEditorStyle::Get().GetBrush("UMGEditor.CenterFill"))
						]
					]

					// Bottom Row
					+ SUniformGridPanel::Slot(0, 2)
					[
						SNew(SButton)
						.OnClicked(this, &FCanvasSlotCustomization::OnAnchorClicked, AnchorsHandle, FAnchors(0, 1, 0, 1))
						[
							SNew(SImage)
							.Image(FEditorStyle::Get().GetBrush("UMGEditor.BottomLeft"))
						]
					]

					+ SUniformGridPanel::Slot(1, 2)
					[
						SNew(SButton)
						.OnClicked(this, &FCanvasSlotCustomization::OnAnchorClicked, AnchorsHandle, FAnchors(0.5, 1, 0.5, 1))
						[
							SNew(SImage)
							.Image(FEditorStyle::Get().GetBrush("UMGEditor.BottomCenter"))
						]
					]

					+ SUniformGridPanel::Slot(2, 2)
					[
						SNew(SButton)
						.OnClicked(this, &FCanvasSlotCustomization::OnAnchorClicked, AnchorsHandle, FAnchors(1, 1, 1, 1))
						[
							SNew(SImage)
							.Image(FEditorStyle::Get().GetBrush("UMGEditor.BottomRight"))
						]
					]

					+ SUniformGridPanel::Slot(3, 2)
					[
						SNew(SButton)
						.OnClicked(this, &FCanvasSlotCustomization::OnAnchorClicked, AnchorsHandle, FAnchors(0, 1, 1, 1))
						[
							SNew(SImage)
							.Image(FEditorStyle::Get().GetBrush("UMGEditor.BottomFill"))
						]
					]

					// Fill Row
					+ SUniformGridPanel::Slot(0, 3)
					[
						SNew(SButton)
						.OnClicked(this, &FCanvasSlotCustomization::OnAnchorClicked, AnchorsHandle, FAnchors(0, 0, 0, 1))
						[
							SNew(SImage)
							.Image(FEditorStyle::Get().GetBrush("UMGEditor.FillLeft"))
						]
					]

					+ SUniformGridPanel::Slot(1, 3)
					[
						SNew(SButton)
						.OnClicked(this, &FCanvasSlotCustomization::OnAnchorClicked, AnchorsHandle, FAnchors(0.5, 0, 0.5, 1))
						[
							SNew(SImage)
							.Image(FEditorStyle::Get().GetBrush("UMGEditor.FillCenter"))
						]
					]

					+ SUniformGridPanel::Slot(2, 3)
					[
						SNew(SButton)
						.OnClicked(this, &FCanvasSlotCustomization::OnAnchorClicked, AnchorsHandle, FAnchors(1, 0, 1, 1))
						[
							SNew(SImage)
							.Image(FEditorStyle::Get().GetBrush("UMGEditor.FillRight"))
						]
					]

					+ SUniformGridPanel::Slot(3, 3)
					[
						SNew(SButton)
						.OnClicked(this, &FCanvasSlotCustomization::OnAnchorClicked, AnchorsHandle, FAnchors(1, 1, 1, 1))
						[
							SNew(SImage)
							.Image(FEditorStyle::Get().GetBrush("UMGEditor.FillFill"))
						]
					]
				]
			]
		];
}

FReply FCanvasSlotCustomization::OnAnchorClicked(TSharedRef<IPropertyHandle> AnchorsHandle, FAnchors Anchors)
{
	FString Value = FString::Printf(TEXT("(Minimum=(X=%f,Y=%f),Maximum=(X=%f,Y=%f))"), Anchors.Minimum.X, Anchors.Minimum.Y, Anchors.Maximum.X, Anchors.Maximum.Y);
	AnchorsHandle->SetValueFromFormattedString(Value);
	
	FSlateApplication::Get().DismissAllMenus();

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
