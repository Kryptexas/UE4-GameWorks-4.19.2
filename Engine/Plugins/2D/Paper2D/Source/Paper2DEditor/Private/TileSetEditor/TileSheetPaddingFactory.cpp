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

	UTexture* SourceTexture = SourceTileSet->GetTileSheetTexture();
	check(SourceTexture);

	if (SourceTexture->Source.GetFormat() != TSF_BGRA8)
	{
		UE_LOG(LogPaper2DEditor, Error, TEXT("Tile sheet texture '%s' is not BGRA8, cannot create a padded texture from it"), *SourceTexture->GetName());
		return nullptr;
	}

	// Determine how big the new texture needs to be
	const int32 NumTilesX = SourceTileSet->GetTileCountX();
	const int32 NumTilesY = SourceTileSet->GetTileCountY();
	const FIntPoint TileSize = SourceTileSet->GetTileSize();

	const uint32 NewMinTextureWidth = (uint32)(NumTilesX * (TileSize.X + 2 * ExtrusionAmount) + (2 * ExtrusionAmount));
	const uint32 NewMinTextureHeight = (uint32)(NumTilesY * (TileSize.Y + 2 * ExtrusionAmount) + (2 * ExtrusionAmount));

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

	for (int32 TileY = 0; TileY < NumTilesY; ++TileY)
	{
		for (int32 TileX = 0; TileX < NumTilesX; ++TileX)
		{
			const FIntPoint TileUV = SourceTileSet->GetTileUVFromTileXY(FIntPoint(TileX, TileY));

			TArray<uint8> DummyBuffer;
			FPaperAtlasTextureHelpers::ReadSpriteTexture(SourceTexture, TileUV, TileSize, DummyBuffer);

			FPaperSpriteAtlasSlot Slot;
			Slot.X = TileX * (TileSize.X + (2 * ExtrusionAmount));
			Slot.Y = TileY * (TileSize.Y + (2 * ExtrusionAmount));
			Slot.Width = TileSize.X;// +(2 * ExtrusionAmount);
			Slot.Height = TileSize.Y;// +(2 * ExtrusionAmount);
			Slot.AtlasIndex = 0;

			FPaperAtlasTextureHelpers::CopyTextureRegionToAtlasTextureData(NewTextureData, NewTextureWidth, NewTextureHeight, sizeof(FColor), EPaperSpriteAtlasPadding::DilateBorder, ExtrusionAmount, DummyBuffer, TileSize, Slot);
		}
	}

	Result->Source.Init(NewTextureWidth, NewTextureHeight, 1, 1, TSF_BGRA8, NewTextureData.GetData());

	Result->UpdateResource();
	Result->PostEditChange();

	// Apply the new tile sheet to the specified tile set
	SourceTileSet->Modify();
	SourceTileSet->SetTileSheetTexture(Result);
	SourceTileSet->SetMargin(FIntMargin(ExtrusionAmount));
	SourceTileSet->SetPerTileSpacing(FIntPoint(2 * ExtrusionAmount, 2 * ExtrusionAmount));
	SourceTileSet->PostEditChange();

	return Result;
}

#undef LOCTEXT_NAMESPACE