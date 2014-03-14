// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "PaperTileMapDetailsCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "EdModeTileMap.h"
#include "PropertyEditing.h"

#include "STileLayerList.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "Paper2D"

//////////////////////////////////////////////////////////////////////////
// FPaperTileMapDetailsCustomization

TSharedRef<IDetailCustomization> FPaperTileMapDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FPaperTileMapDetailsCustomization);
}

void FPaperTileMapDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	const TArray< TWeakObjectPtr<UObject> >& SelectedObjects = DetailLayout.GetDetailsView().GetSelectedObjects();
	
	UPaperTileMapRenderComponent* TileMap = NULL;
	for (int32 ObjectIndex = 0; ObjectIndex < SelectedObjects.Num(); ++ObjectIndex)
	{
		if (AActor* CurrentActor = Cast<AActor>(SelectedObjects[ObjectIndex].Get()))
		{
			if (UPaperTileMapRenderComponent* CurrentTileMap = CurrentActor->FindComponentByClass<UPaperTileMapRenderComponent>())
			{
				TileMap = CurrentTileMap;
				break;
			}
		}
	}
	TileMapPtr = TileMap;

	IDetailCategoryBuilder& TileMapCategory = DetailLayout.EditCategory("TileMap");

	TileMapCategory
	.AddCustomRow(TEXT("EnterEditMode"))
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		[
// 			SNew(SHorizontalBox)
// 			+SHorizontalBox::Slot()
// 			.AutoWidth()
// 			.Padding(10,5)
// 			[
				SNew(SButton)
				.ContentPadding(3)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.OnClicked(this, &FPaperTileMapDetailsCustomization::EnterTileMapEditingMode)
				.Visibility(this, &FPaperTileMapDetailsCustomization::GetNonEditModeVisibility)
				.Text( LOCTEXT( "EnterTileMapEditMode", "Enter Edit Mode" ).ToString() )
//			]
		]
	];

	//@TODO: Handle showing layers when multiple tile maps are selected
	if (TileMap != NULL)
	{
		IDetailCategoryBuilder& LayersCategory = DetailLayout.EditCategory("Tile Layers");

		LayersCategory.AddCustomRow(TEXT("Tile layer list"))
		[
			SNew(STileLayerList, TileMap)
		];

		LayersCategory.AddCustomRow(TEXT("Add Layer, Add Collision Layer"))
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			[
 				SNew(SHorizontalBox)
 				+SHorizontalBox::Slot()
 				.AutoWidth()
 				.Padding(10,5)
 				[
					SNew(SButton)
					.ContentPadding(3)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.OnClicked(this, &FPaperTileMapDetailsCustomization::AddLayerClicked)
					.Text( LOCTEXT( "AddLayer", "Add Layer" ).ToString() )
				]

				+SHorizontalBox::Slot()
 				.AutoWidth()
 				.Padding(10,5)
 				[
					SNew(SButton)
					.ContentPadding(3)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.OnClicked(this, &FPaperTileMapDetailsCustomization::AddCollisionLayerClicked)
					.Text( LOCTEXT( "AddCollisionLayer", "Add Collision Layer" ).ToString() )
				]
			]
		];
	}

	// Make sure the setup category is near the top
	DetailLayout.EditCategory("Setup");
}

FReply FPaperTileMapDetailsCustomization::EnterTileMapEditingMode()
{
	GEditorModeTools().ActivateMode(FEdModeTileMap::EM_TileMap);
	return FReply::Handled();
}

FReply FPaperTileMapDetailsCustomization::AddLayerClicked()
{
	AddLayer(false);

	return FReply::Handled();
}

FReply FPaperTileMapDetailsCustomization::AddCollisionLayerClicked()
{
	AddLayer(true);

	return FReply::Handled();
}

EVisibility FPaperTileMapDetailsCustomization::GetNonEditModeVisibility() const
{
	return GEditorModeTools().IsModeActive(FEdModeTileMap::EM_TileMap) ? EVisibility::Collapsed : EVisibility::Visible;
}

UPaperTileLayer* FPaperTileMapDetailsCustomization::AddLayer(bool bCollisionLayer)
{
	UPaperTileLayer* NewLayer = NULL;

	if (UPaperTileMapRenderComponent* TileMap = TileMapPtr.Get())
	{
		const FScopedTransaction Transaction( LOCTEXT("TileMapAddLayer", "Add New Layer") );
		TileMap->SetFlags(RF_Transactional);
		TileMap->Modify();

		NewLayer = NewObject<UPaperTileLayer>(TileMap);
		NewLayer->SetFlags(RF_Transactional);

		NewLayer->LayerWidth = TileMap->MapWidth;
		NewLayer->LayerHeight = TileMap->MapHeight;
		NewLayer->DestructiveAllocateMap(NewLayer->LayerWidth, NewLayer->LayerHeight);
		NewLayer->TileSet = TileMap->DefaultLayerTileSet;
		NewLayer->LayerName = LOCTEXT("DefaultNewLayerName", "New Layer");
		NewLayer->bCollisionLayer = bCollisionLayer;

		TileMap->TileLayers.Add(NewLayer);
	}

	return NewLayer;
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE