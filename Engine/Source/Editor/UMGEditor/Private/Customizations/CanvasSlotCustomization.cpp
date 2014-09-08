// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "PropertyEditing.h"
#include "ObjectEditorUtils.h"
#include "WidgetGraphSchema.h"
#include "ScopedTransaction.h"
#include "BlueprintEditorUtils.h"

#include "CanvasSlotCustomization.h"

#define LOCTEXT_NAMESPACE "UMG"

// FCanvasSlotCustomization
////////////////////////////////////////////////////////////////////////////////

void FCanvasSlotCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

void FCanvasSlotCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	CustomizeLayoutData(PropertyHandle, ChildBuilder, CustomizationUtils);

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

void FCanvasSlotCustomization::CustomizeLayoutData(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TSharedPtr<IPropertyHandle> LayoutData = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(UCanvasPanelSlot, LayoutData));
	if ( LayoutData.IsValid() )
	{
		LayoutData->MarkHiddenByCustomization();

		CustomizeAnchors(LayoutData, ChildBuilder, CustomizationUtils);
		CustomizeOffsets(LayoutData, ChildBuilder, CustomizationUtils);

		TSharedPtr<IPropertyHandle> AlignmentHandle = LayoutData->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnchorData, Alignment));
		AlignmentHandle->MarkHiddenByCustomization();
		ChildBuilder.AddChildProperty(AlignmentHandle.ToSharedRef());
	}
}

void FCanvasSlotCustomization::CustomizeOffsets(TSharedPtr<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TSharedPtr<IPropertyHandle> OffsetsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnchorData, Offsets));
	TSharedPtr<IPropertyHandle> AnchorsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnchorData, Anchors));

	OffsetsHandle->MarkHiddenByCustomization();

	TSharedPtr<IPropertyHandle> LeftHandle = OffsetsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMargin, Left));
	TSharedPtr<IPropertyHandle> TopHandle = OffsetsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMargin, Top));
	TSharedPtr<IPropertyHandle> RightHandle = OffsetsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMargin, Right));
	TSharedPtr<IPropertyHandle> BottomHandle = OffsetsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMargin, Bottom));

	IDetailPropertyRow& LeftRow = ChildBuilder.AddChildProperty(LeftHandle.ToSharedRef());
	IDetailPropertyRow& TopRow = ChildBuilder.AddChildProperty(TopHandle.ToSharedRef());
	IDetailPropertyRow& RightRow = ChildBuilder.AddChildProperty(RightHandle.ToSharedRef());
	IDetailPropertyRow& BottomRow = ChildBuilder.AddChildProperty(BottomHandle.ToSharedRef());

	TAttribute<FText> LeftLabel = TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateStatic(&FCanvasSlotCustomization::GetOffsetLabel, PropertyHandle, Orient_Horizontal, LOCTEXT("PositionX", "Position X"), LOCTEXT("OffsetLeft", "Offset Left")));
	TAttribute<FText> TopLabel = TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateStatic(&FCanvasSlotCustomization::GetOffsetLabel, PropertyHandle, Orient_Vertical, LOCTEXT("PositionY", "Position Y"), LOCTEXT("OffsetTop", "Offset Top")));
	TAttribute<FText> RightLabel = TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateStatic(&FCanvasSlotCustomization::GetOffsetLabel, PropertyHandle, Orient_Horizontal, LOCTEXT("SizeX", "Size X"), LOCTEXT("OffsetRight", "Offset Right")));
	TAttribute<FText> BottomLabel = TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateStatic(&FCanvasSlotCustomization::GetOffsetLabel, PropertyHandle, Orient_Vertical, LOCTEXT("SizeY", "Size Y"), LOCTEXT("OffsetBottom", "Offset Bottom")));

	CreateEditorWithDynamicLabel(LeftRow, LeftLabel);
	CreateEditorWithDynamicLabel(TopRow, TopLabel);
	CreateEditorWithDynamicLabel(RightRow, RightLabel);
	CreateEditorWithDynamicLabel(BottomRow, BottomLabel);
}

void FCanvasSlotCustomization::CreateEditorWithDynamicLabel(IDetailPropertyRow& PropertyRow, TAttribute<FText> TextAttribute)
{
	TSharedPtr<SWidget> NameWidget;
	TSharedPtr<SWidget> ValueWidget;
	FDetailWidgetRow Row;
	PropertyRow.GetDefaultWidgets(NameWidget, ValueWidget, Row);

	PropertyRow.CustomWidget(/*bShowChildren*/ true)
	.NameContent()
	[
		SNew(STextBlock)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(TextAttribute)
	]
	.ValueContent()
	[
		ValueWidget.ToSharedRef()
	];
}

FText FCanvasSlotCustomization::GetOffsetLabel(TSharedPtr<IPropertyHandle> PropertyHandle, EOrientation Orientation, FText NonStretchingLabel, FText StretchingLabel)
{
	TArray<UObject*> Objects;
	PropertyHandle->GetOuterObjects(Objects);
	
	if ( Objects.Num() > 1 )
	{
		return StretchingLabel;
	}

	TArray<void*> RawData;
	PropertyHandle->AccessRawData(RawData);

	FAnchorData* AnchorData = reinterpret_cast<FAnchorData*>( RawData[0] );

	const bool bStretching =
		( Orientation == Orient_Horizontal && AnchorData->Anchors.IsStretchedHorizontal() ) ||
		( Orientation == Orient_Vertical && AnchorData->Anchors.IsStretchedVertical() );

	return bStretching ? StretchingLabel : NonStretchingLabel;
}

void FCanvasSlotCustomization::CustomizeAnchors(TSharedPtr<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TSharedPtr<IPropertyHandle> AnchorsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnchorData, Anchors));

	AnchorsHandle->MarkHiddenByCustomization();

	IDetailPropertyRow& AnchorsPropertyRow = ChildBuilder.AddChildProperty(AnchorsHandle.ToSharedRef());

	AnchorsPropertyRow.CustomWidget(/*bShowChildren*/ true)
		.NameContent()
		[
			SNew(STextBlock)
			.Font( IDetailLayoutBuilder::GetDetailFont() )
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

FReply FCanvasSlotCustomization::OnAnchorClicked(TSharedPtr<IPropertyHandle> AnchorsHandle, FAnchors Anchors)
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
