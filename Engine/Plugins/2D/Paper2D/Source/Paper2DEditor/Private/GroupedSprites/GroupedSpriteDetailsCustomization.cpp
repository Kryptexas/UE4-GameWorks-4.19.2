// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "GroupedSpriteDetailsCustomization.h"

#include "PaperSpriteActor.h"
#include "PaperSpriteComponent.h"
#include "PaperGroupedSpriteComponent.h"
#include "PaperGroupedSpriteUtilities.h"

#define LOCTEXT_NAMESPACE "SpriteEditor"

//////////////////////////////////////////////////////////////////////////
// FGroupedSpriteComponentDetailsCustomization

TSharedRef<IDetailCustomization> FGroupedSpriteComponentDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FGroupedSpriteComponentDetailsCustomization());
}

FGroupedSpriteComponentDetailsCustomization::FGroupedSpriteComponentDetailsCustomization()
{
}

void FGroupedSpriteComponentDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Create a category so this is displayed early in the properties
	IDetailCategoryBuilder& SpriteCategory = DetailBuilder.EditCategory("Sprite", FText::GetEmpty(), ECategoryPriority::Important);

	ObjectsBeingCustomized.Empty();
	DetailBuilder.GetObjectsBeingCustomized(/*out*/ ObjectsBeingCustomized);

	{
		// Expose split buttons
		FDetailWidgetRow& SplitRow = SpriteCategory.AddCustomRow(LOCTEXT("SplitSearchText", "Split"))
			.WholeRowContent()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f, 0.0f)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				[
					SNew(SButton)
					.Text(LOCTEXT("SplitSprites", "Split Sprites"))
					.ToolTipText(LOCTEXT("SplitSprites_Tooltip", "Splits all sprite instances into separate sprite actors or components"))
					.OnClicked(this, &FGroupedSpriteComponentDetailsCustomization::SplitSprites)
				]
			];
	}
}

FReply FGroupedSpriteComponentDetailsCustomization::SplitSprites()
{
	TArray<UObject*> StrongObjects;
	CopyFromWeakArray(StrongObjects, ObjectsBeingCustomized);

	FPaperGroupedSpriteUtilities::SplitSprites(StrongObjects);

	return FReply::Handled();
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
