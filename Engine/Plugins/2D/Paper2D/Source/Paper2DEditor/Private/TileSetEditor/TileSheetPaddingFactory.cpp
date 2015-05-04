// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "TileSheetPaddingFactory.h"
#include "Atlasing/PaperAtlasTextureHelpers.h"

#define LOCTEXT_NAMESPACE "Paper2D"

/////////////////////////////////////////////////////
// UTileSheetPaddingFactory

UTileSheetPaddingFactory::UTileSheetPaddingFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = false;
	bEditAfterNew = true;
	SupportedClass = UTexture::StaticClass();

	ExtrusionAmount = 2;
	bPadToPowerOf2 = false;
}

UObject* UTileSheetPaddingFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	check(SourceTileSet);
	check(SourceTileSet->TileSheet);

	UTexture* SourceTexture = SourceTileSet->TileSheet;

	if (SourceTexture->Source.GetFormat() != TSF_BGRA8)
	{
		UE_LOG(LogPaper2DEditor, Error, TEXT("Tile sheet texture '%s' is not BGRA8, cannot create a padded texture from it"), *SourceTexture->GetName());
		return nullptr;
	}

	// Determine how big the new texture needs to be
	const int32 NumTilesX = SourceTileSet->GetTileCountX();
	const int32 NumTilesY = SourceTileSet->GetTileCountY();
	const int32 TileWidth = SourceTileSet->TileWidth;
	const int32 TileHeight = SourceTileSet->TileHeight;

	const uint32 NewMinTextureWidth = (uint32)(NumTilesX * (TileWidth + 2 * ExtrusionAmount) + (2 * ExtrusionAmount));
	const uint32 NewMinTextureHeight = (uint32)(NumTilesY * (TileHeight + 2 * ExtrusionAmount) + (2 * ExtrusionAmount));

	const uint32 NewTextureWidth = bPadToPowerOf2 ? FMath::RoundUpToPowerOfTwo(NewMinTextureWidth) : NewMinTextureWidth;
	const uint32 NewTextureHeight = bPadToPowerOf2 ? FMath::RoundUpToPowerOfTwo(NewMinTextureHeight) : NewMinTextureHeight;

	UTexture2D* Result = NewObject<UTexture2D>(InParent, Name, Flags | RF_Transactional);

	//@TODO: Copy more state across (or start by duplicating the texture maybe?)
	Result->LODGroup = SourceTexture->LODGroup;
	Result->CompressionSettings = SourceTexture->CompressionSettings;
	Result->MipGenSettings = TMGS_NoMipmaps; //@TODO: Don't actually want this...
	Result->DeferCompression = true;

	TArray<uint8> NewTextureData;
	NewTextureData.AddZeroed(NewTextureWidth * NewTextureHeight * sizeof(FColor));

	const FIntPoint TileWH(TileWidth, TileHeight);
	for (int32 TileY = 0; TileY < NumTilesY; ++TileY)
	{
		for (int32 TileX = 0; TileX < NumTilesX; ++TileX)
		{
			const FIntPoint TileUV = SourceTileSet->GetTileUVFromTileXY(FIntPoint(TileX, TileY));

			TArray<uint8> DummyBuffer;
			FPaperAtlasTextureHelpers::ReadSpriteTexture(SourceTexture, TileUV, TileWH, DummyBuffer);

			FPaperSpriteAtlasSlot Slot;
			Slot.X = TileX * (TileWidth + (2 * ExtrusionAmount));
			Slot.Y = TileY * (TileHeight + (2 * ExtrusionAmount));
			Slot.Width = TileWidth;// +(2 * ExtrusionAmount);
			Slot.Height = TileHeight;// +(2 * ExtrusionAmount);
			Slot.AtlasIndex = 0;

			FPaperAtlasTextureHelpers::CopyTextureRegionToAtlasTextureData(NewTextureData, NewTextureWidth, NewTextureHeight, sizeof(FColor), EPaperSpriteAtlasPadding::DilateBorder, ExtrusionAmount, DummyBuffer, TileWH, Slot);
		}
	}

	Result->Source.Init(NewTextureWidth, NewTextureHeight, 1, 1, TSF_BGRA8, NewTextureData.GetData());

	Result->UpdateResource();
	Result->PostEditChange();

	// Apply the new tile sheet to the specified tile set
	SourceTileSet->Modify();
	SourceTileSet->TileSheet = Result;
	SourceTileSet->Margin = ExtrusionAmount;
	SourceTileSet->Spacing = 2*ExtrusionAmount;
	SourceTileSet->PostEditChange();

	return Result;
}

#undef LOCTEXT_NAMESPACE