// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "TileSetDetailsCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "PropertyEditing.h"
#include "ScopedTransaction.h"
#include "IPropertyUtilities.h"

#define LOCTEXT_NAMESPACE "Paper2D"

//////////////////////////////////////////////////////////////////////////
// FTileSetDetailsCustomization

FTileSetDetailsCustomization::FTileSetDetailsCustomization(bool bInIsEmbedded)
	: bIsEmbeddedInTileSetEditor(bInIsEmbedded)
	, SelectedSingleTileIndex(INDEX_NONE)
	, MyDetailLayout(nullptr)
{
}

TSharedRef<IDetailCustomization> FTileSetDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FTileSetDetailsCustomization(/*bIsEmbedded=*/ false));
}

TSharedRef<FTileSetDetailsCustomization> FTileSetDetailsCustomization::MakeEmbeddedInstance()
{
	return MakeShareable(new FTileSetDetailsCustomization(/*bIsEmbedded=*/ true));
}

void FTileSetDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	MyDetailLayout = &DetailLayout;
	
	for (const TWeakObjectPtr<UObject> SelectedObject : DetailLayout.GetDetailsView().GetSelectedObjects())
	{
		if (UPaperTileSet* TileSet = Cast<UPaperTileSet>(SelectedObject.Get()))
		{
			TileSetPtr = TileSet;
			break;
		}
	}

 	IDetailCategoryBuilder& TileSetCategory = DetailLayout.EditCategory("TileSet", FText::GetEmpty());

	// Add the width and height in cells of this tile set to the header
	TileSetCategory.HeaderContent
	(
		SNew(SBox)
		.HAlign(HAlign_Right)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.Padding(FMargin(5.0f, 0.0f))
			.AutoWidth()
			[
				SNew(STextBlock)
				.Font(FEditorStyle::GetFontStyle("TinyText"))
				.Text(this, &FTileSetDetailsCustomization::GetCellDimensionHeaderText)
				.ColorAndOpacity(this, &FTileSetDetailsCustomization::GetCellDimensionHeaderColor)
				.ToolTipText(LOCTEXT("NumCellsTooltip", "Number of tile cells in this tile set"))
			]
		]
	);


	if (bIsEmbeddedInTileSetEditor)
	{
		// Hide the array to start with
		const FName MetadataArrayName = UPaperTileSet::GetPerTilePropertyName();
		DetailLayout.HideProperty(MetadataArrayName);

		if (SelectedSingleTileIndex != INDEX_NONE)
		{
			// Customize for the single tile being edited
			IDetailCategoryBuilder& SingleTileCategory = DetailLayout.EditCategory("SingleTileEditor", FText::GetEmpty());

			const FString ArrayEntryPathPrefix = FString::Printf(TEXT("%s[%d]."), *MetadataArrayName.ToString(), SelectedSingleTileIndex);

			// Add all of the editable properties in the array entry
			for (TFieldIterator<UProperty> PropIt(FPaperTileMetadata::StaticStruct()); PropIt; ++PropIt)
			{
				UProperty* TestProperty = *PropIt;
				if (TestProperty->HasAnyPropertyFlags(CPF_Edit))
				{
					const bool bAdvancedDisplay = TestProperty->HasAnyPropertyFlags(CPF_AdvancedDisplay);
					const EPropertyLocation::Type PropertyLocation = bAdvancedDisplay ? EPropertyLocation::Advanced : EPropertyLocation::Common;

					const FString PropertyPath = ArrayEntryPathPrefix + TestProperty->GetName();
					TSharedRef<IPropertyHandle> PropertyHandle = DetailLayout.GetProperty(*PropertyPath);
					SingleTileCategory.AddProperty(PropertyHandle, PropertyLocation);
				}
			}

			// Add a display of the tile index being edited to the header
			const FText TileIndexHeaderText = FText::Format(LOCTEXT("SingleTileSectionHeader", "Editing Tile #{0}"), FText::AsNumber(SelectedSingleTileIndex));
			SingleTileCategory.HeaderContent
			(
				SNew(SBox)
				.HAlign(HAlign_Right)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.Padding(FMargin(5.0f, 0.0f))
					.AutoWidth()
					[
						SNew(STextBlock)
						.Font(FEditorStyle::GetFontStyle("TinyText"))
						.Text(TileIndexHeaderText)
					]
				]
			);
		}
	}
}

FText FTileSetDetailsCustomization::GetCellDimensionHeaderText() const
{
	FText Result;

	if (UPaperTileSet* TileSet = TileSetPtr.Get())
	{
		const int32 NumTilesX = TileSet->GetTileCountX();
		const int32 NumTilesY = TileSet->GetTileCountY();

		if (TileSet->TileSheet == nullptr)
		{
			Result = LOCTEXT("NoTexture", "No TileSheet");
		}
		else if (NumTilesX == 0)
		{
			Result = LOCTEXT("TextureTooNarrow", "TileSheet too narrow");
		}
		else if (NumTilesY == 0)
		{
			Result = LOCTEXT("TextureTooShort", "TileSheet too short");
		}
		else
		{
			const FText TextNumTilesX = FText::AsNumber(NumTilesX, &FNumberFormattingOptions::DefaultNoGrouping());
			const FText TextNumTilesY = FText::AsNumber(NumTilesY, &FNumberFormattingOptions::DefaultNoGrouping());
			Result = FText::Format(LOCTEXT("CellDimensions", "{0} x {0} tiles"), TextNumTilesX, TextNumTilesY);
		}
	}

	return Result;
}

FSlateColor FTileSetDetailsCustomization::GetCellDimensionHeaderColor() const
{
	if (UPaperTileSet* TileSet = TileSetPtr.Get())
	{
		if (TileSet->GetTileCount() == 0)
		{
			return FSlateColor(FLinearColor::Red);
		}
	}

	return FSlateColor::UseForeground();
}

void FTileSetDetailsCustomization::OnTileIndexChanged(int32 NewIndex, int32 OldIndex)
{
	SelectedSingleTileIndex = NewIndex;
	if (MyDetailLayout != nullptr)
	{
		MyDetailLayout->ForceRefreshDetails();
	}
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE