// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "PropertyEditing.h"
#include "ObjectEditorUtils.h"
#include "WidgetGraphSchema.h"
#include "ScopedTransaction.h"
#include "BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "UMG"

void FCanvasSlotCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Build the layout category
	//IDetailCategoryBuilder& LayoutCategory = DetailLayout.EditCategory("Layout");
}

void FCanvasSlotCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	CustomizeAnchors(PropertyHandle, ChildBuilder, CustomizationUtils);

	FillOutChildren(PropertyHandle, ChildBuilder, CustomizationUtils);
}

void FCanvasSlotCustomization::FillOutChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Generate all the other children
	uint32 NumChildren;
	PropertyHandle->GetNumChildren(NumChildren);

	for ( uint32 ChildIndex = 0; ChildIndex < NumChildren; ChildIndex++ )
	{
		TSharedRef<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
		if ( ChildHandle->IsCustomized() )
		{
			continue;
		}

		if ( ChildHandle->GetProperty() == NULL )
		{
			FillOutChildren(ChildHandle, ChildBuilder, CustomizationUtils);
		}
		else
		{
			ChildBuilder.AddChildProperty(ChildHandle);
		}
	}
}

void FCanvasSlotCustomization::CustomizeAnchors(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TSharedPtr<IPropertyHandle> Slot = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnchorData, Anchors));

	if ( !Slot.IsValid() )
	{
		return;
	}

	Slot->MarkHiddenByCustomization();

	TSharedRef<IPropertyHandle> AnchorsHandle = Slot.ToSharedRef();

	IDetailPropertyRow& AnchorsPropertyRow = ChildBuilder.AddChildProperty(AnchorsHandle);

	const bool bShowChildren = true;
	AnchorsPropertyRow.CustomWidget(bShowChildren)
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Anchors", "Anchors"))
		]
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
						.OnClicked(this, &FCanvasSlotCustomization::OnAnchorClicked, AnchorsHandle, FAnchors(0, 0, 1, 1))
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
	//TODO UMG Group under single undo transaction.

	FString Value = FString::Printf(TEXT("(Minimum=(X=%f,Y=%f),Maximum=(X=%f,Y=%f))"), Anchors.Minimum.X, Anchors.Minimum.Y, Anchors.Maximum.X, Anchors.Maximum.Y);
	AnchorsHandle->SetValueFromFormattedString(Value);

	//TODO UMG Reset position, pivot.
	//TODO UMG Moving the anchors shouldn't change the position of the widget, unless stretched.  But maybe that too should be based off current position and size.
	//         Either way need access to cached geometry of the slot in the preview world...
	
	FSlateApplication::Get().DismissAllMenus();

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
