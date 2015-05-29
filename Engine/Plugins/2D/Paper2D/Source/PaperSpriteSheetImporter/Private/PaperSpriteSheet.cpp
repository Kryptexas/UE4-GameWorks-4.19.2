// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PaperSpriteSheetImporterPrivatePCH.h"
#include "PaperSpriteSheet.h"

UPaperSpriteSheet::UPaperSpriteSheet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	AssetImportData = CreateEditorOnlyDefaultSubobject<UAssetImportData>(TEXT("AssetImportData"));
#endif
}

#if WITH_EDITORONLY_DATA
void UPaperSpriteSheet::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	if (AssetImportData)
	{
		OutTags.Add( FAssetRegistryTag(SourceFileTagName(), AssetImportData->ToJson(), FAssetRegistryTag::TT_Hidden) );
	}

	Super::GetAssetRegistryTags(OutTags);
}
void UPaperSpriteSheet::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_ASSET_IMPORT_DATA_AS_JSON && !AssetImportData)
	{
		// AssetImportData should always be valid
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
}
#endif